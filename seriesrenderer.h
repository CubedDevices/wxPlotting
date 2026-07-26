/////////////////////////////////////////////////////////////////////////////
// Name:        seriesrenderer.h
// Purpose:     Short-lived series stroke renderers for wxPlotting
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef SERIESRENDERER_H
#define SERIESRENDERER_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/dc.h"
    #include "wx/gdicmn.h"
#endif

#include <memory>

#include "plotcoords.h"

class Series;

/**
 * @file seriesrenderer.h
 * @brief Per-series stroke renderers with a Begin / feed / End lifecycle.
 */

/**
 * @enum SeriesRenderStyle
 * @brief How consecutive samples are stroked when drawing a @ref Series.
 */
enum class SeriesRenderStyle
{
    /**
     * @brief Straight segments between consecutive samples (polyline).
     */
    Line,

    /**
     * @brief Sample-and-hold stairs: horizontal then vertical between samples.
     * @details From the previous point @c (x0,y0) to the next @c (x1,y1)
     *          in axis-converted space: @c (x0,y0)→(x1,y0) then @c (x1,y0)→(x1,y1).
     */
    StairStep
};

/**
 * @class SeriesRenderer
 * @brief Abstract stroke builder used for the duration of one series draw.
 *
 * @details
 * Lifetime is intentionally short: create for one series, call @ref Begin,
 * feed @ref Axis::ConvertValue results with @ref AddPoint (and @ref Break on
 * gaps), call @ref End, then destroy.
 *
 * @ref Begin receives the plot rectangle (@c m_plot). The renderer maps
 * axis-converted coordinates to screen pixels via @ref ToScreen.
 *
 * **Coordinate hand-off**
 * - Caller runs @c horz.ConvertValue(data.x) and @c vert.ConvertValue(data.y)
 * - Caller passes @c wxRealPoint(xConv, yConv) to @ref AddPoint
 * - Renderer converts to device/logical pixels using the plot rect from Begin
 */
class SeriesRenderer
{
public:
    virtual ~SeriesRenderer() = default;

    /**
     * @brief Start a stroke for @a series on @a dc.
     * @param dc Destination device context (typically a @c wxMemoryDC).
     * @param series Series being drawn (colour and other stroke attributes).
     * @param plot Plot data rectangle in the same coordinate system as the DC
     *        (the control's @c m_plot: origin and size of the plotting area).
     */
    virtual void Begin(wxDC& dc, const Series& series, const wxRect& plot) = 0;

    /**
     * @brief Accept the next in-range sample in axis-converted coordinates.
     * @param plotPt @c x = horizontal @ref Axis::ConvertValue result,
     *               @c y = vertical @ref Axis::ConvertValue result
     *               (normalized plot space along each axis, not raw data units).
     */
    virtual void AddPoint(const wxRealPoint& plotPt) = 0;

    /**
     * @brief Mark a discontinuity (e.g. sample left the visible axis range).
     * @details The next @ref AddPoint starts a new segment and must not connect
     *          to the point before the break.
     */
    virtual void Break() = 0;

    /**
     * @brief Finish the series stroke (flush any pending state).
     */
    virtual void End() = 0;

protected:
    /**
     * @brief Map axis-converted coordinates to screen/logical DC pixels.
     * @param plotPt Result of horizontal/vertical @ref Axis::ConvertValue.
     * @return Pixel position within the DC (includes @ref m_plot origin).
     * @details Y is inverted so increasing data/plot Y goes up the screen.
     */
    wxPoint ToScreen(const wxRealPoint& plotPt) const
    {
        return PlotToScreen(m_plot, plotPt);
    }

    wxDC*  m_dc = nullptr;   ///< Valid between @ref Begin and @ref End.
    wxRect m_plot;           ///< Plot area from @ref Begin (screen/logical space).
};

/**
 * @class SeriesLineRenderer
 * @brief Connects successive points with @c wxDC::DrawLine.
 */
class SeriesLineRenderer : public SeriesRenderer
{
public:
    void Begin(wxDC& dc, const Series& series, const wxRect& plot) override;
    void AddPoint(const wxRealPoint& plotPt) override;
    void Break() override;
    void End() override;

private:
    wxPoint m_lastScreen;
    bool    m_hasLast = false;
};

/**
 * @class SeriesStairStepRenderer
 * @brief Draws sample-and-hold stairs (horizontal segment, then vertical).
 */
class SeriesStairStepRenderer : public SeriesRenderer
{
public:
    void Begin(wxDC& dc, const Series& series, const wxRect& plot) override;
    void AddPoint(const wxRealPoint& plotPt) override;
    void Break() override;
    void End() override;

private:
    wxPoint m_lastScreen;
    bool    m_hasLast = false;
};

/**
 * @brief Create a renderer for the given style.
 * @param style Desired stroke style.
 * @return New renderer; caller owns the lifetime (destroy after @ref SeriesRenderer::End).
 * @note Intended for stack/@c unique_ptr use around a single series draw pass.
 */
std::unique_ptr<SeriesRenderer> CreateSeriesRenderer(SeriesRenderStyle style);

#endif // SERIESRENDERER_H
