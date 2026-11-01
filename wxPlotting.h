/////////////////////////////////////////////////////////////////////////////
// Name:        wxPlotting.h
// Purpose:     Owner-drawn plotting control (MemoryDC double-buffer template)
// Copyright:   Kenneth S. Davis, 2026
// Licence:     GNU General Public License version 2
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2026 Kenneth S. Davis
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
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

    /**
     * @brief Auto-arrange visible vertical axes so their position spans do not overlap.
     * @param spacing Normalized gap between adjacent axes (default @c 0.05).
     * @param topInset Normalized gap left below the top of the plot (default 0).
     *        Keeps a full-scale trace off the top border. Bands are scaled
     *        into @c [0, 1 - topInset].
     * @details Distributes bands from top to bottom in registration order:
     *          the first vertical axis gets the topmost band, the next below
     *          it, and so on. Each band has equal height; @a spacing is
     *          reserved between bands. Does not repaint — call @ref UpdatePlot
     *          afterward.
     * @note Only @ref Axis::Visible axes of type @ref Axis::Vertical are
     *       rearranged. Hidden axes are left unchanged.
     * @warning Must be called on the UI thread.
     * @see ArrangeHorizontalAxes, Axis::SetPosition
     */
    void ArrangeVerticalAxes(double spacing = 0.05, double topInset = 0.0);

    /**
     * @brief Auto-arrange visible horizontal axes so their position spans do not overlap.
     * @param spacing Normalized gap between adjacent axes (default @c 0.05).
     * @details Distributes bands from left to right in registration order:
     *          the first horizontal axis gets the leftmost band, then the next
     *          to its right, and so on. Each band has equal width; @a spacing
     *          is reserved between bands. Does not repaint — call
     *          @ref UpdatePlot afterward.
     * @note Only @ref Axis::Visible axes of type @ref Axis::Horizontal are
     *       rearranged. Hidden axes are left unchanged.
     * @warning Must be called on the UI thread.
     * @see ArrangeVerticalAxes, Axis::SetPosition
     */
    void ArrangeHorizontalAxes(double spacing = 0.05);

    /**
     * @brief Render the plot into @a out at an exact pixel size (no DPI scaling).
     * @param out Receives a new 32-bit bitmap of @a size; previous contents discarded.
     * @param size Logical and physical pixel size (width × height). Must exceed margins.
     * @param background Clear colour before drawing. Use @c wxTransparentColour or
     *        a colour with alpha 0 for a fully transparent clear; non-zero alpha is
     *        written into the bitmap (host composites as needed). RGB is used when
     *        alpha is opaque.
     * @return @c true if @a out is valid and drawing ran; @c false on bad size or
     *         bitmap creation failure.
     * @details Does not change the on-screen control buffer or client size.
     *          Measurement cursors and the measure overlay are not drawn
     *          (window overlay only).
     *          Geometry uses the same margins and axis layout as the interactive
     *          control. The host should pass the final pixel dimensions (e.g.
     *          video frame size); this API does not scale or supersample.
     * @warning Call on the UI thread.
     * @note Drawn content (axes, series, labels) is typically fully opaque;
     *       transparency applies to cleared background areas.
     */
    bool RenderToBitmap(wxBitmap& out,
                        const wxSize& size,
                        const wxColour& background = wxTransparentColour);

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

    /**
     * @brief Show a two-point delta measurement on the overlay.
     * @param horizontalAxis Name of the axis that maps @a x0 and @a x1.
     * @param verticalAxis Name of the axis that maps @a y0 and @a y1.
     * @param x0 Anchor value in horizontal-axis sample units.
     * @param y0 Anchor value in vertical-axis sample units.
     * @param x1 Drag value in horizontal-axis sample units.
     * @param y1 Drag value in vertical-axis sample units.
     * @details Draws a horizontal and a vertical double arrow between the
     *          points, with each distance at the arrow midpoint. Distances
     *          use @ref Axis::FormatTickValue (same units as cursor readouts).
     *          Values stay in sample units, so pan and zoom keep the measure
     *          attached to the data. Does not rebuild the plot buffer.
     * @note Call on the UI thread.
     * @see ClearMeasureOverlay, ClientToAxisValue
     */
    void SetMeasureOverlay(const wxString& horizontalAxis,
                           const wxString& verticalAxis,
                           double x0, double y0,
                           double x1, double y1);

    /**
     * @brief Hide the measure overlay.
     * @see SetMeasureOverlay
     */
    void ClearMeasureOverlay();

    /**
     * @brief True while a measure overlay is visible.
     */
    bool HasMeasureOverlay() const { return m_measureVisible; }

    /**
     * @brief Map a client point into one axis's sample units.
     * @param clientPos Position relative to this window's client area.
     * @param axis Axis to convert through (horizontal uses X, vertical uses Y).
     * @param value Receives the sample-unit value (same space as @ref Axis::SetRange).
     * @return @c false if the plot rectangle is empty or @a axis has no orientation.
     * @details The point is clamped to the plot rectangle first, then converted
     *          with the same normalization as the measurement cursors. Pass the
     *          result through @ref Axis::FormatTickValue for a readout.
     */
    bool ClientToAxisValue(const wxPoint& clientPos,
                           const Axis& axis,
                           double& value) const;

protected:
    /**
     * @brief Draw the full plot into @a dc (axes, series; no buffer clear).
     * @param dc Memory DC with the plot bitmap selected.
     * @param size Logical size of the drawing surface (including margins).
     * @details Does not clear the DC; caller clears (see @ref Render,
     *          @ref RenderToBitmap). Override to customize drawing.
     */
    virtual void DrawPlot(wxMemoryDC& dc, const wxSize& size);

    /**
     * @brief Paint handler: blits the buffer with scroll offset applied.
     */
    void OnPaint(wxPaintEvent& event);

    /**
     * @brief Size handler: grows/shrinks the canvas.
     * @details A cheap last render is rebuilt immediately. After an expensive
     *          render (a large capture), live resize only stretches the
     *          previous frame and rebuilds once the size settles. Windows
     *          size-drag is modal and waits for each paint; a full series
     *          redraw on every step is what makes the frame lag.
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
     * @details Also receives the resize-settle timer; that id is dispatched
     *          to @ref OnResizeSettleTimer.
     */
    void OnUpdateThrottleTimer(wxTimerEvent& event);

    /**
     * @brief Rebuild the plot after a live resize has paused.
     */
    void OnResizeSettleTimer();

    /**
     * @brief Update @c m_plot from @c m_drawingSize without touching the bitmap.
     */
    void UpdatePlotRect();

    /**
     * @brief Blit @c m_buffer. Stretches it when a resize rebuild is still pending.
     */
    void PaintBuffer(wxDC& dc);

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
     * @brief Draw the two-point measure overlay on the paint DC.
     * @param dc Paint DC already prepared for scroll.
     * @see SetMeasureOverlay
     */
    void DrawMeasure(wxDC& dc);

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

    /**
     * @brief Shared implementation for @ref ArrangeVerticalAxes / @ref ArrangeHorizontalAxes.
     * @param type Axis orientation to arrange.
     * @param spacing Normalized gap between bands.
     * @param topToBottomIfVertical When @c true and @a type is vertical, first
     *        axis is placed at the top; horizontal always goes left-to-right.
     */
    void ArrangeAxesOfType(Axis::AxisType type, double spacing, bool topToBottom,
                           double topInset = 0.0);

private:
    std::list<Axis> m_axisList;
    std::list<Series> m_seriesList;

    wxBitmap m_buffer;
    wxSize   m_plotSize;
    wxRect   m_plot;

    int m_marginLeft   = 80;
    int m_marginRight  = 40;
    int m_marginTop    = 40;
    int m_marginBottom = 80;

    wxSize m_minDrawingSize;
    wxSize m_drawingSize;

    // Measurement cursor overlay (drawn on paint DC, not baked into m_buffer).
    PlotCursorMode m_cursorMode = PlotCursorMode::Off;
    bool           m_cursorTracking = false; ///< True when pointer is over the control.
    wxPoint        m_cursorCanvasPos;        ///< Last mouse position in canvas coords.

    // Two-point measure overlay (also paint-DC only). Sample units, not pixels,
    // so the arrows track pan/zoom. See SetMeasureOverlay.
    bool     m_measureVisible = false;
    wxString m_measureHorzAxis;
    wxString m_measureVertAxis;
    double   m_measureX0 = 0.0;
    double   m_measureY0 = 0.0;
    double   m_measureX1 = 0.0;
    double   m_measureY1 = 0.0;

    // --- Paint rate limiting (~60 Hz) --------------------------------------
    // High-frequency mouse motion (especially on Windows) can request more
    // full-buffer rebuilds than the UI can sustain. Coalesce to this interval
    // and always flush a trailing update so the final state is visible.
    static const int kMaxUpdateHz = 60;
    wxTimer    m_updateThrottleTimer;
    wxLongLong m_lastFlushMs = 0;       ///< Time of last FlushPendingUpdate.
    bool       m_fullUpdatePending = false;    ///< Need Render() before paint.
    bool       m_overlayRefreshPending = false;///< Need Refresh only (cursors).

    // Live resize. See OnSize. Budget is the last full Render(); above it we
    // stretch the previous bitmap until size events pause.
    static const int kLiveResizeBudgetMs = 10;
    static const int kResizeSettleMs = 50;
    wxTimer m_resizeSettleTimer;
    long    m_lastRenderMs = 0;
    bool    m_resizeRenderPending = false;
    wxSize  m_renderedSize; ///< Drawing size m_buffer was last rendered at.

    wxDECLARE_DYNAMIC_CLASS(wxPlotting);
    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(wxPlotting);
};

#endif // WXPLOTTING_H

