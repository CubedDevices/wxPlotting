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
    test1.ShowLabel = true;
    test1.Width = 2;
    test1.LabelColor = wxColor("Green");
    test1.LabelFont = wxFont(wxFontInfo(14).Family(wxFONTFAMILY_MODERN).Bold());
    test1.LabelSide = Axis::BelowOrLeft;
    test1.SetLocation(0.0);
    test1.SetPosition(0.0, 1.0);
    test1.SetRange(0, 100.0); // 0 to 100 seconds

    Axis test2("Signal1", Axis::Vertical);
    test2.Foreground = (wxColor("Magenta"));
    test2.Visible = true;
    test2.ShowLabel = true;
    test2.Width = 2;
    test2.LabelColor = wxColor("Magenta");
    test2.LabelSide = Axis::BelowOrLeft;
    test2.SetLocation(0.0);
    test2.SetPosition(0.0, 0.47);
    test2.SetRange(-3.3, 3.3);   // += 3.3v

    Axis test3("Signal2", Axis::Vertical);
    test3.Foreground = (wxColor("Cyan"));
    test3.Visible = true;
    test3.ShowLabel = true;
    test3.Width = 2;
    test3.LabelColor = wxColor("Cyan");
    test3.LabelSide = Axis::BelowOrLeft;
    test3.SetLocation(0.0);
    test3.SetPosition(0.53, 1.0);
    test3.SetRange(-3.3, 3.3);   // += 3.3v

    m_axisList.push_back(test1);
    m_axisList.push_back(test2);
    m_axisList.push_back(test3);

    Series series1("Series1", Series::Doubles);
    series1.HorizontalAxis = wxString("Time");
    series1.VerticalAxis = wxString("Signal1");
    series1.Visible = true;

    for (auto i = -3.3; i < 3.3; i += 0.1)
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
    if ( m_margin == value )
        return;

    m_margin = value;
    RecreateBuffer();
    Render();
    Refresh(false);
}

int wxPlotting::GetMargin() const
{
    return m_margin;
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
    if ( size.x <= 2 * m_margin || size.y <= 2 * m_margin )
        return;

    if ( !m_buffer.IsOk() || m_buffer.GetSize() != size )
    {
        m_plotSize = wxSize(size.x - (2 * m_margin),
                              size.y - (2 * m_margin));
        m_plot = wxRect(wxPoint(m_margin, m_margin), m_plotSize);

        m_buffer = wxBitmap(size);
    }
}

void wxPlotting::Render()
{
    RecreateBuffer();
    if ( !m_buffer.IsOk())
        return;

    wxMemoryDC memDC;

    memDC.SetBackground(wxBrush(GetBackgroundColour()));
    memDC.Clear();

    memDC.SelectObject(m_buffer);
    DrawPlot(memDC, m_buffer.GetSize());
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

            if (iter->ShowLabel && !iter->Label.IsEmpty())
            {
                // Center the label on the horizontal axis, on the requested side.
                // DrawText origin is the top-left of the text box.
                dc.SetFont(iter->LabelFont);
                dc.SetTextForeground(iter->LabelColor);

                const wxSize textSize = dc.GetTextExtent(iter->Label);
                const int midX = (startPt.x + endPt.x) / 2;
                // Clear half the pen plus padding so glyphs do not sit on the line.
                const int clear = (iter->Width + 1) / 2 + 4;
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

            if (iter->ShowLabel && !iter->Label.IsEmpty())
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
                const int clear = (iter->Width + 1) / 2 + 4;
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
