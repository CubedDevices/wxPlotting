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

/**
 * @file wxPlottingApp.h
 * @brief Demo host application for @ref wxPlotting.
 */

/**
 * @class wxPlottingApp
 * @brief wxWidgets application entry for the plotting test harness.
 */
class wxPlottingApp : public wxApp
{
public:
    /**
     * @brief Application startup: create and show the main frame.
     * @return @c true to continue running, @c false to exit.
     */
    bool OnInit() override;
};

wxDECLARE_APP(wxPlottingApp);

/**
 * @class wxPlottingFrame
 * @brief Main window that embeds a @ref wxPlotting control for interactive testing.
 *
 * @details Constructs sample axes and series, hosts the control in a sizer,
 *          and provides Exit / About commands.
 */
class wxPlottingFrame : public wxFrame
{
public:
    /**
     * @brief Construct the demo frame and populate a sample plot.
     */
    wxPlottingFrame();

private:
    /**
     * @brief File → Exit handler.
     */
    void OnExit(wxCommandEvent& event);

    /**
     * @brief Help → About handler.
     */
    void OnAbout(wxCommandEvent& event);

    wxPlotting* m_plot;  ///< Owned plot control (child of this frame).

    wxDECLARE_EVENT_TABLE();
};

#endif // WXPLOTTINGAPP_H
