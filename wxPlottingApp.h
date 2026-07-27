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
class Series;

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
 *          and provides menus to edit demo objects @c Analog1 and @c Series1.
 */
class wxPlottingFrame : public wxFrame
{
public:
    /**
     * @brief Construct the demo frame and populate a sample plot.
     */
    wxPlottingFrame();

private:
    // --- Menu command IDs (demo editors for Analog1 / Series1) -------------
    enum
    {
        // Axis Analog1
        ID_AXIS_VISIBLE = wxID_HIGHEST + 100,
        ID_AXIS_WIDTH,
        ID_AXIS_FOREGROUND,
        ID_AXIS_LOCATION,
        ID_AXIS_POS_START,
        ID_AXIS_POS_END,
        ID_AXIS_RANGE_START,
        ID_AXIS_RANGE_END,
        ID_AXIS_SCALE,
        ID_AXIS_TICK_VISIBLE,
        ID_AXIS_TICK_COLOR,
        ID_AXIS_TICK_SIDE_BELOW,
        ID_AXIS_TICK_SIDE_ABOVE,
        ID_AXIS_TICK_RANGE,
        ID_AXIS_TICK_FONT,
        ID_AXIS_TICK_LABEL_VISIBLE,
        ID_AXIS_TICK_LABEL_FORMAT,
        ID_AXIS_LABEL,
        ID_AXIS_LABEL_VISIBLE,
        ID_AXIS_LABEL_COLOR,
        ID_AXIS_LABEL_SIDE_BELOW,
        ID_AXIS_LABEL_SIDE_ABOVE,
        ID_AXIS_LABEL_FONT,
        ID_AXIS_LABEL_ROTATED,
        ID_AXIS_GRID_VISIBLE,
        ID_AXIS_GRID_COLOR,
        ID_AXIS_GRID_WIDTH,
        ID_AXIS_GRID_STYLE_SOLID,
        ID_AXIS_GRID_STYLE_DOT,
        ID_AXIS_GRID_STYLE_LONG_DASH,
        ID_AXIS_GRID_STYLE_SHORT_DASH,
        ID_AXIS_GRID_STYLE_DOT_DASH,
        ID_AXIS_GRID_STYLE_TRANSPARENT,

        // Series Series1
        ID_SERIES_VISIBLE,
        ID_SERIES_FOREGROUND,
        ID_SERIES_STYLE_LINE,
        ID_SERIES_STYLE_STAIR,
        ID_SERIES_HORZ_AXIS,
        ID_SERIES_VERT_AXIS,

        // Measurement cursors
        ID_CURSOR_OFF,
        ID_CURSOR_HORIZONTAL,
        ID_CURSOR_VERTICAL,
        ID_CURSOR_COMBINED,

        // Axis auto-arrange
        ID_ARRANGE_VERTICAL,
        ID_ARRANGE_HORIZONTAL
    };

    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

    void OnPlotMotion(wxMouseEvent& event);
    void OnPlotLeftDown(wxMouseEvent& event);
    void OnPlotLeftUp(wxMouseEvent& event);
    void OnPlotCaptureLost(wxMouseCaptureLostEvent& event);
    void OnPlotSetCursor(wxSetCursorEvent& event);
    void OnPlotLeaveWindow(wxMouseEvent& event);
    void OnPlotMouseWheel(wxMouseEvent& event);

    void EndAxisDrag();
    static bool IsWheelZoomAxis(const wxString& name);
    static void ZoomAxisRange(Axis& axis, bool zoomIn);
    bool ClientToPlotY(const wxPoint& clientPos, int& canvasY, int& plotHeight) const;
    wxString VerticalAxisFromHit(const PlotHitResult& hit) const;
    wxCursor CursorForHit(const PlotHitResult& hit) const;
    void UpdatePlotCursor(const wxPoint& clientPos);
    void UpdateHitStatus(const PlotHitResult& hit, bool dragging);

    /** Build &Axis and &Series menus for Analog1 / Series1. */
    void BuildPropertyMenus(wxMenuBar* menuBar);

    /** Build &Cursor menu (measurement cursor modes). */
    void BuildCursorMenu(wxMenuBar* menuBar);

    /** Build &Arrange menu (auto-layout axis bands). */
    void BuildArrangeMenu(wxMenuBar* menuBar);

    void OnArrangeMenu(wxCommandEvent& event);

    /** Resolve demo objects (null if missing). */
    Axis*   DemoAxis() const;
    Series* DemoSeries() const;

    void RefreshPlot();

    // Axis menu handlers
    void OnAxisMenu(wxCommandEvent& event);
    void OnAxisUpdateUI(wxUpdateUIEvent& event);

    // Series menu handlers
    void OnSeriesMenu(wxCommandEvent& event);
    void OnSeriesUpdateUI(wxUpdateUIEvent& event);

    // Measurement cursor menu
    void OnCursorMenu(wxCommandEvent& event);
    void OnCursorUpdateUI(wxUpdateUIEvent& event);

    // Property editors (return true if the value changed)
    bool EditInt(const wxString& title, int& value, int minVal, int maxVal);
    bool EditDouble(const wxString& title, double& value);
    bool EditString(const wxString& title, wxString& value);
    bool EditColour(const wxString& title, wxColour& value);
    bool EditFont(const wxString& title, wxFont& value);

    wxPlotting* m_plot;

    bool     m_draggingAxis = false;
    wxString m_dragAxisName;
    double   m_dragOrigStart = 0.0;
    double   m_dragOrigEnd = 0.0;
    int      m_dragStartCanvasY = 0;

    wxDECLARE_EVENT_TABLE();
};

#endif // WXPLOTTINGAPP_H
