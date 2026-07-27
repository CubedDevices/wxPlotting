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
#include "wx/colordlg.h"
#include "wx/fontdlg.h"
#include "wx/numdlg.h"
#include "wx/textdlg.h"
#include "wx/choicdlg.h"

#include <cmath>

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

    // Axis Analog1 property menu (range of IDs)
    EVT_MENU_RANGE(ID_AXIS_VISIBLE, ID_AXIS_GRID_STYLE_TRANSPARENT,
                   wxPlottingFrame::OnAxisMenu)
    EVT_UPDATE_UI_RANGE(ID_AXIS_VISIBLE, ID_AXIS_GRID_STYLE_TRANSPARENT,
                        wxPlottingFrame::OnAxisUpdateUI)

    // Series Series1 property menu
    EVT_MENU_RANGE(ID_SERIES_VISIBLE, ID_SERIES_VERT_AXIS,
                   wxPlottingFrame::OnSeriesMenu)
    EVT_UPDATE_UI_RANGE(ID_SERIES_VISIBLE, ID_SERIES_VERT_AXIS,
                        wxPlottingFrame::OnSeriesUpdateUI)

    // Measurement cursor modes
    EVT_MENU_RANGE(ID_CURSOR_OFF, ID_CURSOR_COMBINED,
                   wxPlottingFrame::OnCursorMenu)
    EVT_UPDATE_UI_RANGE(ID_CURSOR_OFF, ID_CURSOR_COMBINED,
                        wxPlottingFrame::OnCursorUpdateUI)
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
    BuildPropertyMenus(menuBar);
    BuildCursorMenu(menuBar);
    menuBar->Append(helpMenu, "&Help");
    SetMenuBar(menuBar);

    CreateStatusBar(1);
    SetStatusText("wxPlotting owner-drawn control test");

    // Host the plotting control in the client area.
    // Canvas grows with the client; scrollbars only if the client shrinks
    // below this minimum.  Raise min height later as series data grows.
    m_plot = new wxPlotting(this, wxID_ANY);

    Axis timeAxis("Time", Axis::Horizontal);
    timeAxis.Foreground = (wxColor("Green"));
    timeAxis.Visible = true;
    timeAxis.LabelVisible = true;
    timeAxis.Width = 3;
    timeAxis.LabelColor = wxColor("Green");
    timeAxis.LabelFont = wxFont(wxFontInfo(14).Family(wxFONTFAMILY_MODERN).Bold());
    timeAxis.LabelSide = Axis::BelowOrLeft;
    timeAxis.SetLocation(0.0);
    timeAxis.SetPosition(0.0, 1.0);
    timeAxis.SetRange(0, 256); // 0 to 100 seconds
    timeAxis.TickRange = 10; // tick every 10 seconds
    timeAxis.TickColor = wxColor("Green");
    timeAxis.TickFont = wxFont(wxFontInfo(12).Family(wxFONTFAMILY_MODERN));
    timeAxis.TickVisible = true;
    timeAxis.TickLabelVisible = true;
    timeAxis.TickLabelFormat = "%.0f"; // integer seconds
    timeAxis.TickSide = Axis::BelowOrLeft;
    timeAxis.GridVisible = true;
    timeAxis.GridColor = wxColor("Grey");
    timeAxis.GridStyle = wxPENSTYLE_SOLID;
    timeAxis.GridWidth = 1;

    Axis a1("Analog1", Axis::Vertical);
    a1.Foreground = (wxColor("Magenta"));
    a1.Visible = true;
    a1.LabelVisible = true;
    a1.LabelRotated = true;
    a1.Width = 3;
    a1.LabelColor = wxColor("Magenta");
    a1.LabelFont = wxFont(wxFontInfo(14).Family(wxFONTFAMILY_MODERN).Bold());
    a1.LabelSide = Axis::BelowOrLeft;
    a1.SetLocation(0.0);
    a1.SetPosition(0.77, 1.0);
    a1.SetRange(-3.3, 3.3);   // += 3.3v
    a1.TickRange = 1.0;      // 1v ticks
    a1.TickColor = wxColor("Magenta");
    a1.TickFont = wxFont(wxFontInfo(12).Family(wxFONTFAMILY_MODERN));
    a1.TickVisible = true;
    a1.TickLabelVisible = true;
    a1.TickLabelFormat = "%.1f"; // one decimal place
    a1.TickSide = Axis::BelowOrLeft;
    a1.GridVisible = true;
    a1.GridColor = wxColor("Magenta").ChangeLightness(50);
    a1.GridStyle = wxPENSTYLE_DOT_DASH;
    a1.GridWidth = 1;

    Axis a2("Analog2", Axis::Vertical);
    a2.Foreground = (wxColor("Cyan"));
    a2.Visible = true;
    a2.LabelVisible = true;
    a2.LabelRotated = true;
    a2.Width = 3;
    a2.LabelColor = wxColor("Cyan");
    a2.LabelFont = wxFont(wxFontInfo(14).Family(wxFONTFAMILY_MODERN).Bold());
    a2.LabelSide = Axis::BelowOrLeft;
    a2.SetLocation(0.0);
    a2.SetPosition(0.50, 0.73);
    a2.SetRange(-3.3, 3.3);   // += 3.3v
    a2.TickRange = 1.0; // 1v ticks
    a2.TickColor = (wxColor("Cyan"));
    a2.TickFont = wxFont(wxFontInfo(12).Family(wxFONTFAMILY_MODERN));
    a2.TickVisible = true;
    a2.TickLabelVisible = true;
    a2.TickLabelFormat = "%.1f"; // one decimal place
    a2.TickSide = Axis::BelowOrLeft;
    a2.GridVisible = true;
    a2.GridColor = wxColor("Cyan").ChangeLightness(50);
    a2.GridStyle = wxPENSTYLE_DOT_DASH;
    a2.GridWidth = 1;


    Axis d0("D0", Axis::Vertical);
    d0.Visible = true;
    d0.Foreground = wxColor("White");
    d0.GridVisible = false;
    d0.GridStyle = wxPENSTYLE_SOLID;
    d0.GridColor = wxColor("White").ChangeLightness(150);
    d0.Label = "D0";
    d0.LabelColor = wxColor("White");
    d0.LabelVisible = true;
    d0.LabelSide = Axis::BelowOrLeft;
    d0.SetLocation(0);
    d0.SetPosition(0.0, 0.04);
    d0.SetRange(0, 1);
    d0.TickLabelVisible = false;
    d0.TickVisible = false;
    m_plot->AddAxis(d0);

    Axis d1("D1", Axis::Vertical);
    d1.Visible = true;
    d1.Foreground = wxColor("Red");
    d1.GridVisible = false;
    d1.GridStyle = wxPENSTYLE_SOLID;
    d1.GridColor = wxColor("Red").ChangeLightness(150);
    d1.Label = "D1";
    d1.LabelColor = wxColor("Red");
    d1.LabelVisible = true;
    d1.LabelSide = Axis::BelowOrLeft;
    d1.SetLocation(0);
    d1.SetPosition(0.05, 0.10);
    d1.SetRange(0, 1);
    d1.TickLabelVisible = false;
    d1.TickVisible = false;
    m_plot->AddAxis(d1);

    Axis d2("D2", Axis::Vertical);
    d2.Visible = true;
    d2.Foreground = wxColor("Orange");
    d2.GridVisible = false;
    d2.GridStyle = wxPENSTYLE_SOLID;
    d2.GridColor = wxColor("Orange").ChangeLightness(150);
    d2.Label = "D2";
    d2.LabelColor = wxColor("Orange");
    d2.LabelVisible = true;
    d2.LabelSide = Axis::BelowOrLeft;
    d2.SetLocation(0);
    d2.SetPosition(0.11, 0.16);
    d2.SetRange(0, 1);
    d2.TickLabelVisible = false;
    d2.TickVisible = false;
    m_plot->AddAxis(d2);

    Axis d3("D3", Axis::Vertical);
    d3.Visible = true;
    d3.Foreground = wxColor("Yellow");
    d3.GridVisible = false;
    d3.GridStyle = wxPENSTYLE_SOLID;
    d3.GridColor = wxColor("Yellow").ChangeLightness(150);
    d3.Label = "D3";
    d3.LabelColor = wxColor("Yellow");
    d3.LabelVisible = true;
    d3.LabelSide = Axis::BelowOrLeft;
    d3.SetLocation(0);
    d3.SetPosition(0.17, 0.22);
    d3.SetRange(0, 1);
    d3.TickLabelVisible = false;
    d3.TickVisible = false;
    m_plot->AddAxis(d3);

    Axis d4("D4", Axis::Vertical);
    d4.Visible = true;
    d4.Foreground = wxColor("Cyan");
    d4.GridVisible = false;
    d4.GridStyle = wxPENSTYLE_SOLID;
    d4.GridColor = wxColor("Cyan").ChangeLightness(150);
    d4.Label = "D4";
    d4.LabelColor = wxColor("Cyan");
    d4.LabelVisible = true;
    d4.LabelSide = Axis::BelowOrLeft;
    d4.SetLocation(0);
    d4.SetPosition(0.23, 0.28);
    d4.SetRange(0, 1);
    d4.TickLabelVisible = false;
    d4.TickVisible = false;
    m_plot->AddAxis(d4);

    Axis d5("D5", Axis::Vertical);
    d5.Visible = true;
    d5.Foreground = wxColor("Blue");
    d5.GridVisible = false;
    d5.GridStyle = wxPENSTYLE_SOLID;
    d5.GridColor = wxColor("Blue").ChangeLightness(150);
    d5.Label = "D5";
    d5.LabelColor = wxColor("Blue");
    d5.LabelVisible = true;
    d5.LabelSide = Axis::BelowOrLeft;
    d5.SetLocation(0);
    d5.SetPosition(0.29, 0.34);
    d5.SetRange(0, 1);
    d5.TickLabelVisible = false;
    d5.TickVisible = false;
    m_plot->AddAxis(d5);

    Axis d6("D6", Axis::Vertical);
    d6.Visible = true;
    d6.Foreground = wxColor("Magenta");
    d6.GridVisible = false;
    d6.GridStyle = wxPENSTYLE_SOLID;
    d6.GridColor = wxColor("Magenta").ChangeLightness(150);
    d6.Label = "D6";
    d6.LabelColor = wxColor("Magenta");
    d6.LabelVisible = true;
    d6.LabelSide = Axis::BelowOrLeft;
    d6.SetLocation(0);
    d6.SetPosition(0.35, 0.40);
    d6.SetRange(0, 1);
    d6.TickLabelVisible = false;
    d6.TickVisible = false;
    m_plot->AddAxis(d6);

    Axis d7("D7", Axis::Vertical);
    d7.Visible = true;
    d7.Foreground = wxColor("Grey");
    d7.GridVisible = false;
    d7.GridStyle = wxPENSTYLE_SOLID;
    d7.GridColor = wxColor("Grey").ChangeLightness(150);
    d7.Label = "D7";
    d7.LabelColor = wxColor("Grey");
    d7.LabelVisible = true;
    d7.LabelSide = Axis::BelowOrLeft;
    d7.SetLocation(0);
    d7.SetPosition(0.41, 0.46);
    d7.SetRange(0, 1);
    d7.TickLabelVisible = false;
    d7.TickVisible = false;
    m_plot->AddAxis(d7);

    m_plot->AddAxis(timeAxis);
    m_plot->AddAxis(a1);
    m_plot->AddAxis(a2);

    Series series1("Series1", Series::Doubles);
    series1.HorizontalAxis = wxString("Time");
    series1.VerticalAxis = wxString("Analog1");
    series1.Foreground = wxColor("White");
    series1.Visible = true;

    {
        Series::Points pts;
        pts.reserve(100);
        for (double timestamp = 0; timestamp < 256; timestamp += 1.0)
        {
            double s = sin(timestamp * M_PI / 22.5) * 3.3;
            pts.push_back(wxRealPoint(timestamp, s));
        }
        series1.SetPoints(std::move(pts));
    }

    m_plot->AddSeries(series1);


    Series series2("Series2", Series::Doubles);
    series2.HorizontalAxis = wxString("Time");
    series2.VerticalAxis = wxString("Analog2");
    series2.Foreground = wxColor("White");
    series2.Visible = true;

    {
        Series::Points pts;
        pts.reserve(100);
        for (double timestamp = 0; timestamp < 256; timestamp += 1.0)
        {
            double s = cos(timestamp * M_PI / 22.5) * 3.3;
            pts.push_back(wxRealPoint(timestamp, s));
        }
        series2.SetPoints(std::move(pts));
    }

    m_plot->AddSeries(series2);


    Series d0_series("D0", Series::Doubles);
    d0_series.HorizontalAxis = wxString("Time");
    d0_series.VerticalAxis = wxString("D0");
    d0_series.Foreground = wxColor("White");
    d0_series.Visible = true;
    d0_series.RenderStyle = SeriesRenderStyle::StairStep;

    {
        Series::Points pts;
        pts.reserve(100);
        for (double timestamp = 0; timestamp < 256; timestamp += 1.0)
        {
            int v = (int)timestamp;
            int v_b = (v & 0x01);

            pts.push_back(wxRealPoint(timestamp, (double)v_b));
        }
        d0_series.SetPoints(std::move(pts));
    }

    m_plot->AddSeries(d0_series);

    Series d1_series("D1", Series::Doubles);
    d1_series.HorizontalAxis = wxString("Time");
    d1_series.VerticalAxis = wxString("D1");
    d1_series.Foreground = wxColor("Red");
    d1_series.Visible = true;
    d1_series.RenderStyle = SeriesRenderStyle::StairStep;

    {
        Series::Points pts;
        pts.reserve(100);
        for (double timestamp = 0; timestamp < 256; timestamp += 1.0)
        {
            int v = (int)timestamp;
            int v_b = (v & 0x02) >> 1;

            pts.push_back(wxRealPoint(timestamp, (double)v_b));
        }
        d1_series.SetPoints(std::move(pts));
    }

    m_plot->AddSeries(d1_series);

    Series d2_series("D2", Series::Doubles);
    d2_series.HorizontalAxis = wxString("Time");
    d2_series.VerticalAxis = wxString("D2");
    d2_series.Foreground = wxColor("Orange");
    d2_series.Visible = true;
    d2_series.RenderStyle = SeriesRenderStyle::StairStep;

    {
        Series::Points pts;
        pts.reserve(100);
        for (double timestamp = 0; timestamp < 256; timestamp += 1.0)
        {
            int v = (int)timestamp;
            int v_b = (v & 0x04) >> 2;

            pts.push_back(wxRealPoint(timestamp, (double)v_b));
        }
        d2_series.SetPoints(std::move(pts));
    }

    m_plot->AddSeries(d2_series);

    Series d3_series("D3", Series::Doubles);
    d3_series.HorizontalAxis = wxString("Time");
    d3_series.VerticalAxis = wxString("D3");
    d3_series.Foreground = wxColor("Yellow");
    d3_series.Visible = true;
    d3_series.RenderStyle = SeriesRenderStyle::StairStep;

    {
        Series::Points pts;
        pts.reserve(100);
        for (double timestamp = 0; timestamp < 256; timestamp += 1.0)
        {
            int v = (int)timestamp;
            int v_b = (v & 0x08) >> 3;

            pts.push_back(wxRealPoint(timestamp, (double)v_b));
        }
        d3_series.SetPoints(std::move(pts));
    }

    m_plot->AddSeries(d3_series);

    Series d4_series("D4", Series::Doubles);
    d4_series.HorizontalAxis = wxString("Time");
    d4_series.VerticalAxis = wxString("D4");
    d4_series.Foreground = wxColor("Cyan");
    d4_series.Visible = true;
    d4_series.RenderStyle = SeriesRenderStyle::StairStep;

    {
        Series::Points pts;
        pts.reserve(100);
        for (double timestamp = 0; timestamp < 256; timestamp += 1.0)
        {
            int v = (int)timestamp;
            int v_b = (v & 0x10) >> 4;

            pts.push_back(wxRealPoint(timestamp, (double)v_b));
        }
        d4_series.SetPoints(std::move(pts));
    }

    m_plot->AddSeries(d4_series);

    Series d5_series("D5", Series::Doubles);
    d5_series.HorizontalAxis = wxString("Time");
    d5_series.VerticalAxis = wxString("D5");
    d5_series.Foreground = wxColor("Blue");
    d5_series.Visible = true;
    d5_series.RenderStyle = SeriesRenderStyle::StairStep;

    {
        Series::Points pts;
        pts.reserve(100);
        for (double timestamp = 0; timestamp < 256; timestamp += 1.0)
        {
            int v = (int)timestamp;
            int v_b = (v & 0x20) >> 5;

            pts.push_back(wxRealPoint(timestamp, (double)v_b));
        }
        d5_series.SetPoints(std::move(pts));
    }

    m_plot->AddSeries(d5_series);

    Series d6_series("D6", Series::Doubles);
    d6_series.HorizontalAxis = wxString("Time");
    d6_series.VerticalAxis = wxString("D6");
    d6_series.Foreground = wxColor("Magenta");
    d6_series.Visible = true;
    d6_series.RenderStyle = SeriesRenderStyle::StairStep;

    {
        Series::Points pts;
        pts.reserve(100);
        for (double timestamp = 0; timestamp < 256; timestamp += 1.0)
        {
            int v = (int)timestamp;
            int v_b = (v & 0x40) >> 6;

            pts.push_back(wxRealPoint(timestamp, (double)v_b));
        }
        d6_series.SetPoints(std::move(pts));
    }

    m_plot->AddSeries(d6_series);

    Series d7_series("D7", Series::Doubles);
    d7_series.HorizontalAxis = wxString("Time");
    d7_series.VerticalAxis = wxString("D7");
    d7_series.Foreground = wxColor("Grey");
    d7_series.Visible = true;
    d7_series.RenderStyle = SeriesRenderStyle::StairStep;

    {
        Series::Points pts;
        pts.reserve(100);
        for (double timestamp = 0; timestamp < 256; timestamp += 1.0)
        {
            int v = (int)timestamp;
            int v_b = (v & 0x80) >> 7;

            pts.push_back(wxRealPoint(timestamp, (double)v_b));
        }
        d7_series.SetPoints(std::move(pts));
    }

    m_plot->AddSeries(d7_series);

    m_plot->UpdatePlot();

    // Interaction demo: hit-test hover + vertical axis band drag.
    m_plot->Bind(wxEVT_MOTION, &wxPlottingFrame::OnPlotMotion, this);
    m_plot->Bind(wxEVT_LEFT_DOWN, &wxPlottingFrame::OnPlotLeftDown, this);
    m_plot->Bind(wxEVT_LEFT_UP, &wxPlottingFrame::OnPlotLeftUp, this);
    m_plot->Bind(wxEVT_MOUSE_CAPTURE_LOST, &wxPlottingFrame::OnPlotCaptureLost, this);
    m_plot->Bind(wxEVT_SET_CURSOR, &wxPlottingFrame::OnPlotSetCursor, this);
    m_plot->Bind(wxEVT_LEAVE_WINDOW, &wxPlottingFrame::OnPlotLeaveWindow, this);
    m_plot->Bind(wxEVT_MOUSEWHEEL, &wxPlottingFrame::OnPlotMouseWheel, this);

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
        "via an off-screen wxMemoryDC.\n\n"
        "• Hover to see hit-test results in the status bar.\n"
        "• Drag a vertical axis band (not the time axis) to move it "
        "while keeping its height (start/end span) fixed.\n"
        "• Mouse wheel over Analog1/Analog2 zooms that axis data range "
        "by 25% (up = zoom in, down = zoom out).\n"
        "• Use the Cursor menu for measurement guides (horizontal, "
        "vertical, or combined crosshair) with axis values near the pointer.\n"
        "• Use the Axis and Series menus to edit Analog1 and Series1.",
        "About wxPlotting",
        wxOK | wxICON_INFORMATION,
        this);
}

// ----------------------------------------------------------------------------
// property menus (Analog1 / Series1)
// ----------------------------------------------------------------------------

void wxPlottingFrame::BuildPropertyMenus(wxMenuBar* menuBar)
{
    // ----- Axis: Analog1 ---------------------------------------------------
    wxMenu* axisMenu = new wxMenu;

    axisMenu->AppendCheckItem(ID_AXIS_VISIBLE, "&Visible",
                              "Show or hide the Analog1 axis");
    axisMenu->Append(ID_AXIS_WIDTH, "&Width...",
                     "Axis line pen width in pixels");
    axisMenu->Append(ID_AXIS_FOREGROUND, "&Foreground colour...",
                     "Axis line colour");
    axisMenu->AppendSeparator();

    axisMenu->Append(ID_AXIS_LOCATION, "L&ocation...",
                     "Normalized axis line location [0,1]");
    axisMenu->Append(ID_AXIS_POS_START, "Position &start...",
                     "Normalized band start [0,1]");
    axisMenu->Append(ID_AXIS_POS_END, "Position &end...",
                     "Normalized band end [0,1]");
    axisMenu->Append(ID_AXIS_RANGE_START, "&Range start...",
                     "Data range start (e.g. volts)");
    axisMenu->Append(ID_AXIS_RANGE_END, "Range e&nd...",
                     "Data range end (e.g. volts)");
    axisMenu->Append(ID_AXIS_SCALE, "Sca&le...",
                     "Reserved scale factor");
    axisMenu->AppendSeparator();

    axisMenu->AppendCheckItem(ID_AXIS_TICK_VISIBLE, "&Tick marks",
                              "Show tick marks");
    axisMenu->Append(ID_AXIS_TICK_COLOR, "Tick &colour...",
                     "Tick mark and tick label colour");
    {
        wxMenu* tickSide = new wxMenu;
        tickSide->AppendRadioItem(ID_AXIS_TICK_SIDE_BELOW, "Below / &Left");
        tickSide->AppendRadioItem(ID_AXIS_TICK_SIDE_ABOVE, "Above / &Right");
        axisMenu->AppendSubMenu(tickSide, "Tick s&ide");
    }
    axisMenu->Append(ID_AXIS_TICK_RANGE, "Tick r&ange...",
                     "Spacing between ticks in data units");
    axisMenu->Append(ID_AXIS_TICK_FONT, "Tick &font...",
                     "Font for numeric tick labels");
    axisMenu->AppendCheckItem(ID_AXIS_TICK_LABEL_VISIBLE, "Tick la&bels",
                              "Show numeric values at ticks");
    axisMenu->Append(ID_AXIS_TICK_LABEL_FORMAT, "Tick label f&ormat...",
                     "printf-style format (e.g. %.1f)");
    axisMenu->AppendSeparator();

    axisMenu->Append(ID_AXIS_LABEL, "Title te&xt...",
                     "Axis title string");
    axisMenu->AppendCheckItem(ID_AXIS_LABEL_VISIBLE, "Title &visible",
                              "Show the axis title");
    axisMenu->Append(ID_AXIS_LABEL_COLOR, "Title c&olour...",
                     "Axis title colour");
    {
        wxMenu* labelSide = new wxMenu;
        labelSide->AppendRadioItem(ID_AXIS_LABEL_SIDE_BELOW, "Below / &Left");
        labelSide->AppendRadioItem(ID_AXIS_LABEL_SIDE_ABOVE, "Above / &Right");
        axisMenu->AppendSubMenu(labelSide, "Title si&de");
    }
    axisMenu->Append(ID_AXIS_LABEL_FONT, "Title fo&nt...",
                     "Axis title font");
    axisMenu->AppendCheckItem(ID_AXIS_LABEL_ROTATED, "Title &rotated",
                              "Rotate vertical titles 90 degrees");
    axisMenu->AppendSeparator();

    axisMenu->AppendCheckItem(ID_AXIS_GRID_VISIBLE, "&Grid",
                              "Show grid lines at ticks");
    axisMenu->Append(ID_AXIS_GRID_COLOR, "Grid colo&ur...",
                     "Grid line colour");
    axisMenu->Append(ID_AXIS_GRID_WIDTH, "Grid wi&dth...",
                     "Grid pen width in pixels");
    {
        wxMenu* gridStyle = new wxMenu;
        gridStyle->AppendRadioItem(ID_AXIS_GRID_STYLE_SOLID, "&Solid");
        gridStyle->AppendRadioItem(ID_AXIS_GRID_STYLE_DOT, "&Dot");
        gridStyle->AppendRadioItem(ID_AXIS_GRID_STYLE_LONG_DASH, "&Long dash");
        gridStyle->AppendRadioItem(ID_AXIS_GRID_STYLE_SHORT_DASH, "S&hort dash");
        gridStyle->AppendRadioItem(ID_AXIS_GRID_STYLE_DOT_DASH, "Dot-das&h");
        gridStyle->AppendRadioItem(ID_AXIS_GRID_STYLE_TRANSPARENT, "&Transparent");
        axisMenu->AppendSubMenu(gridStyle, "Grid st&yle");
    }

    // ----- Series: Series1 -------------------------------------------------
    wxMenu* seriesMenu = new wxMenu;
    seriesMenu->AppendCheckItem(ID_SERIES_VISIBLE, "&Visible",
                                "Show or hide Series1");
    seriesMenu->Append(ID_SERIES_FOREGROUND, "&Foreground colour...",
                       "Series stroke colour");
    {
        wxMenu* styleMenu = new wxMenu;
        styleMenu->AppendRadioItem(ID_SERIES_STYLE_LINE, "&Line");
        styleMenu->AppendRadioItem(ID_SERIES_STYLE_STAIR, "&Stair step");
        seriesMenu->AppendSubMenu(styleMenu, "&Render style");
    }
    seriesMenu->AppendSeparator();
    seriesMenu->Append(ID_SERIES_HORZ_AXIS, "&Horizontal axis...",
                       "Name of bound horizontal axis");
    seriesMenu->Append(ID_SERIES_VERT_AXIS, "&Vertical axis...",
                       "Name of bound vertical axis");

    menuBar->Append(axisMenu, "&Axis");
    menuBar->Append(seriesMenu, "&Series");
}

void wxPlottingFrame::BuildCursorMenu(wxMenuBar* menuBar)
{
    wxMenu* cursorMenu = new wxMenu;
    cursorMenu->AppendRadioItem(ID_CURSOR_OFF, "&Off",
                                "Hide measurement cursors");
    cursorMenu->AppendRadioItem(ID_CURSOR_HORIZONTAL, "&Horizontal",
                                "Vertical guide line; horizontal-axis value at pointer");
    cursorMenu->AppendRadioItem(ID_CURSOR_VERTICAL, "&Vertical",
                                "Horizontal guide line; vertical-axis value at pointer");
    cursorMenu->AppendRadioItem(ID_CURSOR_COMBINED, "&Combined",
                                "Crosshair with both axis values");
    menuBar->Append(cursorMenu, "&Cursor");
}

Axis* wxPlottingFrame::DemoAxis() const
{
    return m_plot ? m_plot->GetAxis(wxT("Analog1")) : nullptr;
}

Series* wxPlottingFrame::DemoSeries() const
{
    return m_plot ? m_plot->GetSeries(wxT("Series1")) : nullptr;
}

void wxPlottingFrame::RefreshPlot()
{
    if ( m_plot )
        m_plot->UpdatePlot();
}

bool wxPlottingFrame::EditInt(const wxString& title, int& value, int minVal, int maxVal)
{
    long v = wxGetNumberFromUser(wxT("Enter integer value:"), title, title,
                                 value, minVal, maxVal, this);
    if ( v == -1 && value != -1 )
    {
        // wxGetNumberFromUser returns -1 on cancel; allow -1 as a real value only
        // if it was already -1. For our width uses min is >= 0 so cancel is fine.
        return false;
    }
    if ( v < minVal || v > maxVal )
        return false;
    if ( static_cast<int>(v) == value )
        return false;
    value = static_cast<int>(v);
    return true;
}

bool wxPlottingFrame::EditDouble(const wxString& title, double& value)
{
    wxString def = wxString::Format(wxT("%.6g"), value);
    wxTextEntryDialog dlg(this, wxT("Enter numeric value:"), title, def);
    if ( dlg.ShowModal() != wxID_OK )
        return false;
    double v = 0.0;
    if ( !dlg.GetValue().ToDouble(&v) )
    {
        wxMessageBox(wxT("Invalid number."), wxT("Edit"), wxOK | wxICON_WARNING, this);
        return false;
    }
    if ( v == value )
        return false;
    value = v;
    return true;
}

bool wxPlottingFrame::EditString(const wxString& title, wxString& value)
{
    wxTextEntryDialog dlg(this, wxT("Enter text:"), title, value);
    if ( dlg.ShowModal() != wxID_OK )
        return false;
    if ( dlg.GetValue() == value )
        return false;
    value = dlg.GetValue();
    return true;
}

bool wxPlottingFrame::EditColour(const wxString& title, wxColour& value)
{
    const wxColour c = wxGetColourFromUser(this, value, title);
    if ( !c.IsOk() )
        return false;
    if ( c == value )
        return false;
    value = c;
    return true;
}

bool wxPlottingFrame::EditFont(const wxString& title, wxFont& value)
{
    wxFontData data;
    data.SetInitialFont(value);
    wxFontDialog dlg(this, data);
    dlg.SetTitle(title);
    if ( dlg.ShowModal() != wxID_OK )
        return false;
    const wxFont f = dlg.GetFontData().GetChosenFont();
    if ( !f.IsOk() )
        return false;
    value = f;
    return true;
}

void wxPlottingFrame::OnAxisMenu(wxCommandEvent& event)
{
    Axis* axis = DemoAxis();
    if ( !axis )
    {
        wxMessageBox(wxT("Axis \"Analog1\" is not on the plot."),
                     wxT("Axis"), wxOK | wxICON_WARNING, this);
        return;
    }

    bool changed = false;
    const int id = event.GetId();

    switch ( id )
    {
        case ID_AXIS_VISIBLE:
            axis->Visible = event.IsChecked();
            changed = true;
            break;
        case ID_AXIS_WIDTH:
            changed = EditInt(wxT("Axis width"), axis->Width, 1, 64);
            break;
        case ID_AXIS_FOREGROUND:
            changed = EditColour(wxT("Axis foreground"), axis->Foreground);
            break;
        case ID_AXIS_LOCATION:
        {
            double v = axis->GetLocation();
            if ( EditDouble(wxT("Axis location [0,1]"), v) )
            {
                axis->SetLocation(v);
                changed = true;
            }
            break;
        }
        case ID_AXIS_POS_START:
        {
            double s = axis->GetStart();
            double e = axis->GetEnd();
            if ( EditDouble(wxT("Position start"), s) )
            {
                axis->SetPosition(s, e);
                changed = true;
            }
            break;
        }
        case ID_AXIS_POS_END:
        {
            double s = axis->GetStart();
            double e = axis->GetEnd();
            if ( EditDouble(wxT("Position end"), e) )
            {
                axis->SetPosition(s, e);
                changed = true;
            }
            break;
        }
        case ID_AXIS_RANGE_START:
        {
            double s = axis->GetRangeStart();
            double e = axis->GetRangeEnd();
            if ( EditDouble(wxT("Range start"), s) )
            {
                axis->SetRange(s, e);
                changed = true;
            }
            break;
        }
        case ID_AXIS_RANGE_END:
        {
            double s = axis->GetRangeStart();
            double e = axis->GetRangeEnd();
            if ( EditDouble(wxT("Range end"), e) )
            {
                axis->SetRange(s, e);
                changed = true;
            }
            break;
        }
        case ID_AXIS_SCALE:
            changed = EditDouble(wxT("Scale"), axis->Scale);
            break;
        case ID_AXIS_TICK_VISIBLE:
            axis->TickVisible = event.IsChecked();
            changed = true;
            break;
        case ID_AXIS_TICK_COLOR:
            changed = EditColour(wxT("Tick colour"), axis->TickColor);
            break;
        case ID_AXIS_TICK_SIDE_BELOW:
            if ( axis->TickSide != Axis::BelowOrLeft )
            {
                axis->TickSide = Axis::BelowOrLeft;
                changed = true;
            }
            break;
        case ID_AXIS_TICK_SIDE_ABOVE:
            if ( axis->TickSide != Axis::AboveOrRight )
            {
                axis->TickSide = Axis::AboveOrRight;
                changed = true;
            }
            break;
        case ID_AXIS_TICK_RANGE:
            changed = EditDouble(wxT("Tick range"), axis->TickRange);
            break;
        case ID_AXIS_TICK_FONT:
            changed = EditFont(wxT("Tick font"), axis->TickFont);
            break;
        case ID_AXIS_TICK_LABEL_VISIBLE:
            axis->TickLabelVisible = event.IsChecked();
            changed = true;
            break;
        case ID_AXIS_TICK_LABEL_FORMAT:
            changed = EditString(wxT("Tick label format"), axis->TickLabelFormat);
            break;
        case ID_AXIS_LABEL:
            changed = EditString(wxT("Axis title"), axis->Label);
            break;
        case ID_AXIS_LABEL_VISIBLE:
            axis->LabelVisible = event.IsChecked();
            changed = true;
            break;
        case ID_AXIS_LABEL_COLOR:
            changed = EditColour(wxT("Title colour"), axis->LabelColor);
            break;
        case ID_AXIS_LABEL_SIDE_BELOW:
            if ( axis->LabelSide != Axis::BelowOrLeft )
            {
                axis->LabelSide = Axis::BelowOrLeft;
                changed = true;
            }
            break;
        case ID_AXIS_LABEL_SIDE_ABOVE:
            if ( axis->LabelSide != Axis::AboveOrRight )
            {
                axis->LabelSide = Axis::AboveOrRight;
                changed = true;
            }
            break;
        case ID_AXIS_LABEL_FONT:
            changed = EditFont(wxT("Title font"), axis->LabelFont);
            break;
        case ID_AXIS_LABEL_ROTATED:
            axis->LabelRotated = event.IsChecked();
            changed = true;
            break;
        case ID_AXIS_GRID_VISIBLE:
            axis->GridVisible = event.IsChecked();
            changed = true;
            break;
        case ID_AXIS_GRID_COLOR:
            changed = EditColour(wxT("Grid colour"), axis->GridColor);
            break;
        case ID_AXIS_GRID_WIDTH:
            changed = EditInt(wxT("Grid width"), axis->GridWidth, 0, 64);
            break;
        case ID_AXIS_GRID_STYLE_SOLID:
            if ( axis->GridStyle != wxPENSTYLE_SOLID )
            { axis->GridStyle = wxPENSTYLE_SOLID; changed = true; }
            break;
        case ID_AXIS_GRID_STYLE_DOT:
            if ( axis->GridStyle != wxPENSTYLE_DOT )
            { axis->GridStyle = wxPENSTYLE_DOT; changed = true; }
            break;
        case ID_AXIS_GRID_STYLE_LONG_DASH:
            if ( axis->GridStyle != wxPENSTYLE_LONG_DASH )
            { axis->GridStyle = wxPENSTYLE_LONG_DASH; changed = true; }
            break;
        case ID_AXIS_GRID_STYLE_SHORT_DASH:
            if ( axis->GridStyle != wxPENSTYLE_SHORT_DASH )
            { axis->GridStyle = wxPENSTYLE_SHORT_DASH; changed = true; }
            break;
        case ID_AXIS_GRID_STYLE_DOT_DASH:
            if ( axis->GridStyle != wxPENSTYLE_DOT_DASH )
            { axis->GridStyle = wxPENSTYLE_DOT_DASH; changed = true; }
            break;
        case ID_AXIS_GRID_STYLE_TRANSPARENT:
            if ( axis->GridStyle != wxPENSTYLE_TRANSPARENT )
            { axis->GridStyle = wxPENSTYLE_TRANSPARENT; changed = true; }
            break;
        default:
            break;
    }

    if ( changed )
        RefreshPlot();
}

void wxPlottingFrame::OnAxisUpdateUI(wxUpdateUIEvent& event)
{
    Axis* axis = DemoAxis();
    event.Enable(axis != nullptr);
    if ( !axis )
        return;

    switch ( event.GetId() )
    {
        case ID_AXIS_VISIBLE:
            event.Check(axis->Visible);
            break;
        case ID_AXIS_TICK_VISIBLE:
            event.Check(axis->TickVisible);
            break;
        case ID_AXIS_TICK_SIDE_BELOW:
            event.Check(axis->TickSide == Axis::BelowOrLeft);
            break;
        case ID_AXIS_TICK_SIDE_ABOVE:
            event.Check(axis->TickSide == Axis::AboveOrRight);
            break;
        case ID_AXIS_TICK_LABEL_VISIBLE:
            event.Check(axis->TickLabelVisible);
            break;
        case ID_AXIS_LABEL_VISIBLE:
            event.Check(axis->LabelVisible);
            break;
        case ID_AXIS_LABEL_SIDE_BELOW:
            event.Check(axis->LabelSide == Axis::BelowOrLeft);
            break;
        case ID_AXIS_LABEL_SIDE_ABOVE:
            event.Check(axis->LabelSide == Axis::AboveOrRight);
            break;
        case ID_AXIS_LABEL_ROTATED:
            event.Check(axis->LabelRotated);
            break;
        case ID_AXIS_GRID_VISIBLE:
            event.Check(axis->GridVisible);
            break;
        case ID_AXIS_GRID_STYLE_SOLID:
            event.Check(axis->GridStyle == wxPENSTYLE_SOLID);
            break;
        case ID_AXIS_GRID_STYLE_DOT:
            event.Check(axis->GridStyle == wxPENSTYLE_DOT);
            break;
        case ID_AXIS_GRID_STYLE_LONG_DASH:
            event.Check(axis->GridStyle == wxPENSTYLE_LONG_DASH);
            break;
        case ID_AXIS_GRID_STYLE_SHORT_DASH:
            event.Check(axis->GridStyle == wxPENSTYLE_SHORT_DASH);
            break;
        case ID_AXIS_GRID_STYLE_DOT_DASH:
            event.Check(axis->GridStyle == wxPENSTYLE_DOT_DASH);
            break;
        case ID_AXIS_GRID_STYLE_TRANSPARENT:
            event.Check(axis->GridStyle == wxPENSTYLE_TRANSPARENT);
            break;
        default:
            break;
    }
}

void wxPlottingFrame::OnSeriesMenu(wxCommandEvent& event)
{
    Series* series = DemoSeries();
    if ( !series )
    {
        wxMessageBox(wxT("Series \"Series1\" is not on the plot."),
                     wxT("Series"), wxOK | wxICON_WARNING, this);
        return;
    }

    bool changed = false;
    switch ( event.GetId() )
    {
        case ID_SERIES_VISIBLE:
            series->Visible = event.IsChecked();
            changed = true;
            break;
        case ID_SERIES_FOREGROUND:
            changed = EditColour(wxT("Series foreground"), series->Foreground);
            break;
        case ID_SERIES_STYLE_LINE:
            if ( series->RenderStyle != SeriesRenderStyle::Line )
            {
                series->RenderStyle = SeriesRenderStyle::Line;
                changed = true;
            }
            break;
        case ID_SERIES_STYLE_STAIR:
            if ( series->RenderStyle != SeriesRenderStyle::StairStep )
            {
                series->RenderStyle = SeriesRenderStyle::StairStep;
                changed = true;
            }
            break;
        case ID_SERIES_HORZ_AXIS:
            changed = EditString(wxT("Horizontal axis name"), series->HorizontalAxis);
            break;
        case ID_SERIES_VERT_AXIS:
            changed = EditString(wxT("Vertical axis name"), series->VerticalAxis);
            break;
        default:
            break;
    }

    if ( changed )
        RefreshPlot();
}

void wxPlottingFrame::OnSeriesUpdateUI(wxUpdateUIEvent& event)
{
    Series* series = DemoSeries();
    event.Enable(series != nullptr);
    if ( !series )
        return;

    switch ( event.GetId() )
    {
        case ID_SERIES_VISIBLE:
            event.Check(series->Visible);
            break;
        case ID_SERIES_STYLE_LINE:
            event.Check(series->RenderStyle == SeriesRenderStyle::Line);
            break;
        case ID_SERIES_STYLE_STAIR:
            event.Check(series->RenderStyle == SeriesRenderStyle::StairStep);
            break;
        default:
            break;
    }
}

void wxPlottingFrame::OnCursorMenu(wxCommandEvent& event)
{
    if ( !m_plot )
        return;

    PlotCursorMode mode = PlotCursorMode::Off;
    switch ( event.GetId() )
    {
        case ID_CURSOR_OFF:
            mode = PlotCursorMode::Off;
            break;
        case ID_CURSOR_HORIZONTAL:
            mode = PlotCursorMode::Horizontal;
            break;
        case ID_CURSOR_VERTICAL:
            mode = PlotCursorMode::Vertical;
            break;
        case ID_CURSOR_COMBINED:
            mode = PlotCursorMode::Combined;
            break;
        default:
            return;
    }

    m_plot->SetCursorMode(mode);

    switch ( mode )
    {
        case PlotCursorMode::Horizontal:
            SetStatusText(wxT("Cursor: Horizontal — vertical guide, H-axis value"));
            break;
        case PlotCursorMode::Vertical:
            SetStatusText(wxT("Cursor: Vertical — horizontal guide, V-axis value"));
            break;
        case PlotCursorMode::Combined:
            SetStatusText(wxT("Cursor: Combined — crosshair with both axis values"));
            break;
        case PlotCursorMode::Off:
        default:
            SetStatusText(wxT("Cursor: Off"));
            break;
    }
}

void wxPlottingFrame::OnCursorUpdateUI(wxUpdateUIEvent& event)
{
    if ( !m_plot )
    {
        event.Enable(false);
        return;
    }

    event.Enable(true);
    const PlotCursorMode mode = m_plot->GetCursorMode();

    switch ( event.GetId() )
    {
        case ID_CURSOR_OFF:
            event.Check(mode == PlotCursorMode::Off);
            break;
        case ID_CURSOR_HORIZONTAL:
            event.Check(mode == PlotCursorMode::Horizontal);
            break;
        case ID_CURSOR_VERTICAL:
            event.Check(mode == PlotCursorMode::Vertical);
            break;
        case ID_CURSOR_COMBINED:
            event.Check(mode == PlotCursorMode::Combined);
            break;
        default:
            break;
    }
}

bool wxPlottingFrame::IsWheelZoomAxis(const wxString& name)
{
    return name == wxT("Analog1") || name == wxT("Analog2");
}

void wxPlottingFrame::ZoomAxisRange(Axis& axis, bool zoomIn)
{
    const double lo = axis.GetRangeStart();
    const double hi = axis.GetRangeEnd();
    const double mid = (lo + hi) * 0.5;
    double halfSpan = std::fabs(hi - lo) * 0.5;

    // One wheel notch: 25% smaller (zoom in) or 25% larger (zoom out) span.
    if ( zoomIn )
        halfSpan *= 0.75;
    else
        halfSpan *= 1.25;

    // Avoid a degenerate zero-width range.
    if ( halfSpan < 1e-15 )
        halfSpan = 1e-15;

    if ( hi >= lo )
        axis.SetRange(mid - halfSpan, mid + halfSpan);
    else
        axis.SetRange(mid + halfSpan, mid - halfSpan);
}

bool wxPlottingFrame::ClientToPlotY(const wxPoint& clientPos,
                                    int& canvasY,
                                    int& plotHeight) const
{
    if ( !m_plot )
        return false;

    int canvasX = 0;
    m_plot->CalcUnscrolledPosition(clientPos.x, clientPos.y, &canvasX, &canvasY);

    plotHeight = m_plot->GetDrawingHeight()
               - m_plot->GetMarginTop()
               - m_plot->GetMarginBottom();
    return plotHeight > 0;
}

wxString wxPlottingFrame::VerticalAxisFromHit(const PlotHitResult& hit) const
{
    if ( !m_plot )
        return wxString();

    for ( const wxString& name : hit.axes )
    {
        const Axis* axis = m_plot->GetAxis(name);
        if ( axis && axis->GetAxisType() == Axis::Vertical )
            return name;
    }
    return wxString();
}

void wxPlottingFrame::UpdateHitStatus(const PlotHitResult& hit, bool dragging)
{
    wxString axesText;
    for ( size_t i = 0; i < hit.axes.size(); ++i )
    {
        if ( i )
            axesText += wxT(", ");
        axesText += hit.axes[i];
    }
    if ( axesText.empty() )
        axesText = wxT("—");

    wxString prefix;
    if ( dragging )
        prefix = wxString::Format("Dragging vertical axis \"%s\" | ", m_dragAxisName);

    switch ( hit.kind )
    {
        case PlotHitKind::Series:
            SetStatusText(prefix + wxString::Format(
                "Series: %s  axes: [%s]  sample[%lu]=(%.4g, %.4g)  d=%.1fpx",
                hit.name,
                axesText,
                static_cast<unsigned long>(hit.sampleIndex),
                hit.dataPoint.x, hit.dataPoint.y,
                hit.distancePx));
            break;
        case PlotHitKind::Axis:
            SetStatusText(prefix + wxString::Format("Axes: [%s]", axesText));
            break;
        case PlotHitKind::PlotArea:
            SetStatusText(prefix + wxT("Plot area"));
            break;
        case PlotHitKind::None:
        default:
            SetStatusText(prefix.IsEmpty() ? wxString(wxT("—")) : prefix);
            break;
    }
}

wxCursor wxPlottingFrame::CursorForHit(const PlotHitResult& hit) const
{
    if ( m_draggingAxis )
        return wxCursor(wxCURSOR_SIZENS);

    // Vertical axis band (or series over that band): show drag affordance.
    if ( !VerticalAxisFromHit(hit).IsEmpty() )
        return wxCursor(wxCURSOR_SIZENS);

    // Series stroke with no vertical axis region (unlikely) — inspect crosshair.
    if ( hit.kind == PlotHitKind::Series )
        return wxCursor(wxCURSOR_CROSS);

    return *wxSTANDARD_CURSOR;
}

void wxPlottingFrame::UpdatePlotCursor(const wxPoint& clientPos)
{
    if ( !m_plot )
        return;

    if ( m_draggingAxis )
    {
        m_plot->SetCursor(wxCursor(wxCURSOR_SIZENS));
        return;
    }

    const PlotHitResult hit = m_plot->HitTest(clientPos);
    m_plot->SetCursor(CursorForHit(hit));
}

void wxPlottingFrame::EndAxisDrag()
{
    if ( !m_draggingAxis )
        return;

    m_draggingAxis = false;
    m_dragAxisName.clear();

    if ( m_plot && m_plot->HasCapture() )
        m_plot->ReleaseMouse();

    // Restore cursor for whatever is under the pointer after release.
    if ( m_plot )
    {
        const wxPoint pt = m_plot->ScreenToClient(wxGetMousePosition());
        UpdatePlotCursor(pt);
    }
}

void wxPlottingFrame::OnPlotLeftDown(wxMouseEvent& event)
{
    if ( !m_plot || !event.LeftDown() )
    {
        event.Skip();
        return;
    }

    const PlotHitResult hit = m_plot->HitTest(event.GetPosition());
    const wxString vertName = VerticalAxisFromHit(hit);
    if ( vertName.IsEmpty() )
    {
        UpdatePlotCursor(event.GetPosition());
        event.Skip();
        return;
    }

    Axis* axis = m_plot->GetAxis(vertName);
    if ( !axis )
    {
        event.Skip();
        return;
    }

    int canvasY = 0;
    int plotHeight = 0;
    if ( !ClientToPlotY(event.GetPosition(), canvasY, plotHeight) )
    {
        event.Skip();
        return;
    }

    m_draggingAxis = true;
    m_dragAxisName = vertName;
    m_dragOrigStart = axis->GetStart();
    m_dragOrigEnd = axis->GetEnd();
    m_dragStartCanvasY = canvasY;

    m_plot->CaptureMouse();
    m_plot->SetCursor(wxCursor(wxCURSOR_SIZENS));

    UpdateHitStatus(hit, true);
    event.Skip(false); // consume — we are dragging
}

void wxPlottingFrame::OnPlotLeftUp(wxMouseEvent& event)
{
    if ( m_draggingAxis )
        EndAxisDrag();

    if ( m_plot )
    {
        UpdatePlotCursor(event.GetPosition());
        UpdateHitStatus(m_plot->HitTest(event.GetPosition()), false);
    }

    event.Skip();
}

void wxPlottingFrame::OnPlotCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event))
{
    // Do not call ReleaseMouse here — capture is already gone.
    m_draggingAxis = false;
    m_dragAxisName.clear();
    if ( m_plot )
        m_plot->SetCursor(*wxSTANDARD_CURSOR);
}

void wxPlottingFrame::OnPlotSetCursor(wxSetCursorEvent& event)
{
    // Reliable path on Windows: OS queries the window for the cursor.
    if ( !m_plot )
    {
        event.Skip();
        return;
    }

    if ( m_draggingAxis )
    {
        event.SetCursor(wxCursor(wxCURSOR_SIZENS));
        return;
    }

    const PlotHitResult hit = m_plot->HitTest(wxPoint(event.GetX(), event.GetY()));
    event.SetCursor(CursorForHit(hit));
}

void wxPlottingFrame::OnPlotLeaveWindow(wxMouseEvent& event)
{
    // Avoid leaving a resize cursor stuck when the pointer exits the plot.
    if ( m_plot && !m_draggingAxis )
        m_plot->SetCursor(*wxSTANDARD_CURSOR);

    event.Skip();
}

void wxPlottingFrame::OnPlotMouseWheel(wxMouseEvent& event)
{
    if ( !m_plot || m_draggingAxis )
    {
        event.Skip();
        return;
    }

    const int rotation = event.GetWheelRotation();
    if ( rotation == 0 )
    {
        event.Skip();
        return;
    }

    // One physical wheel notch: zoom in (away/up) or out (toward/down).
    const bool zoomIn = rotation > 0;

    const PlotHitResult hit = m_plot->HitTest(event.GetPosition());
    bool handled = false;
    wxString zoomed;

    for ( const wxString& name : hit.axes )
    {
        if ( !IsWheelZoomAxis(name) )
            continue;

        Axis* axis = m_plot->GetAxis(name);
        if ( !axis )
            continue;

        ZoomAxisRange(*axis, zoomIn);
        handled = true;
        if ( !zoomed.IsEmpty() )
            zoomed += wxT(", ");
        zoomed += name;
    }

    if ( !handled )
    {
        // Not over Analog1/Analog2 — allow normal scrolled-window wheel behavior.
        event.Skip();
        return;
    }

    m_plot->UpdatePlot();

    // Refresh status after the range change.
    const PlotHitResult hitAfter = m_plot->HitTest(event.GetPosition());
    Axis* a = nullptr;
    for ( const wxString& name : hitAfter.axes )
    {
        if ( IsWheelZoomAxis(name) )
        {
            a = m_plot->GetAxis(name);
            break;
        }
    }

    if ( a )
    {
        SetStatusText(wxString::Format(
            "Wheel %s on [%s]  range [%.4g, %.4g]  span=%.4g",
            zoomIn ? wxT("zoom in") : wxT("zoom out"),
            zoomed,
            a->GetRangeStart(), a->GetRangeEnd(),
            a->GetRangeEnd() - a->GetRangeStart()));
    }
    else
    {
        UpdateHitStatus(hitAfter, false);
    }

    // Consume the event so the plot does not also scroll this notch.
    event.Skip(false);
}

void wxPlottingFrame::OnPlotMotion(wxMouseEvent& event)
{
    if ( !m_plot )
        return;

    // Drag update: keep size-NS cursor for the whole gesture (including outside).
    if ( m_draggingAxis && event.LeftIsDown() )
    {
        m_plot->SetCursor(wxCursor(wxCURSOR_SIZENS));

        Axis* axis = m_plot->GetAxis(m_dragAxisName);
        int canvasY = 0;
        int plotHeight = 0;
        if ( axis && ClientToPlotY(event.GetPosition(), canvasY, plotHeight) )
        {
            // Screen Y increases downward; plot-normalized Y increases upward.
            const double dNy =
                -static_cast<double>(canvasY - m_dragStartCanvasY)
                / static_cast<double>(plotHeight);

            const double span = m_dragOrigEnd - m_dragOrigStart;
            double start = m_dragOrigStart + dNy;
            double end = m_dragOrigEnd + dNy;

            // Keep (end - start) fixed; clamp the band into [0, 1].
            if ( span >= 0.0 )
            {
                if ( start < 0.0 )
                {
                    start = 0.0;
                    end = span;
                }
                if ( end > 1.0 )
                {
                    end = 1.0;
                    start = 1.0 - span;
                }
            }
            else
            {
                // Inverted span (end < start): preserve signed distance.
                if ( end < 0.0 )
                {
                    end = 0.0;
                    start = -span;
                }
                if ( start > 1.0 )
                {
                    start = 1.0;
                    end = 1.0 + span;
                }
            }

            axis->SetPosition(start, end);
            m_plot->UpdatePlot();

            SetStatusText(wxString::Format(
                "Dragging vertical axis \"%s\"  position [%.3f, %.3f]  span=%.3f",
                m_dragAxisName, start, end, end - start));
        }

        event.Skip(false);
        return;
    }

    // Hover: hit-test feedback + cursor for drag target / series inspect.
    UpdatePlotCursor(event.GetPosition());
    UpdateHitStatus(m_plot->HitTest(event.GetPosition()), false);
    event.Skip();
}
