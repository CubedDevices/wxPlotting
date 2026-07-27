/////////////////////////////////////////////////////////////////////////////
// Name:        wxPlotting.h
// Purpose:     Owner-drawn plotting control (MemoryDC double-buffer template)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef WXPLOTTING_H
#define WXPLOTTING_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/scrolwin.h"
    #include "wx/bitmap.h"
    #include "wx/dcmemory.h"
    #include "wx/timer.h"
#endif

#include <list>
#include "axis.h"
#include "series.h"
#include "plothit.h"

// turn on Series hit-testing.  slows down considerably
// comment out to disable
//#define WXPLOTTING_SERIES_HITTESTING

/**
 * @class wxPlotting
 * @brief Owner-drawn plotting control with double-buffered MemoryDC rendering.
 *
 * @details
 * wxPlotting is a wxScrolledWindow that:
 * - Renders axes and series into an off-screen @c wxBitmap via @c wxMemoryDC
 * - Uses a fixed minimum canvas size; the canvas grows with the client area
 * - Shows scrollbars only when the client is smaller than that minimum
 * - Supports named axes and series with unique name checks
 *
 * **Threading**
 * - Call configuration methods (@ref AddAxis, @ref SetMargins, etc.) on the UI thread.
 * - Series sample data may be updated from worker threads via @ref Series
 *   point APIs; schedule @ref UpdatePlot on the UI thread afterward
 *   (e.g. @c CallAfter).
 *
 * **Typical use**
 * @code
 * auto* plot = new wxPlotting(parent);
 * Axis time("Time", Axis::Horizontal);
 * // configure time...
 * plot->AddAxis(time);
 * Series s("Ch1", Series::Doubles);
 * s.HorizontalAxis = "Time";
 * s.VerticalAxis = "Signal1";
 * plot->AddSeries(s);
 * plot->UpdatePlot();
 * @endcode
 */
class wxPlotting : public wxScrolledWindow
{
public:
    /**
     * @brief Default minimum drawing canvas size when none is set by the host.
     * @details Effective size is always at least this large on each axis;
     *          the canvas may grow larger with the client area.
     */
    static const wxSize DefaultMinDrawingSize;

    /**
     * @brief Default constructor for two-step creation.
     * @details Call @ref Create before showing or using the control.
     */
    wxPlotting();

    /**
     * @brief Construct and create the control in one step.
     * @param parent Parent window; must not be null for a usable control.
     * @param id Window identifier.
     * @param pos Initial position.
     * @param size Initial size (client area of the control).
     * @param style Extra window styles (scroll styles are added automatically).
     * @param name Window name used by wxWidgets.
     */
    wxPlotting(wxWindow* parent,
               wxWindowID id = wxID_ANY,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0,
               const wxString& name = wxASCII_STR("wxPlotting"));

    /**
     * @brief Two-step create (use after the default constructor).
     * @param parent Parent window.
     * @param id Window identifier.
     * @param pos Initial position.
     * @param size Initial size.
     * @param style Extra window styles.
     * @param name Window name.
     * @return @c true on success, @c false if window creation failed.
     */
    bool Create(wxWindow* parent,
                wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = wxASCII_STR("wxPlotting"));

    /**
     * @brief Destructor.
     */
    virtual ~wxPlotting();

    // --- Axis / series collections -----------------------------------------

    /**
     * @brief Add an axis to the plot.
     * @param axis Axis definition to store (copied). Name must be unique.
     * @return @c true if added; @c false if the name is empty or already used.
     * @note Does not repaint; call @ref UpdatePlot after batching changes.
     * @see GetAxis, RemoveAxis, ClearAxes
     */
    bool AddAxis(const Axis& axis);

    /**
     * @brief Add a data series to the plot.
     * @param series Series definition to store (copied). Name must be unique.
     * @return @c true if added; @c false if the name is empty or already used.
     * @note @c HorizontalAxis / @c VerticalAxis should match existing axis names.
     * @note Does not repaint; call @ref UpdatePlot after batching changes.
     * @see GetSeries, RemoveSeries, ClearSeries
     */
    bool AddSeries(const Series& series);

    /**
     * @brief Remove an axis by name.
     * @param name Unique axis name.
     * @return @c true if removed; @c false if not found or name empty.
     * @warning Any pointer previously returned by @ref GetAxis for this name
     *          becomes invalid.
     */
    bool RemoveAxis(const wxString& name);

    /**
     * @brief Remove a series by name.
     * @param name Unique series name.
     * @return @c true if removed; @c false if not found or name empty.
     * @warning Any pointer previously returned by @ref GetSeries for this name
     *          becomes invalid.
     */
    bool RemoveSeries(const wxString& name);

    /**
     * @brief Remove all axes.
     * @warning Invalidates all pointers previously returned by @ref GetAxis.
     */
    void ClearAxes();

    /**
     * @brief Remove all series.
     * @warning Invalidates all pointers previously returned by @ref GetSeries.
     */
    void ClearSeries();

    /**
     * @brief Remove all axes and series.
     * @see ClearAxes, ClearSeries
     */
    void Clear();

    /**
     * @brief Find a mutable axis by name.
     * @param name Unique axis name.
     * @return Pointer to the stored axis, or @c nullptr if not found.
     * @note The pointer remains valid until that axis is removed or cleared.
     * @warning Mutate on the UI thread only (not thread-safe).
     */
    Axis* GetAxis(const wxString& name);

    /**
     * @brief Find a read-only axis by name.
     * @param name Unique axis name.
     * @return Pointer to the stored axis, or @c nullptr if not found.
     */
    const Axis* GetAxis(const wxString& name) const;

    /**
     * @brief Find a mutable series by name.
     * @param name Unique series name.
     * @return Pointer to the stored series, or @c nullptr if not found.
     * @note Point data is thread-safe via @ref Series accessors; other fields
     *       should be changed on the UI thread.
     * @note After point updates from a worker, call @ref UpdatePlot on the UI
     *       thread (e.g. via @c CallAfter).
     */
    Series* GetSeries(const wxString& name);

    /**
     * @brief Find a read-only series by name.
     * @param name Unique series name.
     * @return Pointer to the stored series, or @c nullptr if not found.
     */
    const Series* GetSeries(const wxString& name) const;

    /**
     * @brief Rebuild the off-screen plot and request a repaint.
     * @param forceImmediate If @c false (default), full buffer rebuilds are
     *        rate-limited to about 60 Hz so high-frequency callers (axis drag,
     *        streaming data) cannot flood the UI thread. A trailing update is
     *        always scheduled so the last state is painted. If @c true, rebuild
     *        and paint immediately (also calls @c Update()).
     * @warning Must be called on the UI thread.
     */
    void UpdatePlot(bool forceImmediate = false);

    // --- Margins -----------------------------------------------------------

    /**
     * @brief Set all four plot margins to the same value (pixels).
     * @param margin Margin applied to left, right, top, and bottom.
     * @see SetMargins
     */
    void SetMargin(int margin);

    /**
     * @brief Set independent plot margins in pixels.
     * @param left Space left of the plot rectangle (titles/ticks/labels).
     * @param right Space right of the plot rectangle.
     * @param top Space above the plot rectangle.
     * @param bottom Space below the plot rectangle.
     * @details Negative values are clamped to zero. Triggers a redraw.
     */
    void SetMargins(int left, int right, int top, int bottom);

    /**
     * @brief Set the left margin in pixels.
     * @param margin New left margin (>= 0).
     */
    void SetMarginLeft(int margin);

    /**
     * @brief Set the right margin in pixels.
     * @param margin New right margin (>= 0).
     */
    void SetMarginRight(int margin);

    /**
     * @brief Set the top margin in pixels.
     * @param margin New top margin (>= 0).
     */
    void SetMarginTop(int margin);

    /**
     * @brief Set the bottom margin in pixels.
     * @param margin New bottom margin (>= 0).
     */
    void SetMarginBottom(int margin);

    /**
     * @brief Get the left margin in pixels.
     */
    int GetMarginLeft() const   { return m_marginLeft; }

    /**
     * @brief Get the right margin in pixels.
     */
    int GetMarginRight() const  { return m_marginRight; }

    /**
     * @brief Get the top margin in pixels.
     */
    int GetMarginTop() const    { return m_marginTop; }

    /**
     * @brief Get the bottom margin in pixels.
     */
    int GetMarginBottom() const { return m_marginBottom; }

    /**
     * @brief Get the left margin (legacy single-value API).
     * @return Same as @ref GetMarginLeft.
     * @deprecated Prefer @ref GetMarginLeft for clarity.
     */
    int GetMargin() const       { return m_marginLeft; }

    // --- Drawing / canvas size ---------------------------------------------

    /**
     * @brief Set the minimum drawing canvas size in pixels.
     * @param size Minimum width and height (must be positive).
     * @details The effective canvas is @c max(client, min) per axis.
     *          Scrollbars appear only when the client is smaller than this
     *          minimum. Raise height as more vertical content is needed.
     */
    void SetMinDrawingSize(const wxSize& size);

    /**
     * @brief Get the configured minimum drawing size.
     */
    wxSize GetMinDrawingSize() const { return m_minDrawingSize; }

    /**
     * @brief Set only the minimum drawing width.
     * @param width Minimum width in pixels.
     */
    void SetMinDrawingWidth(int width);

    /**
     * @brief Set only the minimum drawing height.
     * @param height Minimum height in pixels.
     */
    void SetMinDrawingHeight(int height);

    /**
     * @brief Get the minimum drawing width.
     */
    int GetMinDrawingWidth() const  { return m_minDrawingSize.x; }

    /**
     * @brief Get the minimum drawing height.
     */
    int GetMinDrawingHeight() const { return m_minDrawingSize.y; }

    /**
     * @brief Alias for @ref SetMinDrawingSize.
     * @param size Minimum canvas size.
     */
    void SetDrawingSize(const wxSize& size) { SetMinDrawingSize(size); }

    /**
     * @brief Alias for @ref SetMinDrawingWidth.
     * @param width Minimum width.
     */
    void SetDrawingWidth(int width)         { SetMinDrawingWidth(width); }

    /**
     * @brief Alias for @ref SetMinDrawingHeight.
     * @param height Minimum height.
     */
    void SetDrawingHeight(int height)       { SetMinDrawingHeight(height); }

    /**
     * @brief Get the current effective canvas size (max of client and minimum).
     */
    wxSize GetDrawingSize() const { return m_drawingSize; }

    /**
     * @brief Get the current effective canvas width.
     */
    int GetDrawingWidth() const  { return m_drawingSize.x; }

    /**
     * @brief Get the current effective canvas height.
     */
    int GetDrawingHeight() const { return m_drawingSize.y; }

    /**
     * @brief Access the off-screen bitmap used for double buffering.
     * @return Reference to the plot buffer (may be invalid before first layout).
     */
    const wxBitmap& GetBuffer() const { return m_buffer; }

    // --- Hit-testing (UI interaction) --------------------------------------

    /**
     * @brief Hit-test a point in client coordinates (typical for mouse events).
     * @param clientPos Position relative to this window's client area.
     * @param tolerancePx Max distance in canvas pixels for series stroke hits.
     * @return Hit result; @ref PlotHitResult::axes lists all axes covering the point.
     * @details Converts client → unscrolled canvas coordinates first, matching
     *          scroll handling in @ref OnPaint.
     * @see HitTestCanvas, PlotHitResult
     */
    PlotHitResult HitTest(const wxPoint& clientPos, int tolerancePx = 6) const;

    /**
     * @brief Hit-test a point already in unscrolled canvas/logical coordinates.
     * @param canvasPos Position in the same space as the off-screen buffer / @c m_plot.
     * @param tolerancePx Max distance in canvas pixels for series stroke hits
     *        (axis hits use strip containment to the canvas borders — band plus
     *        margins for ticks/labels — not line distance).
     * @return Hit result. Priority for @ref PlotHitResult::kind: series stroke,
     *         then axis strip(s), then plot area. @ref PlotHitResult::axes always
     *         lists every visible axis whose strip contains the pick
     *         (typically one horizontal and one vertical).
     * @see HitTest, PlotHitResult
     */
    PlotHitResult HitTestCanvas(const wxPoint& canvasPos, int tolerancePx = 6) const;

    // --- Measurement cursors -----------------------------------------------

    /**
     * @brief Enable or change the measurement cursor overlay.
     * @param mode @ref PlotCursorMode::Off disables tracking and hides guides.
     * @details When not @c Off, the control tracks the mouse and draws guide
     *          lines on paint (without re-rendering the full plot buffer).
     * @note Call on the UI thread.
     * @see GetCursorMode, PlotCursorMode
     */
    void SetCursorMode(PlotCursorMode mode);

    /**
     * @brief Get the current measurement cursor mode.
     */
    PlotCursorMode GetCursorMode() const { return m_cursorMode; }

protected:
    /**
     * @brief Draw the full plot into @a dc (background, axes, series).
     * @param dc Memory DC with the plot bitmap selected.
     * @param size Logical size of the drawing surface (including margins).
     * @details Override to customize drawing; call the base if needed.
     */
    virtual void DrawPlot(wxMemoryDC& dc, const wxSize& size);

    /**
     * @brief Paint handler: blits the buffer with scroll offset applied.
     */
    void OnPaint(wxPaintEvent& event);

    /**
     * @brief Size handler: grows/shrinks canvas and rebuilds when needed.
     */
    void OnSize(wxSizeEvent& event);

    /**
     * @brief Erase handler: intentionally empty to reduce flicker.
     */
    void OnEraseBackground(wxEraseEvent& event);

    /**
     * @brief Mouse motion: update measurement cursor tracking when enabled.
     */
    void OnMotion(wxMouseEvent& event);

    /**
     * @brief Mouse leave: hide measurement cursor until re-entry.
     */
    void OnLeaveWindow(wxMouseEvent& event);

    /**
     * @brief Trailing-edge timer for rate-limited @ref UpdatePlot / overlay refresh.
     */
    void OnUpdateThrottleTimer(wxTimerEvent& event);

    /**
     * @brief Recreate the off-screen bitmap and update the plot rectangle.
     */
    void RecreateBuffer();

    /**
     * @brief Clear the buffer and call @ref DrawPlot.
     */
    void Render();

    /**
     * @brief Apply pending full rebuild and/or refresh (used by throttle path).
     * @param forceImmediate When @c true, also call @c Update() after @c Refresh.
     */
    void FlushPendingUpdate(bool forceImmediate);

    /**
     * @brief Request a rate-limited overlay-only repaint (no buffer rebuild).
     * @details Used by measurement cursors during mouse motion.
     */
    void RequestOverlayRefresh();

    /**
     * @brief DrawSeries, called from @ref DrawPlot.
     */
    void DrawSeries(wxMemoryDC& dc, const wxSize& size);

    /**
     * @brief Draw measurement cursor overlay on the paint DC (after buffer blit).
     * @param dc Paint DC already prepared for scroll.
     */
    void DrawCursors(wxDC& dc);

    /**
     * @brief Sync effective drawing size from client size and minimum.
     * @return @c true if the effective drawing size changed.
     */
    bool SyncDrawingSizeToClient();

    /**
     * @brief Preferred client size for layout (matches minimum canvas).
     */
    wxSize DoGetBestClientSize() const override;

    /**
     * @brief Find first axis of @a type listed in a hit result.
     */
    const Axis* AxisOfTypeFromHit(const PlotHitResult& hit, Axis::AxisType type) const;

private:
    std::list<Axis> m_axisList;
    std::list<Series> m_seriesList;

    wxBitmap m_buffer;
    wxSize   m_plotSize;
    wxRect   m_plot;

    int m_marginLeft   = 80;
    int m_marginRight  = 40;
    int m_marginTop    = 40;
    int m_marginBottom = 70;

    wxSize m_minDrawingSize;
    wxSize m_drawingSize;

    // Measurement cursor overlay (drawn on paint DC, not baked into m_buffer).
    PlotCursorMode m_cursorMode = PlotCursorMode::Off;
    bool           m_cursorTracking = false; ///< True when pointer is over the control.
    wxPoint        m_cursorCanvasPos;        ///< Last mouse position in canvas coords.

    // --- Paint rate limiting (~60 Hz) --------------------------------------
    // High-frequency mouse motion (especially on Windows) can request more
    // full-buffer rebuilds than the UI can sustain. Coalesce to this interval
    // and always flush a trailing update so the final state is visible.
    static const int kMaxUpdateHz = 60;
    wxTimer    m_updateThrottleTimer;
    wxLongLong m_lastFlushMs = 0;       ///< Time of last FlushPendingUpdate.
    bool       m_fullUpdatePending = false;    ///< Need Render() before paint.
    bool       m_overlayRefreshPending = false;///< Need Refresh only (cursors).

    wxDECLARE_DYNAMIC_CLASS(wxPlotting);
    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(wxPlotting);
};

#endif // WXPLOTTING_H
