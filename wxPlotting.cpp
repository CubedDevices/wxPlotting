/////////////////////////////////////////////////////////////////////////////
// Name:        wxPlotting.cpp
// Purpose:     Owner-drawn plotting control (MemoryDC double-buffer template)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wxPlotting.h"
#include "seriesrenderer.h"
#include "plotcoords.h"

#include "wx/dcclient.h"
#include "wx/dcmemory.h"
#include "wx/settings.h"

#include <cmath>
#include <limits>
#include <vector>

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// Minimum canvas floor.  The effective canvas grows with the client area
// above this; scrollbars appear only when the client is smaller.
// Raise the minimum later (SetMinDrawingHeight) as series data is added.
const wxSize wxPlotting::DefaultMinDrawingSize(400, 200);

// ----------------------------------------------------------------------------
// event table / RTTI
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(wxPlotting, wxScrolledWindow);

wxBEGIN_EVENT_TABLE(wxPlotting, wxScrolledWindow)
    EVT_PAINT(wxPlotting::OnPaint)
    EVT_SIZE(wxPlotting::OnSize)
    EVT_ERASE_BACKGROUND(wxPlotting::OnEraseBackground)
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

    SyncDrawingSizeToClient();
    RecreateBuffer();
    Render();

    return true;
}

wxPlotting::~wxPlotting()
{
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


void wxPlotting::UpdatePlot(bool forceImmediate)
{
    // Rebuild the off-screen buffer, then ask the window to repaint.
    // Avoid Update() by default so high-rate CallAfter(UpdatePlot) does not
    // force a synchronous paint storm on the UI thread.
    Render();
    Refresh(false);
    if ( forceImmediate )
        Update();
}

// ----------------------------------------------------------------------------
// hit-testing
// ----------------------------------------------------------------------------

namespace
{
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

    // True if canvasPos lies in the axis's valid data region (not merely near
    // the drawn axis line). Horizontal axes cover their X span across the
    // full plot height; vertical axes cover their Y span across the full width.
    bool AxisRegionContains(const Axis& axis, const wxRect& plot, const wxPoint& p)
    {
        if ( plot.IsEmpty() || plot.GetWidth() <= 0 || plot.GetHeight() <= 0 )
            return false;
        if ( !plot.Contains(p) )
            return false;

        const double nx = (p.x - plot.x) / static_cast<double>(plot.GetWidth());
        const double ny = 1.0
            - (p.y - plot.y) / static_cast<double>(plot.GetHeight());

        double lo = axis.GetStart();
        double hi = axis.GetEnd();
        if ( lo > hi )
        {
            const double t = lo;
            lo = hi;
            hi = t;
        }

        if ( axis.GetAxisType() == Axis::Horizontal )
            return nx >= lo && nx <= hi;

        if ( axis.GetAxisType() == Axis::Vertical )
            return ny >= lo && ny <= hi;

        return false;
    }

    void CollectAxesAtPoint(const std::list<Axis>& axes,
                            const wxRect& plot,
                            const wxPoint& canvasPos,
                            std::vector<wxString>& outNames)
    {
        outNames.clear();
        for ( const auto& axis : axes )
        {
            if ( !axis.Visible )
                continue;
            if ( AxisRegionContains(axis, plot, canvasPos) )
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

            if ( points[i].x < horz.GetRangeStart() || points[i].x > horz.GetRangeEnd() ||
                 points[i].y < vert.GetRangeStart() || points[i].y > vert.GetRangeEnd() )
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

        // Single-point series: hit disk around the sample.
        if ( mapped.size() == 1 && mapped[0].valid )
        {
            const double dx = canvasPos.x - mapped[0].screen.x;
            const double dy = canvasPos.y - mapped[0].screen.y;
            const double d = std::sqrt(dx * dx + dy * dy);
            if ( d <= static_cast<double>(tolerancePx) )
            {
                best.hit = true;
                best.distance = d;
                best.sampleIndex = mapped[0].index;
                best.dataPoint = mapped[0].data;
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

    // Axes: region containment (full valid-data area), collect all matches.
    // A typical pick over data yields one horizontal and one vertical axis.
    CollectAxesAtPoint(m_axisList, m_plot, canvasPos, result.axes);

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

namespace
{
    // Length of a major tick mark in pixels.
    const int kTickLength = 10;
    const int kLabelPad = 4;

    wxString FormatTickValue(const Axis& axis, double value)
    {
        const wxString& fmt = axis.TickLabelFormat.IsEmpty()
            ? wxString("%g")
            : axis.TickLabelFormat;
        return wxString::Format(fmt, value);
    }

    // Extra clearance beyond the axis pen for the axis title, so it sits
    // outside ticks and/or tick labels when those share LabelSide.
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
                // Tick values are upright under/over the axis; reserve font height.
                clear += dc.GetCharHeight() + kLabelPad;
            }
            else
            {
                // Tick values are upright left/right of the axis; reserve widest value.
                const wxString s0 = FormatTickValue(axis, axis.GetRangeStart());
                const wxString s1 = FormatTickValue(axis, axis.GetRangeEnd());
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
} // namespace

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
                    double v = iter->ConvertValue(t_start);
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

                        const wxString text = FormatTickValue(*iter, t_start);
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
        std::unique_ptr<SeriesRenderer> renderer =
            CreateSeriesRenderer(iter->RenderStyle);
        renderer->Begin(dc, *iter, m_plot);

        // Snapshot under the series mutex so a worker can append/replace
        // points while we draw without racing or holding the lock during GDI.
        const auto points = iter->GetPoints();

        for (auto ptIter = points.begin();
            ptIter != points.end();
            ++ptIter
            )
        {
            if (ptIter->x < horzAxis->GetRangeStart() || (ptIter->x > horzAxis->GetRangeEnd()))
            {
                renderer->Break();
                continue;
            }
            if (ptIter->y < vertAxis->GetRangeStart() || (ptIter->y > vertAxis->GetRangeEnd()))
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
