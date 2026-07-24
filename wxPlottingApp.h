/////////////////////////////////////////////////////////////////////////////
// Name:        wxPlottingApp.h
// Purpose:     Test application hosting a wxPlotting control
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef WXPLOTTINGAPP_H
#define WXPLOTTINGAPP_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/frame.h"
#endif

class wxPlotting;

// ----------------------------------------------------------------------------
// wxPlottingApp
// ----------------------------------------------------------------------------

class wxPlottingApp : public wxApp
{
public:
    bool OnInit() override;
};

wxDECLARE_APP(wxPlottingApp);

// ----------------------------------------------------------------------------
// wxPlottingFrame - main window for exercising the control
// ----------------------------------------------------------------------------

class wxPlottingFrame : public wxFrame
{
public:
    wxPlottingFrame();

private:
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

    wxPlotting* m_plot;

    wxDECLARE_EVENT_TABLE();
};

#endif // WXPLOTTINGAPP_H
