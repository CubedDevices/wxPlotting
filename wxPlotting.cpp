/////////////////////////////////////////////////////////////////////////////
// Name:        wxPlotting.cpp
// Purpose:     Owner-drawn plotting control (MemoryDC double-buffer template)
// Copyright:   Kenneth S. Davis, 2026
// Licence:     GNU General Public License version 2
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2026 Kenneth S. Davis
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
/////////////////////////////////////////////////////////////////////////////
#include "wxPlotting.h"
#include "seriesrenderer.h"
#include "plotcoords.h"

#include "wx/brush.h"
#include "wx/dcclient.h"
#include "wx/dcmemory.h"
#include "wx/image.h"
#include "wx/pen.h"
#include "wx/settings.h"
#include "wx/stopwatch.h"
#include "wx/timer.h"

// Separate from the update-throttle timer so the shared wxEVT_TIMER handler
// can tell a resize-settle flush from a plot update.
enum
{
    ID_PLOT_RESIZE_SETTLE = wxID_HIGHEST + 1
};

#include <cmath>
#include <limits>
#include <vector>

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// Minimum canvas floor.  The effective canvas grows with the client area
// above this; scrollbars appear only when the client is smaller.
// Raise the minimum later (SetMinDrawingHeight) as series data is added.
const wxSize wxPlotting::DefaultMinDrawingSize(240, 240);

// ----------------------------------------------------------------------------
// event table / RTTI
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(wxPlotting, wxScrolledWindow);

wxBEGIN_EVENT_TABLE(wxPlotting, wxScrolledWindow)
    EVT_PAINT(wxPlotting::OnPaint)
    EVT_SIZE(wxPlotting::OnSize)
    EVT_ERASE_BACKGROUND(wxPlotting::OnEraseBackground)
    EVT_MOTION(wxPlotting::OnMotion)
    EVT_LEAVE_WINDOW(wxPlotting::OnLeaveWindow)
wxEND_EVENT_TABLE()

// ----------------------------------------------------------------------------
// construction / destruction
// ----------------------------------------------------------------------------

wxPlotting::wxPlotting()
    : m_minDrawingSize(DefaultMinDrawingSize),
      m_drawingSize(DefaultMinDrawingSize)
{
}

wxPlotting::wxPlotting(wxWindow* parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxString& name)
    : m_minDrawingSize(DefaultMinDrawingSize),
      m_drawingSize(DefaultMinDrawingSize)
{
    Create(parent, id, pos, size, style, name);
}

bool wxPlotting::Create(wxWindow* parent,
                        wxWindowID id,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        const wxString& name)
{
    // wxHSCROLL / wxVSCROLL enable scrollbars when the client is smaller
    // than the minimum drawing size.
    if ( !wxScrolledWindow::Create(parent, id, pos, size,
                                   style | wxFULL_REPAINT_ON_RESIZE |
                                   wxHSCROLL | wxVSCROLL,
                                   name) )
    {
        return false;
    }

    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxBLACK);
    SetForegroundColour(*wxWHITE);

    // Scroll a modest number of pixels per line/page unit.
    SetScrollRate(20, 20);
    EnableScrolling(true, true);

    m_updateThrottleTimer.SetOwner(this);
    m_resizeSettleTimer.SetOwner(this, ID_PLOT_RESIZE_SETTLE);
    Bind(wxEVT_TIMER, &wxPlotting::OnUpdateThrottleTimer, this);

    SyncDrawingSizeToClient();
    RecreateBuffer();
    Render();
    m_lastFlushMs = wxGetLocalTimeMillis();

    return true;
}

wxPlotting::~wxPlotting()
{
    if ( m_updateThrottleTimer.IsRunning() )
        m_updateThrottleTimer.Stop();
    if ( m_resizeSettleTimer.IsRunning() )
        m_resizeSettleTimer.Stop();
}

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------


bool wxPlotting::AddAxis(const Axis& axis)
{
    if ( axis.GetName().IsEmpty() )
        return false;

    if ( GetAxis(axis.GetName()) != nullptr )
        return false; // name collision

    m_axisList.push_back(axis);
    return true;
}

bool wxPlotting::AddSeries(const Series& series)
{
    if ( series.GetName().IsEmpty() )
        return false;

    if ( GetSeries(series.GetName()) != nullptr )
        return false; // name collision

    m_seriesList.push_back(series);
    return true;
}

bool wxPlotting::RemoveAxis(const wxString& name)
{
    if ( name.IsEmpty() )
        return false;

    for ( auto it = m_axisList.begin(); it != m_axisList.end(); ++it )
    {
        if ( it->GetName() == name )
        {
            m_axisList.erase(it);
            return true;
        }
    }
    return false;
}

bool wxPlotting::RemoveSeries(const wxString& name)
{
    if ( name.IsEmpty() )
        return false;

    for ( auto it = m_seriesList.begin(); it != m_seriesList.end(); ++it )
    {
        if ( it->GetName() == name )
        {
            m_seriesList.erase(it);
            return true;
        }
    }
    return false;
}

void wxPlotting::ClearAxes()
{
    m_axisList.clear();
}

void wxPlotting::ClearSeries()
{
    m_seriesList.clear();
}

void wxPlotting::Clear()
{
    ClearAxes();
    ClearSeries();
    // Drop a stale measure without an extra refresh; the caller repaints.
    m_measureVisible = false;
    m_measureHorzAxis.clear();
    m_measureVertAxis.clear();
}

Axis* wxPlotting::GetAxis(const wxString& name)
{
    if ( name.IsEmpty() )
        return nullptr;

    for ( auto& axis : m_axisList )
    {
        if ( axis.GetName() == name )
            return &axis;
    }
    return nullptr;
}

const Axis* wxPlotting::GetAxis(const wxString& name) const
{
    if ( name.IsEmpty() )
        return nullptr;

    for ( const auto& axis : m_axisList )
    {
        if ( axis.GetName() == name )
            return &axis;
    }
    return nullptr;
}

Series* wxPlotting::GetSeries(const wxString& name)
{
    if ( name.IsEmpty() )
        return nullptr;

    for ( auto& series : m_seriesList )
    {
        if ( series.GetName() == name )
            return &series;
    }
    return nullptr;
}

const Series* wxPlotting::GetSeries(const wxString& name) const
{
    if ( name.IsEmpty() )
        return nullptr;

    for ( const auto& series : m_seriesList )
    {
        if ( series.GetName() == name )
            return &series;
    }
    return nullptr;
}


void wxPlotting::ArrangeVerticalAxes(double spacing, double topInset)
{
    // Top → bottom: first registered visible vertical axis is highest on screen.
    if ( topInset < 0.0 )
        topInset = 0.0;
    if ( topInset > 0.4 )
        topInset = 0.4;
    ArrangeAxesOfType(Axis::Vertical, spacing, true, topInset);
}

void wxPlotting::ArrangeHorizontalAxes(double spacing)
{
    // Left → right: first registered visible horizontal axis is leftmost.
    ArrangeAxesOfType(Axis::Horizontal, spacing, false);
}

void wxPlotting::ArrangeAxesOfType(Axis::AxisType type, double spacing,
                                  bool topToBottom, double topInset)
{
    if ( spacing < 0.0 )
        spacing = 0.0;

    // Collect pointers in registration order (stable layout order).
    std::vector<Axis*> axes;
    axes.reserve(m_axisList.size());
    for ( auto& axis : m_axisList )
    {
        if ( !axis.Visible )
            continue;
        if ( axis.GetAxisType() != type )
            continue;
        axes.push_back(&axis);
        //axes.push_back(&axis);
    }

    const size_t n = axes.size();
    if ( n == 0 )
        return;

    // Annotation lanes take one share per text row so each row is as tall
    // as a digital channel. Other axes stay at one share.
    std::vector<double> weights(n, 1.0);
    double weightSum = 0.0;
    if ( type == Axis::Vertical )
    {
        for ( size_t i = 0; i < n; ++i )
        {
            for ( const Series& series : m_seriesList )
            {
                if ( !series.Visible )
                    continue;
                if ( series.GetSeriesType() != Series::Annotation )
                    continue;
                if ( series.VerticalAxis.CompareTo(axes[i]->GetName()) != 0 )
                    continue;
                int rows = series.AnnotationRowCount();
                if ( rows < 1 )
                    rows = 1;
                if ( static_cast<double>(rows) > weights[i] )
                    weights[i] = static_cast<double>(rows);
            }
            weightSum += weights[i];
        }
    }
    else
    {
        weightSum = static_cast<double>(n);
    }
    if ( weightSum <= 0.0 )
        weightSum = 1.0;

    // Vertical stack can stop short of the top border so a full-scale
    // sample is not drawn on the frame.
    const double topLimit =
        (type == Axis::Vertical && topToBottom) ? (1.0 - topInset) : 1.0;

    if ( n == 1 )
    {
        axes[0]->SetPosition(0.0, topLimit);
        return;
    }

    // Bands share the plot in proportion to weight, with fixed gaps.
    // Shrink gaps if they would leave no room.
    const double minBand = 0.02;
    double gap = spacing;
    double totalGap = gap * static_cast<double>(n - 1);
    double usable = 1.0 - totalGap;
    const double minTotal = minBand * weightSum;
    if ( usable < minTotal )
    {
        usable = minTotal;
        if ( usable > 1.0 )
            usable = 1.0;
        totalGap = 1.0 - usable;
        gap = (n > 1) ? totalGap / static_cast<double>(n - 1) : 0.0;
    }

    // Walk from the top. Each axis consumes its share, then a gap.
    double cursor = 1.0;
    for ( size_t i = 0; i < n; ++i )
    {
        const double band = usable * (weights[i] / weightSum);
        double lo = 0.0;
        double hi = 1.0;

        if ( type == Axis::Vertical && topToBottom )
        {
            hi = cursor;
            lo = cursor - band;
            cursor = lo - gap;
            hi *= topLimit;
            lo *= topLimit;
        }
        else
        {
            // Horizontal (or bottom-to-top if ever requested): low → high.
            lo = (1.0 - cursor);
            hi = lo + band;
            cursor -= band + gap;
        }

        // Numerical safety into [0, 1].
        if ( lo < 0.0 ) lo = 0.0;
        if ( hi > 1.0 ) hi = 1.0;
        if ( lo > hi )
        {
            const double t = lo;
            lo = hi;
            hi = t;
        }

        axes[i]->SetPosition(lo, hi);
    }
}

void wxPlotting::UpdatePlot(bool forceImmediate)
{
    // Mark a full buffer rebuild. High-rate callers (axis drag, streaming)
    // are coalesced to ~kMaxUpdateHz with a trailing flush so the last
    // state is always shown without saturating the UI thread on Windows.
    m_fullUpdatePending = true;

    if ( forceImmediate )
    {
        if ( m_updateThrottleTimer.IsRunning() )
            m_updateThrottleTimer.Stop();
        FlushPendingUpdate(true);
        return;
    }

    const wxLongLong now = wxGetLocalTimeMillis();
    const wxLongLong minIntervalMs = 1000 / kMaxUpdateHz; // ~16 ms at 60 Hz
    const wxLongLong elapsed = now - m_lastFlushMs;

    if ( m_lastFlushMs == 0 || elapsed >= minIntervalMs )
    {
        if ( m_updateThrottleTimer.IsRunning() )
            m_updateThrottleTimer.Stop();
        FlushPendingUpdate(false);
        return;
    }

    // Too soon: schedule a trailing update for the remainder of the interval.
    if ( !m_updateThrottleTimer.IsRunning() )
    {
        int delay = static_cast<int>((minIntervalMs - elapsed).GetValue());
        if ( delay < 1 )
            delay = 1;
        m_updateThrottleTimer.StartOnce(delay);
    }
}

void wxPlotting::RequestOverlayRefresh()
{
    m_overlayRefreshPending = true;

    const wxLongLong now = wxGetLocalTimeMillis();
    const wxLongLong minIntervalMs = 1000 / kMaxUpdateHz;
    const wxLongLong elapsed = now - m_lastFlushMs;

    if ( m_lastFlushMs == 0 || elapsed >= minIntervalMs )
    {
        if ( m_updateThrottleTimer.IsRunning() )
            m_updateThrottleTimer.Stop();
        FlushPendingUpdate(false);
        return;
    }

    if ( !m_updateThrottleTimer.IsRunning() )
    {
        int delay = static_cast<int>((minIntervalMs - elapsed).GetValue());
        if ( delay < 1 )
            delay = 1;
        m_updateThrottleTimer.StartOnce(delay);
    }
}

void wxPlotting::FlushPendingUpdate(bool forceImmediate)
{
    const bool needRender = m_fullUpdatePending;
    const bool needRefresh = m_fullUpdatePending || m_overlayRefreshPending;

    m_fullUpdatePending = false;
    m_overlayRefreshPending = false;

    if ( needRender )
        Render();

    if ( needRefresh )
        Refresh(false);

    if ( forceImmediate )
        Update();

    m_lastFlushMs = wxGetLocalTimeMillis();
}

void wxPlotting::OnUpdateThrottleTimer(wxTimerEvent& event)
{
    if ( event.GetId() == ID_PLOT_RESIZE_SETTLE )
    {
        OnResizeSettleTimer();
        return;
    }

    if ( m_fullUpdatePending || m_overlayRefreshPending )
        FlushPendingUpdate(false);
}

void wxPlotting::OnResizeSettleTimer()
{
    if ( !m_resizeRenderPending )
        return;

    Render();
    Refresh(false);
}

// ----------------------------------------------------------------------------
// hit-testing
// ----------------------------------------------------------------------------

namespace
{
    // Shared with drawing and hit-test.
    const int kTickLength = 10;
    const int kLabelPad = 4;

    int AxisTitleClearance(wxDC& dc, const Axis& axis, bool horizontal)
    {
        int clear = (axis.Width + 1) / 2 + kLabelPad;

        if ( axis.LabelSide != axis.TickSide )
            return clear + 10;

        if ( axis.TickVisible )
            clear += kTickLength;

        if ( axis.TickLabelVisible )
        {
            dc.SetFont(axis.TickFont);
            if ( horizontal )
            {
                clear += dc.GetCharHeight() + kLabelPad;
            }
            else
            {
                const wxString s0 = axis.FormatTickValue(axis.GetRangeStart());
                const wxString s1 = axis.FormatTickValue(axis.GetRangeEnd());
                const int w0 = dc.GetTextExtent(s0).x;
                const int w1 = dc.GetTextExtent(s1).x;
                clear += wxMax(w0, w1) + kLabelPad;
            }
        }
        else if ( !axis.TickVisible )
        {
            clear += 10;
        }

        return clear;
    }

    double DistancePointToSegment(const wxPoint& p, const wxPoint& a, const wxPoint& b)
    {
        const double abx = static_cast<double>(b.x - a.x);
        const double aby = static_cast<double>(b.y - a.y);
        const double apx = static_cast<double>(p.x - a.x);
        const double apy = static_cast<double>(p.y - a.y);
        const double abLen2 = abx * abx + aby * aby;
        if ( abLen2 <= 0.0 )
        {
            const double dx = apx;
            const double dy = apy;
            return std::sqrt(dx * dx + dy * dy);
        }

        double t = (apx * abx + apy * aby) / abLen2;
        if ( t < 0.0 ) t = 0.0;
        else if ( t > 1.0 ) t = 1.0;

        const double cx = a.x + t * abx;
        const double cy = a.y + t * aby;
        const double dx = p.x - cx;
        const double dy = p.y - cy;
        return std::sqrt(dx * dx + dy * dy);
    }

    // Axis hit region: the strip of valid plot positions for this axis,
    // extended to the full canvas so ticks, tick labels, and titles in the
    // margins are included without per-element geometry tests.
    //
    // Horizontal: X span [start,end] within m_plot, Y = entire canvas height.
    // Vertical:   Y span [start,end] within m_plot, X = entire canvas width.
    bool AxisRegionContains(const Axis& axis,
                            const wxRect& plot,
                            const wxSize& canvasSize,
                            const wxPoint& p)
    {
        if ( plot.IsEmpty() || plot.GetWidth() <= 0 || plot.GetHeight() <= 0 )
            return false;
        if ( canvasSize.x <= 0 || canvasSize.y <= 0 )
            return false;
        if ( p.x < 0 || p.y < 0 || p.x >= canvasSize.x || p.y >= canvasSize.y )
            return false;

        double lo = axis.GetStart();
        double hi = axis.GetEnd();
        if ( lo > hi )
        {
            const double t = lo;
            lo = hi;
            hi = t;
        }

        if ( axis.GetAxisType() == Axis::Horizontal )
        {
            // Full canvas height covers bottom/top margin ticks and labels.
            const int x0 = plot.x + wxRound(lo * plot.GetWidth());
            const int x1 = plot.x + wxRound(hi * plot.GetWidth());
            const int left = wxMin(x0, x1);
            const int right = wxMax(x0, x1);
            return p.x >= left && p.x <= right;
        }

        if ( axis.GetAxisType() == Axis::Vertical )
        {
            // Full canvas width covers left/right margin ticks and labels.
            // Y is inverted: larger normalized value is higher on screen.
            const int yTop = plot.y + plot.GetHeight()
                           - wxRound(hi * plot.GetHeight());
            const int yBot = plot.y + plot.GetHeight()
                           - wxRound(lo * plot.GetHeight());
            const int top = wxMin(yTop, yBot);
            const int bot = wxMax(yTop, yBot);
            return p.y >= top && p.y <= bot;
        }

        return false;
    }

    void CollectAxesAtPoint(const std::list<Axis>& axes,
                            const wxRect& plot,
                            const wxSize& canvasSize,
                            const wxPoint& canvasPos,
                            std::vector<wxString>& outNames)
    {
        outNames.clear();
        for ( const auto& axis : axes )
        {
            if ( !axis.Visible )
                continue;
            if ( AxisRegionContains(axis, plot, canvasSize, canvasPos) )
                outNames.push_back(axis.GetName());
        }
    }

    struct SeriesHitCandidate
    {
        bool        hit = false;
        double      distance = std::numeric_limits<double>::max();
        size_t      sampleIndex = 0;
        wxRealPoint dataPoint;
    };

    SeriesHitCandidate HitTestSeriesGeometry(
        const Series& series,
        const Axis& horz,
        const Axis& vert,
        const wxRect& plot,
        const wxPoint& canvasPos,
        int tolerancePx)
    {
        SeriesHitCandidate best;

        const Series::Points points = series.GetPoints();
        if ( points.size() < 1 )
            return best;

        // Build screen polyline with the same break rules as DrawSeries.
        struct MappedSample
        {
            bool     valid = false;
            wxPoint  screen;
            size_t   index = 0;
            wxRealPoint data;
        };

        std::vector<MappedSample> mapped;
        mapped.reserve(points.size());

        for ( size_t i = 0; i < points.size(); ++i )
        {
            MappedSample m;
            m.index = i;
            m.data = points[i];

            if ( !horz.IsValueInRange(points[i].x) || !vert.IsValueInRange(points[i].y) )
            {
                m.valid = false;
            }
            else
            {
                const wxRealPoint conv(horz.ConvertValue(points[i].x),
                                       vert.ConvertValue(points[i].y));
                m.screen = PlotToScreen(plot, conv);
                m.valid = true;
            }
            mapped.push_back(m);
        }

        auto considerSegment = [&](const wxPoint& a, const wxPoint& b,
                                   size_t idxA, size_t idxB,
                                   const wxRealPoint& dataA, const wxRealPoint& dataB)
        {
            const double d = DistancePointToSegment(canvasPos, a, b);
            if ( d > static_cast<double>(tolerancePx) )
                return;
            if ( !best.hit || d < best.distance )
            {
                best.hit = true;
                best.distance = d;
                // Nearer sample endpoint for tooltips / selection anchors.
                const double da = (canvasPos.x - a.x) * (canvasPos.x - a.x)
                                + (canvasPos.y - a.y) * (canvasPos.y - a.y);
                const double db = (canvasPos.x - b.x) * (canvasPos.x - b.x)
                                + (canvasPos.y - b.y) * (canvasPos.y - b.y);
                if ( da <= db )
                {
                    best.sampleIndex = idxA;
                    best.dataPoint = dataA;
                }
                else
                {
                    best.sampleIndex = idxB;
                    best.dataPoint = dataB;
                }
            }
        };

        // Points (and single-sample series): hit disk around each sample only.
        if ( series.RenderStyle == SeriesRenderStyle::Points ||
             mapped.size() == 1 )
        {
            for ( const auto& m : mapped )
            {
                if ( !m.valid )
                    continue;
                const double dx = canvasPos.x - m.screen.x;
                const double dy = canvasPos.y - m.screen.y;
                const double d = std::sqrt(dx * dx + dy * dy);
                if ( d > static_cast<double>(tolerancePx) )
                    continue;
                if ( !best.hit || d < best.distance )
                {
                    best.hit = true;
                    best.distance = d;
                    best.sampleIndex = m.index;
                    best.dataPoint = m.data;
                }
            }
            return best;
        }

        for ( size_t i = 1; i < mapped.size(); ++i )
        {
            if ( !mapped[i - 1].valid || !mapped[i].valid )
                continue; // gap / Break — no segment

            const wxPoint& a = mapped[i - 1].screen;
            const wxPoint& b = mapped[i].screen;

            if ( series.RenderStyle == SeriesRenderStyle::StairStep )
            {
                const wxPoint corner(b.x, a.y);
                considerSegment(a, corner,
                                mapped[i - 1].index, mapped[i].index,
                                mapped[i - 1].data, mapped[i].data);
                considerSegment(corner, b,
                                mapped[i - 1].index, mapped[i].index,
                                mapped[i - 1].data, mapped[i].data);
            }
            else
            {
                // Line, Fill, Custom (default): hit polyline segments.
                considerSegment(a, b,
                                mapped[i - 1].index, mapped[i].index,
                                mapped[i - 1].data, mapped[i].data);
            }
        }

        return best;
    }
} // namespace

PlotHitResult wxPlotting::HitTest(const wxPoint& clientPos, int tolerancePx) const
{
    int canvasX = clientPos.x;
    int canvasY = clientPos.y;
    // Match OnPaint / DoPrepareDC scroll mapping.
    CalcUnscrolledPosition(clientPos.x, clientPos.y, &canvasX, &canvasY);
    return HitTestCanvas(wxPoint(canvasX, canvasY), tolerancePx);
}

PlotHitResult wxPlotting::HitTestCanvas(const wxPoint& canvasPos, int tolerancePx) const
{
    PlotHitResult result;
    if ( tolerancePx < 0 )
        tolerancePx = 0;

    // Axes: full strips to the canvas borders (covers plot band + margins for
    // ticks, tick labels, and titles). No per-label/per-tick geometry.
    CollectAxesAtPoint(m_axisList, m_plot, m_drawingSize, canvasPos, result.axes);

 #ifdef WXPLOTTING_SERIES_HITTESTING
    // --- Series (front-most first: reverse draw order) ---------------------
    for ( auto it = m_seriesList.rbegin(); it != m_seriesList.rend(); ++it )
    {
        if ( !it->Visible )
            continue;

        const Axis* horz = GetAxis(it->HorizontalAxis);
        const Axis* vert = GetAxis(it->VerticalAxis);
        if ( !horz || !vert )
            continue;

        const SeriesHitCandidate cand =
            HitTestSeriesGeometry(*it, *horz, *vert, m_plot, canvasPos, tolerancePx);
        if ( cand.hit )
        {
            result.kind = PlotHitKind::Series;
            result.name = it->GetName();
            result.distancePx = cand.distance;
            result.sampleIndex = cand.sampleIndex;
            result.dataPoint = cand.dataPoint;
            // result.axes already filled for this canvas position
            return result;
        }
    }
#endif

    // --- Axes only (region hit, no series stroke) --------------------------
    if ( !result.axes.empty() )
    {
        result.kind = PlotHitKind::Axis;
        // Primary name: first axis in registration order (see axes vector).
        result.name = result.axes.front();
        result.distancePx = 0.0;
        return result;
    }

    // --- Empty data area (inside plot, no axis region — uncommon) ----------
    if ( m_plot.Contains(canvasPos) )
    {
        result.kind = PlotHitKind::PlotArea;
        return result;
    }

    result.kind = PlotHitKind::None;
    return result;
}

void wxPlotting::SetMargin(int value)
{
    SetMargins(value, value, value, value);
}

void wxPlotting::SetMargins(int left, int right, int top, int bottom)
{
    left   = wxMax(left, 0);
    right  = wxMax(right, 0);
    top    = wxMax(top, 0);
    bottom = wxMax(bottom, 0);

    if ( left == m_marginLeft && right == m_marginRight &&
         top == m_marginTop && bottom == m_marginBottom )
        return;

    m_marginLeft   = left;
    m_marginRight  = right;
    m_marginTop    = top;
    m_marginBottom = bottom;

    RecreateBuffer();
    Render();
    Refresh(false);
}

void wxPlotting::SetMarginLeft(int margin)
{
    SetMargins(margin, m_marginRight, m_marginTop, m_marginBottom);
}

void wxPlotting::SetMarginRight(int margin)
{
    SetMargins(m_marginLeft, margin, m_marginTop, m_marginBottom);
}

void wxPlotting::SetMarginTop(int margin)
{
    SetMargins(m_marginLeft, m_marginRight, margin, m_marginBottom);
}

void wxPlotting::SetMarginBottom(int margin)
{
    SetMargins(m_marginLeft, m_marginRight, m_marginTop, margin);
}

void wxPlotting::SetMinDrawingSize(const wxSize& size)
{
    if ( size.x < 1 || size.y < 1 )
        return;

    if ( size == m_minDrawingSize )
        return;

    m_minDrawingSize = size;

    // Effective canvas may grow (higher floor) or shrink toward client
    // (lower floor).  Scrollbars follow virtual vs client size.
    if ( SyncDrawingSizeToClient() )
    {
        RecreateBuffer();
        Render();
    }

    Refresh(false);
}

void wxPlotting::SetMinDrawingWidth(int width)
{
    SetMinDrawingSize(wxSize(width, m_minDrawingSize.y));
}

void wxPlotting::SetMinDrawingHeight(int height)
{
    SetMinDrawingSize(wxSize(m_minDrawingSize.x, height));
}


// ----------------------------------------------------------------------------
// sizing / preferred size
// ----------------------------------------------------------------------------

wxSize wxPlotting::DoGetBestClientSize() const
{
    // Best size matches the minimum so a snug layout needs no scrollbars.
    return m_minDrawingSize;
}

// ----------------------------------------------------------------------------
// buffer / scroll management
// ----------------------------------------------------------------------------

bool wxPlotting::SyncDrawingSizeToClient()
{
    // Canvas tracks the client as it grows, but never goes below the minimum.
    // Scrollbars appear only when client < min on an axis.
    const wxSize client = GetClientSize();
    const wxSize needed(wxMax(wxMax(client.x, 1), m_minDrawingSize.x),
                        wxMax(wxMax(client.y, 1), m_minDrawingSize.y));

    if ( needed == m_drawingSize )
    {
        if ( GetVirtualSize() != m_drawingSize )
            SetVirtualSize(m_drawingSize);
        return false;
    }

    m_drawingSize = needed;
    SetVirtualSize(m_drawingSize);
    return true;
}

void wxPlotting::UpdatePlotRect()
{
    const wxSize size = m_drawingSize;
    if ( size.x <= m_marginLeft + m_marginRight ||
         size.y <= m_marginTop + m_marginBottom )
        return;

    // Plot rectangle always tracks margins, even when the bitmap size is unchanged.
    m_plotSize = wxSize(size.x - m_marginLeft - m_marginRight,
                        size.y - m_marginTop - m_marginBottom);
    m_plot = wxRect(wxPoint(m_marginLeft, m_marginTop), m_plotSize);
}

void wxPlotting::RecreateBuffer()
{
    const wxSize size = m_drawingSize;
    if ( size.x <= m_marginLeft + m_marginRight ||
         size.y <= m_marginTop + m_marginBottom )
        return;

    UpdatePlotRect();

    // Use the window content scale so MemoryDC text matches on-screen DPI.
    // Without this, fonts are measured/drawn at 96 DPI and look undersized
    // on high-DPI displays while the rest of the UI is scaled up.
    const double scale = GetContentScaleFactor();
    const bool needNewBitmap =
        !m_buffer.IsOk() ||
        m_buffer.GetLogicalSize() != size ||
        !wxIsSameDouble(m_buffer.GetScaleFactor(), scale);

    if ( needNewBitmap )
    {
        m_buffer = wxBitmap();
        m_buffer.CreateScaled(size.x, size.y, wxBITMAP_SCREEN_DEPTH, scale);
    }
}

void wxPlotting::Render()
{
    wxStopWatch sw;

    RecreateBuffer();
    if ( !m_buffer.IsOk())
    {
        m_resizeRenderPending = false;
        return;
    }

    wxMemoryDC memDC;
    memDC.SelectObject(m_buffer);
    // DrawLines walks every vertex for the bounding box unless this is off.
    memDC.DisableAutomaticBoundingBoxUpdates();

    memDC.SetBackground(wxBrush(GetBackgroundColour()));
    memDC.Clear();

    // Logical size matches the drawing coordinate system (not physical pixels).
    DrawPlot(memDC, m_buffer.GetLogicalSize());
    memDC.SelectObject(wxNullBitmap);

    m_lastRenderMs = sw.Time();
    m_renderedSize = m_drawingSize;
    m_resizeRenderPending = false;
    if ( m_resizeSettleTimer.IsRunning() )
        m_resizeSettleTimer.Stop();
}

bool wxPlotting::RenderToBitmap(wxBitmap& out,
                                const wxSize& size,
                                const wxColour& background)
{
    if ( size.x < 1 || size.y < 1 )
        return false;
    if ( size.x <= m_marginLeft + m_marginRight ||
         size.y <= m_marginTop + m_marginBottom )
        return false;

    // Exact pixel bitmap — no CreateScaled / content-scale factor. Hosts pass
    // the final dimensions (e.g. video frame size) so pixels stay sharp.
    wxImage img(size.x, size.y, false);
    if ( !img.IsOk() )
        return false;
    img.InitAlpha();

    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 0;
    // wxTransparentColour / invalid / alpha 0 → fully transparent clear.
    if ( background.IsOk() && background != wxTransparentColour &&
         background.Alpha() != 0 )
    {
        r = background.Red();
        g = background.Green();
        b = background.Blue();
        a = background.Alpha();
    }

    unsigned char* const rgb = img.GetData();
    unsigned char* const alpha = img.GetAlpha();
    const size_t n = static_cast<size_t>(size.x) * static_cast<size_t>(size.y);
    for ( size_t i = 0; i < n; ++i )
    {
        rgb[i * 3 + 0] = r;
        rgb[i * 3 + 1] = g;
        rgb[i * 3 + 2] = b;
        alpha[i] = a;
    }

    out = wxBitmap(img);
    if ( !out.IsOk() )
        return false;

    // Temporarily drive geometry for DrawPlot / DrawSeries (same as RecreateBuffer).
    const wxSize savedDrawing = m_drawingSize;
    const wxSize savedPlotSize = m_plotSize;
    const wxRect savedPlot = m_plot;

    m_drawingSize = size;
    m_plotSize = wxSize(size.x - m_marginLeft - m_marginRight,
                        size.y - m_marginTop - m_marginBottom);
    m_plot = wxRect(wxPoint(m_marginLeft, m_marginTop), m_plotSize);

    {
        wxMemoryDC memDC;
        memDC.SelectObject(out);
        memDC.DisableAutomaticBoundingBoxUpdates();
        // Background already filled with alpha; DrawPlot does not clear.
        DrawPlot(memDC, size);
        memDC.SelectObject(wxNullBitmap);
    }

    m_drawingSize = savedDrawing;
    m_plotSize = savedPlotSize;
    m_plot = savedPlot;

    return out.IsOk();
}

// ----------------------------------------------------------------------------
// drawing (override / extend later)
// ----------------------------------------------------------------------------

void wxPlotting::DrawPlot(wxMemoryDC& dc, const wxSize& size)
{
    // Caller is responsible for clearing (Render / RenderToBitmap).

    wxPen penBorder(wxColor("White"));
    dc.SetPen(penBorder);
    
    dc.DrawLine(m_plot.GetBottomLeft(), m_plot.GetBottomRight());
    dc.DrawLine(m_plot.GetBottomRight(), m_plot.GetTopRight());
    dc.DrawLine(m_plot.GetTopRight(), m_plot.GetTopLeft());
    dc.DrawLine(m_plot.GetTopLeft(), m_plot.GetBottomLeft());


    for (auto iter = m_axisList.begin();
        iter != m_axisList.end();
        ++iter)
    {
        if (!iter->Visible) continue;

        if (iter->GetAxisType() == Axis::Horizontal)
        {
            wxPoint startPt, endPt;

            startPt.x = (iter->GetStart() / 1.0) * m_plot.GetWidth();
            startPt.y = m_plot.GetHeight() - (iter->GetLocation() * m_plot.GetHeight());
            endPt.x = (iter->GetEnd() / 1.0) * m_plot.GetWidth();
            endPt.y = m_plot.GetHeight() - (iter->GetLocation() * m_plot.GetHeight());

            startPt.y += m_plot.y;
            startPt.x += m_plot.x;
            endPt.y += m_plot.y;
            endPt.x += m_plot.x;

            wxPen pen;
            pen.SetWidth(iter->Width);
            pen.SetColour(iter->Foreground);
            dc.SetPen(pen);

            dc.DrawLine(startPt, endPt);

            if (iter->LabelVisible && !iter->Label.IsEmpty())
            {
                // Center the label on the horizontal axis, on the requested side.
                // DrawText origin is the top-left of the text box.
                dc.SetFont(iter->LabelFont);
                dc.SetTextForeground(iter->LabelColor);

                const wxSize textSize = dc.GetTextExtent(iter->Label);
                const int midX = (startPt.x + endPt.x) / 2;
                const int clear = AxisTitleClearance(dc, *iter, true);
                const int labelX = midX - textSize.x / 2;
                int labelY;
                if (iter->LabelSide == Axis::AboveOrRight)
                {
                    // Above the axis: bottom of text is `clear` above the line.
                    labelY = startPt.y - clear - textSize.y;
                }
                else
                {
                    // Below the axis: top of text is `clear` below the line.
                    labelY = startPt.y + clear;
                }

                dc.DrawText(iter->Label, labelX, labelY);
            }
        }
        else if (iter->GetAxisType() == Axis::Vertical)
        {
            wxPoint startPt, endPt;

            startPt.x = iter->GetLocation() * m_plot.GetWidth();
            startPt.y = m_plot.GetHeight() - (iter->GetStart() / 1.0) * m_plot.GetHeight();
            endPt.x = iter->GetLocation() * m_plot.GetWidth();
            endPt.y = m_plot.GetHeight() - (iter->GetEnd() / 1.0) * m_plot.GetHeight();

            startPt.y += m_plot.y;
            startPt.x += m_plot.x;
            endPt.y += m_plot.y;
            endPt.x += m_plot.x;

            wxPen pen;
            pen.SetWidth(iter->Width);
            pen.SetColour(iter->Foreground);
            dc.SetPen(pen);

            dc.DrawLine(startPt, endPt);

            if (iter->LabelVisible && !iter->Label.IsEmpty())
            {
                // Center the label along the vertical axis on LabelSide.
                // LabelRotated (default true): 90° CCW, reads bottom-to-top.
                // LabelRotated false: upright text for long titles.
                dc.SetFont(iter->LabelFont);
                dc.SetTextForeground(iter->LabelColor);

                const wxSize textSize = dc.GetTextExtent(iter->Label);
                const int midY = (startPt.y + endPt.y) / 2;
                const int clear = AxisTitleClearance(dc, *iter, false);

                if ( iter->LabelRotated )
                {
                    // On MSW, DrawRotatedText at 90° occupies:
                    //   x in [ox, ox + h],  y in [oy - w, oy]
                    // where (w, h) is the unrotated text extent.
                    const int w = textSize.x; // length along the axis after rotation
                    const int h = textSize.y; // thickness perpendicular to the axis

                    const int labelY = midY + w / 2;
                    int labelX;
                    if (iter->LabelSide == Axis::AboveOrRight)
                        labelX = startPt.x + clear;
                    else
                        labelX = startPt.x - clear - h;

                    dc.DrawRotatedText(iter->Label, labelX, labelY, 90.0);
                }
                else
                {
                    // Upright: center vertically on the axis span.
                    const int labelY = midY - textSize.y / 2;
                    int labelX;
                    if (iter->LabelSide == Axis::AboveOrRight)
                        labelX = startPt.x + clear;
                    else
                        labelX = startPt.x - clear - textSize.x;

                    dc.DrawText(iter->Label, labelX, labelY);
                }
            }
        }

        if (iter->TickVisible || iter->TickLabelVisible || iter->GridVisible)
        {
            double rangeStart = iter->GetRangeStart();
            double rangeEnd = iter->GetRangeEnd();

            double rangeDist = rangeEnd - rangeStart;
            if (iter->TickRange >= 0.0 && iter->TickRange <= rangeDist)
            {
                wxPen penGrid(iter->GridColor, iter->GridWidth, iter->GridStyle);
                wxPen penTick(iter->TickColor);

                // First tick at or above rangeStart on the TickRange grid.
                double t_start = ceil(rangeStart / iter->TickRange) * iter->TickRange;
                // Guard floating noise so the start value itself can appear.
                if ( t_start > rangeStart && (t_start - rangeStart) < iter->TickRange * 1e-9 )
                    t_start = rangeStart;
                if ( t_start < rangeStart )
                    t_start += iter->TickRange;

                while (t_start <= rangeEnd + iter->TickRange * 1e-9)
                {
                    if ( t_start > rangeEnd )
                        break;

                    wxPoint tickPoint;
                    // Tick values are already in range/display units — do not
                    // apply Axis::Scale (that is only for raw series samples).
                    double v = iter->ConvertRangeValue(t_start);
                    if (iter->GetAxisType() == Axis::Horizontal)
                    {
                        tickPoint.x = (v / 1.0) * m_plot.GetWidth();
                        tickPoint.y = m_plot.GetHeight() - (iter->GetLocation() * m_plot.GetHeight());
                    }
                    else if (iter->GetAxisType() == Axis::Vertical)
                    {
                        tickPoint.x = (iter->GetLocation() / 1.0) * m_plot.GetWidth();
                        tickPoint.y = m_plot.GetHeight() - (v * m_plot.GetHeight());
                    }

                    tickPoint.y += m_plot.y;
                    tickPoint.x += m_plot.x;

                    wxPoint gridStart = tickPoint;
                    wxPoint gridEnd = tickPoint;

                    if (iter->GridVisible)
                    {
                        if (iter->GetAxisType() == Axis::Horizontal)
                        {
                            gridEnd.y = m_plot.y;
                        }
                        else if (iter->GetAxisType() == Axis::Vertical)
                        {
                            gridEnd.x = m_plot.x + m_plot.width;
                        }

                        dc.SetPen(penGrid);
                        dc.DrawLine(gridStart, gridEnd);

                    }

                    const int tickLen = iter->TickVisible ? kTickLength : 0;

                    if (iter->TickVisible)
                    {
                        wxPoint tickStart = tickPoint;
                        wxPoint tickEnd = tickPoint;

                        if (iter->GetAxisType() == Axis::Horizontal)
                        {
                            if (iter->TickSide == Axis::BelowOrLeft)
                                tickEnd.y = tickPoint.y + kTickLength;
                            else
                                tickEnd.y = tickPoint.y - kTickLength;
                        }
                        else if (iter->GetAxisType() == Axis::Vertical)
                        {
                            if (iter->TickSide == Axis::BelowOrLeft)
                                tickEnd.x = tickPoint.x - kTickLength;
                            else
                                tickEnd.x = tickPoint.x + kTickLength;
                        }

                        dc.SetPen(penTick);
                        dc.DrawLine(tickStart, tickEnd);
                    }

                    if (iter->TickLabelVisible)
                    {
                        // Always upright — never rotate tick values.
                        dc.SetFont(iter->TickFont);
                        dc.SetTextForeground(iter->TickColor);

                        const wxString text = iter->FormatTickValue(t_start);
                        const wxSize ts = dc.GetTextExtent(text);

                        int tx = 0;
                        int ty = 0;
                        if (iter->GetAxisType() == Axis::Horizontal)
                        {
                            tx = tickPoint.x - ts.x / 2;
                            if (iter->TickSide == Axis::BelowOrLeft)
                                ty = tickPoint.y + tickLen + kLabelPad;
                            else
                                ty = tickPoint.y - tickLen - kLabelPad - ts.y;
                        }
                        else if (iter->GetAxisType() == Axis::Vertical)
                        {
                            ty = tickPoint.y - ts.y / 2;
                            if (iter->TickSide == Axis::BelowOrLeft)
                                tx = tickPoint.x - tickLen - kLabelPad - ts.x;
                            else
                                tx = tickPoint.x + tickLen + kLabelPad;
                        }

                        dc.DrawText(text, tx, ty);
                    }

                    t_start += iter->TickRange;
                }
            }
        }
    }

    DrawSeries(dc, size);
}

void wxPlotting::DrawSeries(wxMemoryDC& dc, const wxSize& size)
{
    for (auto iter = m_seriesList.begin();
        iter != m_seriesList.end();
        ++iter)
    {
        if (!iter->Visible) continue;

        std::list<Axis>::iterator horzAxis;
        std::list<Axis>::iterator vertAxis;
        bool foundVertical = false;
        bool foundHorizontal = false;

        // find horizonal axis
        for (auto axisiter = m_axisList.begin();
            axisiter != m_axisList.end();
            ++axisiter)
        {
            if (!iter->HorizontalAxis.CompareTo(axisiter->GetName()))
            {
                horzAxis = axisiter;
                foundHorizontal = true;
                break;
            }
        }

        // find vertical axis
        for (auto axisiter = m_axisList.begin();
            axisiter != m_axisList.end();
            ++axisiter)
        {
            if (!iter->VerticalAxis.CompareTo(axisiter->GetName()))
            {
                vertAxis = axisiter;
                foundVertical = true;
                break;
            }
        }

        if (!foundVertical || !foundHorizontal)
            continue;

        if (iter->GetSeriesType() == Series::Annotation)
        {
            DrawAnnotationSeries(dc, *iter, *horzAxis, *vertAxis);
            continue;
        }

        // Renderer lives only for this series draw pass. It receives
        // axis-converted coordinates and maps them to pixels via m_plot.
        // Custom style uses Series::CustomRendererFactory (may return null).
        std::unique_ptr<SeriesRenderer> renderer =
            CreateSeriesRenderer(*iter);
        if ( !renderer )
            continue;

        renderer->Begin(dc, *iter, m_plot);
        // Fill style closes to the lower edge of the vertical axis band.
        {
            const double lo = wxMin(vertAxis->GetStart(), vertAxis->GetEnd());
            renderer->SetBaselinePlotY(lo);
        }

        // Line, stair, and fill record screen geometry in AddPoint and finish
        // the stroke in End(), after the lock is released. A range gap strokes
        // the run so far during the walk (one polyline). Custom and point
        // markers can draw inside AddPoint, so those still copy under the lock
        // and draw after it is released.
        auto feed = [&](const Series::Points& points)
        {
            for ( auto ptIter = points.begin();
                  ptIter != points.end();
                  ++ptIter )
            {
                // Clip in scaled (range) units so Axis::Scale is respected.
                if ( !horzAxis->IsValueInRange(ptIter->x) ||
                     !vertAxis->IsValueInRange(ptIter->y) )
                {
                    renderer->Break();
                    continue;
                }

                const wxRealPoint converted(
                    horzAxis->ConvertValue(ptIter->x),
                    vertAxis->ConvertValue(ptIter->y));

                renderer->AddPoint(converted);
            }
        };

        if ( iter->RenderStyle == SeriesRenderStyle::Custom ||
             iter->RenderStyle == SeriesRenderStyle::Points )
        {
            const Series::Points points = iter->GetPoints();
            feed(points);
        }
        else
        {
            iter->WithPoints(feed);
        }

        renderer->End();
        // unique_ptr destroys the renderer here.
    }

}

void wxPlotting::DrawAnnotationSeries(wxDC& dc, const Series& series,
                                      const Axis& horz, const Axis& vert)
{
    const Series::Annotations spans = series.GetAnnotations();
    if ( spans.empty() )
        return;

    int rowCount = 1;
    for ( const auto& span : spans )
    {
        if ( span.row + 1 > rowCount )
            rowCount = span.row + 1;
    }

    const double bandLo = wxMin(vert.GetStart(), vert.GetEnd());
    const double bandHi = wxMax(vert.GetStart(), vert.GetEnd());
    const double rowH = (bandHi - bandLo) / static_cast<double>(rowCount);
    // A hair of inset keeps neighbouring rows from touching. The glyph
    // size below is taken from whatever height is left.
    double pad = rowH * 0.04;
    if ( pad * 2.0 >= rowH )
        pad = 0.0;

    const double xMin = wxMin(horz.GetRangeStart(), horz.GetRangeEnd());
    const double xMax = wxMax(horz.GetRangeStart(), horz.GetRangeEnd());

    dc.SetBackgroundMode(wxTRANSPARENT);

    const wxColour fill = series.Foreground;
    const int lum = (fill.Red() * 30 + fill.Green() * 59 + fill.Blue() * 11) / 100;
    const wxColour textColour = (lum > 160) ? *wxBLACK : *wxWHITE;
    dc.SetTextForeground(textColour);
    dc.SetPen(wxPen(fill));
    dc.SetBrush(wxBrush(fill));

    for ( const auto& span : spans )
    {
        double x0 = span.x0;
        double x1 = span.x1;
        if ( x0 > x1 )
        {
            const double tmp = x0;
            x0 = x1;
            x1 = tmp;
        }
        if ( x1 < xMin || x0 > xMax )
            continue;
        if ( x0 < xMin )
            x0 = xMin;
        if ( x1 > xMax )
            x1 = xMax;

        int row = span.row;
        if ( row < 0 )
            row = 0;
        if ( row >= rowCount )
            row = rowCount - 1;

        // Row 0 sits at the top of the vertical axis band.
        const double yTop = bandHi - static_cast<double>(row) * rowH - pad;
        const double yBot = bandHi - static_cast<double>(row + 1) * rowH + pad;
        if ( yBot >= yTop )
            continue;

        const wxPoint p0 = PlotToScreen(m_plot, wxRealPoint(horz.ConvertValue(x0), yTop));
        const wxPoint p1 = PlotToScreen(m_plot, wxRealPoint(horz.ConvertValue(x1), yBot));

        int left = wxMin(p0.x, p1.x);
        int right = wxMax(p0.x, p1.x);
        const int top = wxMin(p0.y, p1.y);
        const int bottom = wxMax(p0.y, p1.y);
        if ( right <= left )
            right = left + 1;

        // A one-pixel gap keeps adjacent spans from fusing into one bar.
        if ( right - left > 4 )
        {
            left += 1;
            right -= 1;
        }

        const wxRect bar(wxPoint(left, top), wxPoint(right, bottom));
        dc.DrawRectangle(bar);

        if ( span.text.empty() )
            continue;

        // Fill the row vertically. The old fixed 9pt face left a short
        // glyph in the middle of a much taller bar. Point size is derived
        // from the bar, then corrected once if the DC's PPI undershoots.
        int targetPx = bar.GetHeight() - 2;
        if ( targetPx < 1 )
            targetPx = 1;
        if ( targetPx > 64 )
            targetPx = 64;
        const wxSize ppi = dc.GetPPI();
        const double ppiY = (ppi.y > 0) ? static_cast<double>(ppi.y) : 96.0;
        double points = static_cast<double>(targetPx) * 72.0 / ppiY;
        dc.SetFont(wxFontInfo(points).Family(wxFONTFAMILY_MODERN));
        const int probe = dc.GetTextExtent("Hg").y;
        if ( probe > 0 && probe < (targetPx * 3) / 4 )
        {
            points *= static_cast<double>(targetPx) / static_cast<double>(probe);
            dc.SetFont(wxFontInfo(points).Family(wxFONTFAMILY_MODERN));
        }

        const wxSize ts = dc.GetTextExtent(span.text);
        // A tall row uses a large face, so the full string is often wider
        // than the span. Clip to the bar and show the start of the label
        // instead of hiding it until the span can hold every glyph.
        // Center only when the whole string fits.
        int tx = bar.x + 2;
        if ( ts.x + 4 <= bar.GetWidth() )
            tx = bar.x + (bar.GetWidth() - ts.x) / 2;
        const int ty = bar.y + (bar.GetHeight() - ts.y) / 2;
        dc.SetClippingRegion(bar);
        dc.DrawText(span.text, tx, ty);
        dc.DestroyClippingRegion();
    }
}

// ----------------------------------------------------------------------------
// event handlers
// ----------------------------------------------------------------------------

void wxPlotting::PaintBuffer(wxDC& dc)
{
    const wxSize bufSize = m_buffer.GetLogicalSize();
    const bool stretch = m_resizeRenderPending &&
                         m_renderedSize.x > 0 &&
                         m_renderedSize.y > 0 &&
                         m_renderedSize != m_drawingSize &&
                         bufSize.x > 0 &&
                         bufSize.y > 0 &&
                         m_drawingSize.x > 0 &&
                         m_drawingSize.y > 0;

    if ( !stretch )
    {
        dc.DrawBitmap(m_buffer, 0, 0, false);
        return;
    }

    // Map the whole previous frame onto the new canvas. Source extent is the
    // bitmap's own coordinate space (physical pixels on MSW when the bitmap
    // is CreateScaled); dest extent is the new drawing size. The ratio is
    // the window size change, so the content-scale factor cancels out.
    wxMemoryDC srcDC;
    srcDC.SelectObject(m_buffer);
    const bool ok = dc.StretchBlit(0, 0, m_drawingSize.x, m_drawingSize.y,
                                   &srcDC, 0, 0, bufSize.x, bufSize.y);
    srcDC.SelectObject(wxNullBitmap);
    if ( !ok )
        dc.DrawBitmap(m_buffer, 0, 0, false);
}

void wxPlotting::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);
    // Map logical (canvas) coordinates through the current scroll offset.
    DoPrepareDC(dc);

    if ( !m_buffer.IsOk() )
    {
        RecreateBuffer();
        Render();
    }

    if ( m_buffer.IsOk() )
        PaintBuffer(dc);

    // Cursors and the measure drag are overlays — not baked into the buffer.
    DrawCursors(dc);
    DrawMeasure(dc);
}

void wxPlotting::OnSize(wxSizeEvent& event)
{
    // Grow/shrink the canvas with the client (down to the minimum only).
    if ( SyncDrawingSizeToClient() )
    {
        // Windows runs size-drag inside a modal loop and does not move the
        // frame until this handler and the matching paint return. Rebuilding
        // every series on each step is what makes a large capture feel stuck.
        // A cheap plot still redraws live (same as a small capture, and the
        // usual Linux case). An expensive one keeps the last frame, stretched,
        // until the drag pauses.
        const bool drawNow =
            m_lastRenderMs <= kLiveResizeBudgetMs || !m_buffer.IsOk();

        if ( drawNow )
        {
            Render();
        }
        else
        {
            UpdatePlotRect();
            m_resizeRenderPending = true;
            m_resizeSettleTimer.StartOnce(kResizeSettleMs);
        }
    }

    Refresh(false);
    event.Skip();
}

void wxPlotting::OnEraseBackground(wxEraseEvent& WXUNUSED(event))
{
    // Intentional no-op: painting is fully handled in OnPaint via the
    // off-screen MemoryDC buffer, which avoids flicker.
}

void wxPlotting::OnMotion(wxMouseEvent& event)
{
    if ( m_cursorMode != PlotCursorMode::Off )
    {
        int cx = event.GetX();
        int cy = event.GetY();
        CalcUnscrolledPosition(event.GetX(), event.GetY(), &cx, &cy);
        m_cursorCanvasPos = wxPoint(cx, cy);
        m_cursorTracking = true;
        // Overlay only — rate-limited; do not re-render the full plot buffer.
        RequestOverlayRefresh();
    }
    event.Skip();
}

void wxPlotting::OnLeaveWindow(wxMouseEvent& event)
{
    if ( m_cursorMode != PlotCursorMode::Off && m_cursorTracking )
    {
        m_cursorTracking = false;
        // Hide promptly (infrequent); bypass throttle so the overlay vanishes.
        m_overlayRefreshPending = true;
        if ( m_updateThrottleTimer.IsRunning() )
            m_updateThrottleTimer.Stop();
        FlushPendingUpdate(false);
    }
    event.Skip();
}

// ----------------------------------------------------------------------------
// measurement cursors and delta measure
// ----------------------------------------------------------------------------

namespace
{
    // Yellow value on a dark backdrop — shared by cursor "both" and measure.
    void DrawValueReadout(wxDC& dc, const wxString& text, int tx, int ty)
    {
        if ( text.IsEmpty() )
            return;

        const wxSize ts = dc.GetTextExtent(text);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(wxColour(0, 0, 0, 160)));
        dc.DrawRectangle(tx - 2, ty - 1, ts.x + 4, ts.y + 2);
        dc.SetTextForeground(wxColour(255, 255, 0));
        dc.SetBackgroundMode(wxTRANSPARENT);
        dc.DrawText(text, tx, ty);
    }

    // Filled double-headed arrow. Returns false when the segment is too short
    // to draw. Arrowheads are omitted on very short spans; the shaft still counts.
    bool DrawDoubleArrow(wxDC& dc, const wxPoint& a, const wxPoint& b)
    {
        const double dx = static_cast<double>(b.x - a.x);
        const double dy = static_cast<double>(b.y - a.y);
        const double len = std::sqrt(dx * dx + dy * dy);
        if ( len < 2.0 )
            return false;

        dc.DrawLine(a, b);

        const double ah = wxMin(8.0, len * 0.45);
        if ( ah < 3.0 )
            return true;

        const double ux = dx / len;
        const double uy = dy / len;
        const double aw = ah * 0.42;
        const double px = -uy;
        const double py = ux;

        const wxBrush saved = dc.GetBrush();
        dc.SetBrush(wxBrush(wxColour(255, 255, 0)));

        wxPoint head[3];
        head[0] = a;
        head[1] = wxPoint(wxRound(a.x + ux * ah + px * aw),
                          wxRound(a.y + uy * ah + py * aw));
        head[2] = wxPoint(wxRound(a.x + ux * ah - px * aw),
                          wxRound(a.y + uy * ah - py * aw));
        dc.DrawPolygon(3, head);

        head[0] = b;
        head[1] = wxPoint(wxRound(b.x - ux * ah + px * aw),
                          wxRound(b.y - uy * ah + py * aw));
        head[2] = wxPoint(wxRound(b.x - ux * ah - px * aw),
                          wxRound(b.y - uy * ah - py * aw));
        dc.DrawPolygon(3, head);

        dc.SetBrush(saved);
        return true;
    }

    // Distance label beside an arrow, centered on its midpoint.
    // horizontalArrow selects the axis of the shaft; outward is -1/+1 on the
    // perpendicular (screen Y for a horizontal shaft, screen X for a vertical one).
    void DrawMeasureDistance(wxDC& dc, const wxRect& plot, const wxString& text,
                             const wxPoint& mid, bool horizontalArrow, int outward)
    {
        if ( text.IsEmpty() )
            return;

        const wxSize ts = dc.GetTextExtent(text);
        int tx = 0;
        int ty = 0;
        if ( horizontalArrow )
        {
            tx = mid.x - ts.x / 2;
            ty = (outward < 0) ? (mid.y - ts.y - 6) : (mid.y + 6);
        }
        else
        {
            ty = mid.y - ts.y / 2;
            tx = (outward > 0) ? (mid.x + 8) : (mid.x - ts.x - 8);
        }

        if ( tx < plot.GetLeft() )
            tx = plot.GetLeft();
        if ( ty < plot.GetTop() )
            ty = plot.GetTop();
        if ( tx + ts.x > plot.GetRight() )
            tx = wxMax(plot.GetLeft(), plot.GetRight() - ts.x);
        if ( ty + ts.y > plot.GetBottom() )
            ty = wxMax(plot.GetTop(), plot.GetBottom() - ts.y);

        DrawValueReadout(dc, text, tx, ty);
    }
} // namespace

bool wxPlotting::ClientToAxisValue(const wxPoint& clientPos,
                                   const Axis& axis,
                                   double& value) const
{
    if ( m_plot.IsEmpty() || m_plot.GetWidth() <= 0 || m_plot.GetHeight() <= 0 )
        return false;

    int cx = clientPos.x;
    int cy = clientPos.y;
    CalcUnscrolledPosition(clientPos.x, clientPos.y, &cx, &cy);

    // Same clamp as the cursor guides so a drag past the plot edge sticks
    // to the rectangle the arrows are drawn in.
    if ( cx < m_plot.GetLeft() ) cx = m_plot.GetLeft();
    if ( cx > m_plot.GetRight() ) cx = m_plot.GetRight();
    if ( cy < m_plot.GetTop() ) cy = m_plot.GetTop();
    if ( cy > m_plot.GetBottom() ) cy = m_plot.GetBottom();

    if ( axis.GetAxisType() == Axis::Horizontal )
    {
        const double norm =
            (cx - m_plot.x) / static_cast<double>(m_plot.GetWidth());
        value = axis.InverseConvertValue(norm);
        return true;
    }

    if ( axis.GetAxisType() == Axis::Vertical )
    {
        const double norm =
            1.0 - (cy - m_plot.y) / static_cast<double>(m_plot.GetHeight());
        value = axis.InverseConvertValue(norm);
        return true;
    }

    return false;
}

void wxPlotting::SetMeasureOverlay(const wxString& horizontalAxis,
                                   const wxString& verticalAxis,
                                   double x0, double y0,
                                   double x1, double y1)
{
    m_measureVisible = true;
    m_measureHorzAxis = horizontalAxis;
    m_measureVertAxis = verticalAxis;
    m_measureX0 = x0;
    m_measureY0 = y0;
    m_measureX1 = x1;
    m_measureY1 = y1;
    RequestOverlayRefresh();
}

void wxPlotting::ClearMeasureOverlay()
{
    if ( !m_measureVisible && m_measureHorzAxis.IsEmpty() && m_measureVertAxis.IsEmpty() )
        return;

    m_measureVisible = false;
    m_measureHorzAxis.clear();
    m_measureVertAxis.clear();
    RequestOverlayRefresh();
}

void wxPlotting::SetCursorMode(PlotCursorMode mode)
{
    if ( m_cursorMode == mode )
        return;

    m_cursorMode = mode;
    if ( m_cursorMode == PlotCursorMode::Off )
        m_cursorTracking = false;

    // Mode changes are infrequent — paint immediately.
    m_overlayRefreshPending = true;
    if ( m_updateThrottleTimer.IsRunning() )
        m_updateThrottleTimer.Stop();
    FlushPendingUpdate(false);
}

const Axis* wxPlotting::AxisOfTypeFromHit(const PlotHitResult& hit,
                                          Axis::AxisType type) const
{
    for ( const wxString& name : hit.axes )
    {
        const Axis* axis = GetAxis(name);
        if ( axis && axis->GetAxisType() == type && axis->Visible )
            return axis;
    }
    return nullptr;
}

void wxPlotting::DrawCursors(wxDC& dc)
{
    if ( m_cursorMode == PlotCursorMode::Off || !m_cursorTracking )
        return;
    if ( m_plot.IsEmpty() || m_plot.GetWidth() <= 0 || m_plot.GetHeight() <= 0 )
        return;

    // Select axes from the same strip hit-test used for interaction.
    const PlotHitResult hit = HitTestCanvas(m_cursorCanvasPos, 0);
    const Axis* horzAxis = AxisOfTypeFromHit(hit, Axis::Horizontal);
    const Axis* vertAxis = AxisOfTypeFromHit(hit, Axis::Vertical);

    const bool wantH = (m_cursorMode == PlotCursorMode::Horizontal ||
                        m_cursorMode == PlotCursorMode::Combined);
    const bool wantV = (m_cursorMode == PlotCursorMode::Vertical ||
                        m_cursorMode == PlotCursorMode::Combined);

    // Clamp guide lines to the data rectangle.
    int x = m_cursorCanvasPos.x;
    int y = m_cursorCanvasPos.y;
    if ( x < m_plot.GetLeft() ) x = m_plot.GetLeft();
    if ( x > m_plot.GetRight() ) x = m_plot.GetRight();
    if ( y < m_plot.GetTop() ) y = m_plot.GetTop();
    if ( y > m_plot.GetBottom() ) y = m_plot.GetBottom();

    const double plotNormX =
        (x - m_plot.x) / static_cast<double>(m_plot.GetWidth());
    const double plotNormY =
        1.0 - (y - m_plot.y) / static_cast<double>(m_plot.GetHeight());

    wxPen cursorPen(wxColour(255, 255, 0), 1, wxPENSTYLE_DOT);
    dc.SetPen(cursorPen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetTextForeground(wxColour(255, 255, 0));
    dc.SetFont(GetFont());

    wxString labelH;
    wxString labelV;

    // Horizontal cursor mode → vertical tracking line + H-axis value.
    // Value uses the axis TickLabelFormat (same as tick labels).
    if ( wantH && horzAxis )
    {
        dc.DrawLine(x, m_plot.GetTop(), x, m_plot.GetBottom());
        const double dataX = horzAxis->InverseConvertValue(plotNormX);
        labelH = horzAxis->FormatTickValue(dataX);
    }

    // Vertical cursor mode → horizontal tracking line + V-axis value.
    if ( wantV && vertAxis )
    {
        dc.DrawLine(m_plot.GetLeft(), y, m_plot.GetRight(), y);
        const double dataY = vertAxis->InverseConvertValue(plotNormY);
        labelV = vertAxis->FormatTickValue(dataY);
    }

    // Compose readout near the pointer (offset so it is not under the cursor).
    wxString readout;
    if ( !labelH.IsEmpty() && !labelV.IsEmpty() )
        readout = labelH + wxT(", ") + labelV;
    else if ( !labelH.IsEmpty() )
        readout = labelH;
    else if ( !labelV.IsEmpty() )
        readout = labelV;

    if ( !readout.IsEmpty() )
    {
        const wxSize ts = dc.GetTextExtent(readout);
        int tx = x + 10;
        int ty = y - ts.y - 6;
        if ( tx + ts.x > m_plot.GetRight() )
            tx = x - ts.x - 10;
        if ( ty < m_plot.GetTop() )
            ty = y + 6;

        DrawValueReadout(dc, readout, tx, ty);
    }
}

void wxPlotting::DrawMeasure(wxDC& dc)
{
    if ( !m_measureVisible )
        return;
    if ( m_plot.IsEmpty() || m_plot.GetWidth() <= 0 || m_plot.GetHeight() <= 0 )
        return;

    const Axis* horz = GetAxis(m_measureHorzAxis);
    const Axis* vert = GetAxis(m_measureVertAxis);
    if ( !horz || !vert )
        return;
    if ( horz->GetAxisType() != Axis::Horizontal ||
         vert->GetAxisType() != Axis::Vertical )
        return;

    // Anchor p0, drag point p1. The shaft is an L: horizontal at the anchor's
    // Y, then vertical out to the drag point. Each leg's length is that delta.
    const wxPoint p0 = PlotToScreen(m_plot, wxRealPoint(
        horz->ConvertValue(m_measureX0),
        vert->ConvertValue(m_measureY0)));
    const wxPoint p1 = PlotToScreen(m_plot, wxRealPoint(
        horz->ConvertValue(m_measureX1),
        vert->ConvertValue(m_measureY1)));
    const wxPoint corner(p1.x, p0.y);

    wxDCClipper clip(dc, m_plot);

    dc.SetPen(wxPen(wxColour(255, 255, 0), 1, wxPENSTYLE_SOLID));
    dc.SetBrush(wxBrush(wxColour(255, 255, 0)));
    dc.SetFont(GetFont());
    dc.SetTextForeground(wxColour(255, 255, 0));

    const bool drewH = DrawDoubleArrow(dc, p0, corner);
    const bool drewV = DrawDoubleArrow(dc, corner, p1);

    // Press, before the pointer has moved: mark the anchor.
    if ( !drewH && !drewV )
    {
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawCircle(p0, 3);
        return;
    }

    if ( drewH )
    {
        const wxPoint mid((p0.x + corner.x) / 2, (p0.y + corner.y) / 2);
        // Keep the readout on the outside of the L.
        const int outwardY = (p1.y >= p0.y) ? -1 : 1;
        DrawMeasureDistance(dc, m_plot,
            horz->FormatTickValue(std::fabs(m_measureX1 - m_measureX0)),
            mid, true, outwardY);
    }

    if ( drewV )
    {
        const wxPoint mid((corner.x + p1.x) / 2, (corner.y + p1.y) / 2);
        const int outwardX = (p1.x >= p0.x) ? 1 : -1;
        DrawMeasureDistance(dc, m_plot,
            vert->FormatTickValue(std::fabs(m_measureY1 - m_measureY0)),
            mid, false, outwardX);
    }
}

