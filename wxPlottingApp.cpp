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
    m_plot->SetMinDrawingSize(wxSize(800, 600));
    m_plot->CreateDefaultPlot();
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
