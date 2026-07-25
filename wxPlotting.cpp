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
    test1.Width = 1;
    test1.LabelColor = wxColor("Green");
    test1.SetPosition(0.0);
    test1.SetRange(0.0, 1.0);

    Axis test2("Signal1", Axis::Vertical);
    test2.Foreground = (wxColor("Magenta"));
    test2.Visible = true;
    test2.ShowLabel = true;
    test2.Width = 1;
    test2.LabelColor = wxColor("Magenta");
    test2.SetPosition(0.0);
    test2.SetRange(0.0, 0.20);

    Axis test3("Signal2", Axis::Vertical);
    test3.Foreground = (wxColor("Cyan"));
    test3.Visible = true;
    test3.ShowLabel = true;
    test3.Width = 1;
    test3.LabelColor = wxColor("Cyan");
    test3.SetPosition(0.0);
    test3.SetRange(0.25, 0.40);

    m_axisList.push_back(test1);
    m_axisList.push_back(test2);
    m_axisList.push_back(test3);

    Series series1("Series1", Series::Doubles);
    series1.HorizontalAxis = wxString("Time");
    series1.VerticalAxis = wxString("Signal1");

    for (auto i = 0; i < 100; i++)
    {
        series1.Data.push_back(wxRealPoint(i, i));
    }

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

            startPt.x = iter->GetRangeStart() * m_plot.GetWidth();
            startPt.y = m_plot.GetHeight() - (iter->GetPosition() * m_plot.GetHeight());
            endPt.x = iter->GetRangeEnd() * m_plot.GetWidth();
            endPt.y = m_plot.GetHeight() - (iter->GetPosition() * m_plot.GetHeight());

            startPt.y += (m_plot.GetHeight() - m_plot.GetBottomLeft().y);
            startPt.x += m_plot.GetBottomLeft().x;
            endPt.y += (m_plot.GetHeight() - m_plot.GetBottomLeft().y);
            endPt.x += m_plot.GetBottomLeft().x;

            wxPen pen;
            pen.SetWidth(iter->Width);
            pen.SetColour(iter->Foreground);
            dc.SetPen(pen);

            dc.DrawLine(startPt, endPt);

            if (iter->ShowLabel && !iter->Label.IsEmpty())
            {
                // Center the label under the horizontal axis.
                dc.SetFont(GetFont());
                dc.SetTextForeground(iter->LabelColor);

                const wxSize textSize = dc.GetTextExtent(iter->Label);
                const int midX = (startPt.x + endPt.x) / 2;
                const int labelGap = 4;
                const int labelX = midX - textSize.x / 2;
                const int labelY = startPt.y + (iter->Width + 1) / 2 + labelGap;

                dc.DrawText(iter->Label, labelX, labelY);
            }
        }
        else if (iter->GetAxisType() == Axis::Vertical)
        {
            wxPoint startPt, endPt;

            startPt.y = m_plot.GetHeight() - (iter->GetRangeStart() * m_plot.GetHeight());
            startPt.x = iter->GetPosition() * m_plot.GetWidth();
            endPt.y = m_plot.GetHeight() - (iter->GetRangeEnd() * m_plot.GetHeight());
            endPt.x = iter->GetPosition() * m_plot.GetWidth();

            startPt.y += (m_plot.GetHeight() - m_plot.GetBottomLeft().y);
            startPt.x += m_plot.GetBottomLeft().x;
            endPt.y += (m_plot.GetHeight() - m_plot.GetBottomLeft().y);
            endPt.x += m_plot.GetBottomLeft().x;


            wxPen pen;
            pen.SetWidth(iter->Width);
            pen.SetColour(iter->Foreground);
            dc.SetPen(pen);

            dc.DrawLine(startPt, endPt);

            if (iter->ShowLabel && !iter->Label.IsEmpty())
            {
                // Center the label along the vertical axis and rotate 90° CCW
                // so the text reads bottom-to-top, just left of the axis line.
                dc.SetFont(GetFont());
                dc.SetTextForeground(iter->LabelColor);

                const wxSize textSize = dc.GetTextExtent(iter->Label);
                const int midY = (startPt.y + endPt.y) / 2;
                const int labelGap = 4;
                // DrawRotatedText origin is the start of the baseline; at 90°
                // the string extends upward, so start below the midpoint.
                const int labelX = startPt.x - (iter->Width + 1) / 2 - labelGap;
                const int labelY = midY + textSize.x / 2;

                dc.DrawRotatedText(iter->Label, labelX, labelY, 90.0);
            }
        }
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
