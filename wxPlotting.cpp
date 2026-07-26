/////////////////////////////////////////////////////////////////////////////
// Name:        wxPlotting.cpp
// Purpose:     Owner-drawn plotting control (MemoryDC double-buffer template)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wxPlotting.h"

#include "wx/dcclient.h"
#include "wx/dcmemory.h"
#include "wx/settings.h"

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

void wxPlotting::CreateDefaultPlot()
{
    Axis test1("Time", Axis::Horizontal);
    test1.Foreground = (wxColor("Green"));
    test1.Visible = true;
    test1.LabelVisible = true;
    test1.Width = 3;
    test1.LabelColor = wxColor("Green");
    test1.LabelFont = wxFont(wxFontInfo(14).Family(wxFONTFAMILY_MODERN).Bold());
    test1.LabelSide = Axis::BelowOrLeft;
    test1.SetLocation(0.0);
    test1.SetPosition(0.0, 1.0);
    test1.SetRange(0, 100.0); // 0 to 100 seconds
    test1.TickRange = 10.0; // tick every 10 seconds
    test1.TickColor = wxColor("Green");
    test1.TickFont = wxFont(wxFontInfo(12).Family(wxFONTFAMILY_MODERN));
    test1.TickVisible = true;
    test1.TickLabelVisible = true;
    test1.TickLabelFormat = "%.0f"; // integer seconds
    test1.TickSide = Axis::BelowOrLeft;
    test1.GridVisible = true;
    test1.GridColor = wxColor("Grey");
    test1.GridStyle = wxPENSTYLE_SOLID;
    test1.GridWidth = 1;

    Axis test2("Signal1", Axis::Vertical);
    test2.Foreground = (wxColor("Magenta"));
    test2.Visible = true;
    test2.LabelVisible = true;
    test2.Width = 3;
    test2.LabelColor = wxColor("Magenta");
    test2.LabelFont = wxFont(wxFontInfo(14).Family(wxFONTFAMILY_MODERN).Bold());
    test2.LabelSide = Axis::BelowOrLeft;
    test2.SetLocation(0.0);
    test2.SetPosition(0.0, 0.47);
    test2.SetRange(-3.3, 3.3);   // += 3.3v
    test2.TickRange = 1.0;      // 1v ticks
    test2.TickColor = wxColor("Magenta");
    test2.TickFont = wxFont(wxFontInfo(12).Family(wxFONTFAMILY_MODERN));
    test2.TickVisible = true;
    test2.TickLabelVisible = true;
    test2.TickLabelFormat = "%.1f"; // one decimal place
    test2.TickSide = Axis::BelowOrLeft;
    test2.GridVisible = true;
    test2.GridColor = wxColor("Magenta").ChangeLightness(50);
    test2.GridStyle = wxPENSTYLE_DOT_DASH;
    test2.GridWidth = 1;

    Axis test3("Signal2", Axis::Vertical);
    test3.Foreground = (wxColor("Cyan"));
    test3.Visible = true;
    test3.LabelVisible = true;
    test3.Width = 3;
    test3.LabelColor = wxColor("Cyan");
    test3.LabelFont = wxFont(wxFontInfo(14).Family(wxFONTFAMILY_MODERN).Bold());
    test3.LabelSide = Axis::BelowOrLeft;
    test3.SetLocation(0.0);
    test3.SetPosition(0.53, 1.0);
    test3.SetRange(-3.3, 3.3);   // += 3.3v
    test3.TickRange = 1.0; // 1v ticks
    test3.TickColor = (wxColor("Cyan"));
    test3.TickFont = wxFont(wxFontInfo(12).Family(wxFONTFAMILY_MODERN));
    test3.TickVisible = true;
    test3.TickLabelVisible = true;
    test3.TickLabelFormat = "%.1f"; // one decimal place
    test3.TickSide = Axis::BelowOrLeft;
    test3.GridVisible = true;
    test3.GridColor = wxColor("Cyan").ChangeLightness(50);
    test3.GridStyle = wxPENSTYLE_DOT_DASH;
    test3.GridWidth = 1;

    m_axisList.push_back(test1);
    m_axisList.push_back(test2);
    m_axisList.push_back(test3);

    Series series1("Series1", Series::Doubles);
    series1.HorizontalAxis = wxString("Time");
    series1.VerticalAxis = wxString("Signal1");
    series1.Visible = true;

    for (double i = -3.3; i < 3.3; i += 0.1)
    {
        series1.Data.push_back(wxRealPoint(i, i));
    }

    m_seriesList.push_back(series1);

}


void wxPlotting::UpdatePlot()
{
    Render();
    Refresh(false);
    Update();
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
                // Center the label along the vertical axis, on the requested
                // side, rotated 90° CCW so text reads bottom-to-top.
                //
                // On MSW, DrawRotatedText uses GDI escapement; for angle 90°
                // the string occupies (see wxMSWDCImpl::DoDrawRotatedText):
                //   x in [ox, ox + h],  y in [oy - w, oy]
                // where (w, h) is the unrotated text extent.  The origin is
                // therefore the bottom-left of the upright bounding box of the
                // rotated glyphs — not the side nearer the axis.
                dc.SetFont(iter->LabelFont);
                dc.SetTextForeground(iter->LabelColor);

                const wxSize textSize = dc.GetTextExtent(iter->Label);
                const int midY = (startPt.y + endPt.y) / 2;
                const int clear = AxisTitleClearance(dc, *iter, false);
                const int w = textSize.x; // length along the axis after rotation
                const int h = textSize.y; // thickness perpendicular to the axis

                // Origin y so the vertical span [oy - w, oy] is centered on midY.
                const int labelY = midY + w / 2;

                int labelX;
                if (iter->LabelSide == Axis::AboveOrRight)
                {
                    // Right of the axis: left edge of text (ox) at axisX + clear.
                    labelX = startPt.x + clear;
                }
                else
                {
                    // Left of the axis: right edge of text (ox + h) at axisX - clear.
                    labelX = startPt.x - clear - h;
                }

                dc.DrawRotatedText(iter->Label, labelX, labelY, 90.0);
            }
        }

        if (iter->TickVisible || iter->TickLabelVisible || iter->GridVisible)
        {
            double rangeStart = iter->GetRangeStart();
            double rangeEnd = iter->GetRangeEnd();

            double rangeDist = rangeEnd - rangeStart;
            if (iter->TickRange > 0.0 && iter->TickRange < rangeDist)
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



    }

    /*
    dc.SetPen(wxPen(GetForegroundColour()));
    dc.SetTextForeground(GetForegroundColour());
    dc.SetFont(GetFont());

    const wxString label = "wxPlotting (MemoryDC template)";
    const wxSize textSize = dc.GetTextExtent(label);
    const int x = (size.x - textSize.x) / 2;
    const int y = (size.y - textSize.y) / 2;
    dc.DrawText(label, x > 0 ? x : 0, y > 0 ? y : 0);

    // Simple border so the control bounds are obvious during testing.
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(0, 0, size.x, size.y);
    */
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
