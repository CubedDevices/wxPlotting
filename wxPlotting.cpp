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
// event table / RTTI
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(wxPlotting, wxControl);

wxBEGIN_EVENT_TABLE(wxPlotting, wxControl)
    EVT_PAINT(wxPlotting::OnPaint)
    EVT_SIZE(wxPlotting::OnSize)
    EVT_ERASE_BACKGROUND(wxPlotting::OnEraseBackground)
wxEND_EVENT_TABLE()

// ----------------------------------------------------------------------------
// construction / destruction
// ----------------------------------------------------------------------------

wxPlotting::wxPlotting()
{
}

wxPlotting::wxPlotting(wxWindow* parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxString& name)
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
    // wxBORDER_NONE keeps the client area fully available for plotting.
    // Callers may still pass border styles via style.
    if ( !wxControl::Create(parent, id, pos, size,
                            style | wxFULL_REPAINT_ON_RESIZE,
                            wxDefaultValidator, name) )
    {
        return false;
    }

    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxBLACK);
    SetForegroundColour(*wxWHITE);

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
    Axis horz("hor", Axis::Horizontal);
    Axis vert("vert", Axis::Vertical);

    horz.Foreground = wxColor("Green");
    vert.Foreground = wxColor("Green");
    horz.Visible = false;
    vert.Visible = false;
    horz.Width = 5;
    vert.Width = 5;

    Axis test1("test1", Axis::Horizontal);
    test1.Foreground = (wxColor("Green"));
    test1.Visible = (true);
    test1.Width = (2);
    test1.SetPosition(0.05);
    test1.SetRange(0.05, 0.95);

    Axis test2("test2", Axis::Vertical);
    test2.Foreground = (wxColor("Magenta"));
    test2.Visible =(true);
    test2.Width = (2);
    test2.SetPosition(0.05);
    test2.SetRange(0.05, 0.20);

    Axis test3("test3", Axis::Vertical);
    test3.Foreground = (wxColor("Cyan"));
    test3.Visible = (true);
    test3.Width = (2);
    test3.SetPosition(0.05);
    test3.SetRange(0.25, 0.40);

    m_axisList.push_back(horz);
    m_axisList.push_back(vert);
    m_axisList.push_back(test1);
    m_axisList.push_back(test2);
    m_axisList.push_back(test3);

    Series series1("Series1", Series::Doubles);
    series1.HorizontalAxis = wxString("test1");
    series1.VerticalAxis = wxString("test2");

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
    m_margin = value;
}

int wxPlotting::GetMargin() const
{
    return m_margin;
}


// ----------------------------------------------------------------------------
// sizing / preferred size
// ----------------------------------------------------------------------------

wxSize wxPlotting::DoGetBestClientSize() const
{
    return wxSize(400, 300);
}

// ----------------------------------------------------------------------------
// buffer management
// ----------------------------------------------------------------------------

void wxPlotting::RecreateBuffer()
{
    const wxSize size = GetClientSize();
    if ( size.x <= 2 * m_margin || size.y <= 2 * m_margin)
        return;

    if ( !m_buffer.IsOk() || m_buffer.GetSize() != size )
    {
        auto plotSize = wxSize(size.x - (2*m_margin), size.y - (2*m_margin));
        m_plotBuffer = wxBitmap(plotSize);
        m_buffer = wxBitmap(size);
    }
}

void wxPlotting::Render()
{
    RecreateBuffer();
    if ( !m_buffer.IsOk() )
        return;

    wxMemoryDC memDC;
    memDC.SelectObject(m_plotBuffer);
    DrawPlot(memDC, m_plotBuffer.GetSize());


    memDC.SelectObject(m_buffer);
    memDC.SetBackground(wxBrush(GetBackgroundColour()));
    memDC.Clear();

    memDC.DrawBitmap(m_plotBuffer, m_margin, m_margin, false);

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
    dc.DrawLine(wxPoint(0, 0), wxPoint(size.GetWidth() - 1, 0));
    dc.DrawLine(wxPoint(size.GetWidth()-1, 0), wxPoint(size.GetWidth() - 1, size.GetHeight() - 1));
    dc.DrawLine(wxPoint(size.GetWidth() - 1, size.GetHeight() - 1), wxPoint(0, size.GetHeight() - 1));
    dc.DrawLine(wxPoint(0, size.GetHeight() - 1), wxPoint(0, 0));



    for (auto iter = m_axisList.begin();
        iter != m_axisList.end();
        ++iter)
    {
        if (iter->Visible == false) continue;

        if (iter->GetAxisType() == Axis::Horizontal)
        {
            wxPoint startPt, endPt;

            startPt.x = iter->GetRangeStart() * size.GetWidth();
            startPt.y = size.GetHeight() - (iter->GetPosition() * size.GetHeight());
            endPt.x = iter->GetRangeEnd() * size.GetWidth();
            endPt.y = size.GetHeight() - (iter->GetPosition() * size.GetHeight());

            wxPen pen;
            pen.SetWidth(iter->Width);
            pen.SetColour(iter->Foreground);
            dc.SetPen(pen);

            dc.DrawLine(startPt, endPt);
        }
        else if (iter->GetAxisType() == Axis::Vertical)
        {
            wxPoint startPt, endPt;

            startPt.y = size.GetHeight() - (iter->GetRangeStart() * size.GetHeight());
            startPt.x = iter->GetPosition() * size.GetWidth();
            endPt.y = size.GetHeight() - (iter->GetRangeEnd() * size.GetHeight());
            endPt.x = iter->GetPosition() * size.GetWidth();

            wxPen pen;
            pen.SetWidth(iter->Width);
            pen.SetColour(iter->Foreground);
            dc.SetPen(pen);

            dc.DrawLine(startPt, endPt);
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
    RecreateBuffer();
    Render();
    Refresh(false);
    event.Skip();
}

void wxPlotting::OnEraseBackground(wxEraseEvent& WXUNUSED(event))
{
    // Intentional no-op: painting is fully handled in OnPaint via the
    // off-screen MemoryDC buffer, which avoids flicker.
}
