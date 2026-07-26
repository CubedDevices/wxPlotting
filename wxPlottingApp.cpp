/////////////////////////////////////////////////////////////////////////////
// Name:        wxPlottingApp.cpp
// Purpose:     Test application hosting a wxPlotting control
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wxPlottingApp.h"
#include "wxPlotting.h"

#include "wx/sizer.h"
#include "wx/menu.h"
#include "wx/msgdlg.h"
#include "wx/statusbr.h"

// ----------------------------------------------------------------------------
// application
// ----------------------------------------------------------------------------

wxIMPLEMENT_APP(wxPlottingApp);

bool wxPlottingApp::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

    wxPlottingFrame* frame = new wxPlottingFrame();
    frame->Show(true);
    return true;
}

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(wxPlottingFrame, wxFrame)
    EVT_MENU(wxID_EXIT,  wxPlottingFrame::OnExit)
    EVT_MENU(wxID_ABOUT, wxPlottingFrame::OnAbout)
wxEND_EVENT_TABLE()

wxPlottingFrame::wxPlottingFrame()
    : wxFrame(nullptr, wxID_ANY, "wxPlotting Test",
              wxDefaultPosition, wxSize(800, 600)),
      m_plot(nullptr)
{
    // Menu bar
    wxMenu* fileMenu = new wxMenu;
    fileMenu->Append(wxID_EXIT, "E&xit\tAlt-X", "Quit this program");

    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(wxID_ABOUT, "&About\tF1", "Show about dialog");

    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(helpMenu, "&Help");
    SetMenuBar(menuBar);

    CreateStatusBar(1);
    SetStatusText("wxPlotting owner-drawn control test");

    // Host the plotting control in the client area.
    // Canvas grows with the client; scrollbars only if the client shrinks
    // below this minimum.  Raise min height later as series data grows.
    m_plot = new wxPlotting(this, wxID_ANY);

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
    test1.SetRange(0, 100); // 0 to 100 seconds
    test1.TickRange = 10; // tick every 10 seconds
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
    test2.SetPosition(0.0, 0.32);
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
    test3.SetPosition(0.34, 0.65);
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

    Axis test4("Signal3", Axis::Vertical);
    test4.Foreground = (wxColor("Yellow"));
    test4.Visible = true;
    test4.LabelVisible = true;
    test4.Width = 3;
    test4.LabelColor = wxColor("Yellow");
    test4.LabelFont = wxFont(wxFontInfo(14).Family(wxFONTFAMILY_MODERN).Bold());
    test4.LabelSide = Axis::BelowOrLeft;
    test4.SetLocation(0.0);
    test4.SetPosition(0.67, 1.0);
    test4.SetRange(-3.3, 3.3);   // += 3.3v
    test4.TickRange = 1.0; // 1v ticks
    test4.TickColor = (wxColor("Yellow"));
    test4.TickFont = wxFont(wxFontInfo(12).Family(wxFONTFAMILY_MODERN));
    test4.TickVisible = true;
    test4.TickLabelVisible = true;
    test4.TickLabelFormat = "%.1f"; // one decimal place
    test4.TickSide = Axis::BelowOrLeft;
    test4.GridVisible = true;
    test4.GridColor = wxColor("Yellow").ChangeLightness(50);
    test4.GridStyle = wxPENSTYLE_DOT_DASH;
    test4.GridWidth = 1;

    m_plot->AddAxis(test1);
    m_plot->AddAxis(test2);
    m_plot->AddAxis(test3);
    m_plot->AddAxis(test4);

    Series series1("Series1", Series::Doubles);
    series1.HorizontalAxis = wxString("Time");
    series1.VerticalAxis = wxString("Signal1");
    series1.Foreground = wxString("White");
    series1.Visible = true;

    {
        Series::Points pts;
        pts.reserve(100);
        for (double timestamp = 0; timestamp < 100.0; timestamp += 1.0)
        {
            double s = sin(timestamp * M_PI / 22.5) * 3.3;
            pts.push_back(wxRealPoint(timestamp, s));
        }
        series1.SetPoints(std::move(pts));
    }

    m_plot->AddSeries(series1);

    Series series2("Series2", Series::Doubles);
    series2.HorizontalAxis = wxString("Time");
    series2.VerticalAxis = wxString("Signal2");
    series2.Foreground = wxString("White");
    series2.Visible = true;

    {
        Series::Points pts;
        pts.reserve(100);
        for (double timestamp = 0; timestamp < 100.0; timestamp += 1.0)
        {
            double s = cos(timestamp * M_PI / 22.5) * 3.3;
            pts.push_back(wxRealPoint(timestamp, s));
        }
        series2.SetPoints(std::move(pts));
    }

    m_plot->AddSeries(series2);

    Series series3("Series3", Series::Doubles);
    series3.HorizontalAxis = wxString("Time");
    series3.VerticalAxis = wxString("Signal3");
    series3.Foreground = wxString("White");
    series3.Visible = true;

    {
        Series::Points pts;
        pts.reserve(100);
        for (double timestamp = 0; timestamp < 100.0; timestamp += 1.0)
        {
            double s = (cos(timestamp * M_PI / 22.5) + sin(timestamp * M_PI / 22.5)) * 3.3;
            pts.push_back(wxRealPoint(timestamp, s));
        }
        series3.SetPoints(std::move(pts));
    }

    m_plot->AddSeries(series3);


    m_plot->UpdatePlot();

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_plot, 1, wxEXPAND | wxALL, 0);
    SetSizer(sizer);

    Centre();
}

void wxPlottingFrame::OnExit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

void wxPlottingFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox(
        "wxPlotting test application\n\n"
        "Hosts an owner-drawn wxPlotting control that renders "
        "via an off-screen wxMemoryDC.",
        "About wxPlotting",
        wxOK | wxICON_INFORMATION,
        this);
}
