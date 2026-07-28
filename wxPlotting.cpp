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

#include "wx/dcclient.h"
#include "wx/dcmemory.h"
#include "wx/settings.h"
#include "wx/timer.h"

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


void wxPlotting::ArrangeVerticalAxes(double spacing)
{
    // Top → bottom: first registered visible vertical axis is highest on screen.
    ArrangeAxesOfType(Axis::Vertical, spacing, true);
}

void wxPlotting::ArrangeHorizontalAxes(double spacing)
{
    // Left → right: first registered visible horizontal axis is leftmost.
    ArrangeAxesOfType(Axis::Horizontal, spacing, false);
}

void wxPlotting::ArrangeAxesOfType(Axis::AxisType type, double spacing, bool topToBottom)
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

    if ( n == 1 )
    {
        axes[0]->SetPosition(0.0, 1.0);
        return;
    }

    // Equal bands with fixed gaps; shrink gaps if they would leave no room.
    const double minBand = 0.02;
    double gap = spacing;
    double totalGap = gap * static_cast<double>(n - 1);
    double usable = 1.0 - totalGap;
    if ( usable < minBand * static_cast<double>(n) )
    {
        usable = minBand * static_cast<double>(n);
        if ( usable > 1.0 )
            usable = 1.0;
        totalGap = 1.0 - usable;
        gap = totalGap / static_cast<double>(n - 1);
    }

    const double band = usable / static_cast<double>(n);

    for ( size_t i = 0; i < n; ++i )
    {
        double lo = 0.0;
        double hi = 1.0;

        if ( type == Axis::Vertical && topToBottom )
        {
            // i = 0 at top (high normalized Y), then downward.
            hi = 1.0 - static_cast<double>(i) * (band + gap);
            lo = hi - band;
        }
        else
        {
            // Horizontal (or bottom-to-top if ever requested): low → high.
            lo = static_cast<double>(i) * (band + gap);
            hi = lo + band;
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

void wxPlotting::OnUpdateThrottleTimer(wxTimerEvent& WXUNUSED(event))
{
    if ( m_fullUpdatePending || m_overlayRefreshPending )
        FlushPendingUpdate(false);
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

void wxPlotting::RecreateBuffer()
{
    const wxSize size = m_drawingSize;
    if ( size.x <= m_marginLeft + m_marginRight ||
         size.y <= m_marginTop + m_marginBottom )
        return;

    // Plot rectangle always tracks margins, even when the bitmap size is unchanged.
    m_plotSize = wxSize(size.x - m_marginLeft - m_marginRight,
                        size.y - m_marginTop - m_marginBottom);
    m_plot = wxRect(wxPoint(m_marginLeft, m_marginTop), m_plotSize);

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
    RecreateBuffer();
    if ( !m_buffer.IsOk())
        return;

    wxMemoryDC memDC;
    memDC.SelectObject(m_buffer);

    memDC.SetBackground(wxBrush(GetBackgroundColour()));
    memDC.Clear();

    // Logical size matches the drawing coordinate system (not physical pixels).
    DrawPlot(memDC, m_buffer.GetLogicalSize());
    memDC.SelectObject(wxNullBitmap);
}

// ----------------------------------------------------------------------------
// drawing (override / extend later)
// ----------------------------------------------------------------------------

void wxPlotting::DrawPlot(wxMemoryDC& dc, const wxSize& size)
{
    // Placeholder: clear to background and mark the control as a template.
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();

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

        // Snapshot under the series mutex so a worker can append/replace
        // points while we draw without racing or holding the lock during GDI.
        const auto points = iter->GetPoints();

        for (auto ptIter = points.begin();
            ptIter != points.end();
            ++ptIter
            )
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

        renderer->End();
        // unique_ptr destroys the renderer here.
    }

}

// ----------------------------------------------------------------------------
// event handlers
// ----------------------------------------------------------------------------

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
        dc.DrawBitmap(m_buffer, 0, 0, false);

    // Cursors are an overlay — not baked into the buffer (cheap mouse tracking).
    DrawCursors(dc);
}

void wxPlotting::OnSize(wxSizeEvent& event)
{
    // Grow/shrink the canvas with the client (down to the minimum only).
    if ( SyncDrawingSizeToClient() )
    {
        RecreateBuffer();
        Render();
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
// measurement cursors
// ----------------------------------------------------------------------------

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

        // Slight backdrop for readability on dense plots.
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(wxColour(0, 0, 0, 160)));
        dc.DrawRectangle(tx - 2, ty - 1, ts.x + 4, ts.y + 2);
        dc.SetTextForeground(wxColour(255, 255, 0));
        dc.DrawText(readout, tx, ty);
    }
}

