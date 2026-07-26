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

#include "plothit.h"
#include "axis.h"

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
 *          demonstrates hit-testing and vertical-axis band dragging.
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

    /**
     * @brief Mouse move: status bar hit-test and vertical-axis drag updates.
     */
    void OnPlotMotion(wxMouseEvent& event);

    /**
     * @brief Left button down: begin vertical-axis drag if hit-test finds one.
     */
    void OnPlotLeftDown(wxMouseEvent& event);

    /**
     * @brief Left button up: end axis drag.
     */
    void OnPlotLeftUp(wxMouseEvent& event);

    /**
     * @brief Capture lost: end axis drag without a matching mouse-up.
     */
    void OnPlotCaptureLost(wxMouseCaptureLostEvent& event);

    /**
     * @brief Set cursor for the current interaction (Windows-friendly path).
     * @details Prefer this over only @c SetCursor on motion; OS resets cursors often.
     */
    void OnPlotSetCursor(wxSetCursorEvent& event);

    /**
     * @brief Mouse left the plot: restore the standard cursor.
     */
    void OnPlotLeaveWindow(wxMouseEvent& event);

    /**
     * @brief Mouse wheel: zoom Analog1/Analog2 data range by 25% when hit.
     */
    void OnPlotMouseWheel(wxMouseEvent& event);

    /**
     * @brief Stop an in-progress axis drag and release mouse capture if held.
     */
    void EndAxisDrag();

    /**
     * @brief True if @a name is a demo axis that supports wheel range zoom.
     */
    static bool IsWheelZoomAxis(const wxString& name);

    /**
     * @brief Scale an axis data range by 25% (zoom in or out), keeping the center fixed.
     * @param axis Axis to update.
     * @param zoomIn If @c true, span becomes 75% of current; if @c false, 125%.
     */
    static void ZoomAxisRange(Axis& axis, bool zoomIn);

    /**
     * @brief Client position → unscrolled canvas Y and plot height for drag math.
     */
    bool ClientToPlotY(const wxPoint& clientPos, int& canvasY, int& plotHeight) const;

    /**
     * @brief Pick the first vertical axis name listed in a hit result (if any).
     */
    wxString VerticalAxisFromHit(const PlotHitResult& hit) const;

    /**
     * @brief Cursor appropriate for @a hit and current drag state.
     */
    wxCursor CursorForHit(const PlotHitResult& hit) const;

    /**
     * @brief Apply the correct cursor for the client position (and drag state).
     */
    void UpdatePlotCursor(const wxPoint& clientPos);

    /**
     * @brief Refresh status text from a hit-test result (and optional drag note).
     */
    void UpdateHitStatus(const PlotHitResult& hit, bool dragging);

    wxPlotting* m_plot;  ///< Owned plot control (child of this frame).

    // Vertical-axis drag state (demo only; horizontal axes are not dragged).
    bool     m_draggingAxis = false;
    wxString m_dragAxisName;
    double   m_dragOrigStart = 0.0;
    double   m_dragOrigEnd = 0.0;
    int      m_dragStartCanvasY = 0;

    wxDECLARE_EVENT_TABLE();
};

#endif // WXPLOTTINGAPP_H
