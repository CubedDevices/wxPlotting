/////////////////////////////////////////////////////////////////////////////
// Name:        seriesrenderer.h
// Purpose:     Short-lived series stroke renderers for wxPlotting
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
#ifndef SERIESRENDERER_H
#define SERIESRENDERER_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/dc.h"
    #include "wx/gdicmn.h"
#endif

#include <memory>
#include <vector>

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
    StairStep,

    /**
     * @brief Filled area under the curve down to the vertical axis band baseline.
     * @details Collects each continuous segment, then fills the polygon from the
     *          polyline down to the lower edge of the bound vertical axis span.
     *          The curve outline is stroked with the series foreground colour.
     */
    Fill
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

    /**
     * @brief Optional baseline in normalized plot Y for fill styles.
     * @param plotNormY Lower edge of the vertical axis band (typically
     *        @c min(axis.GetStart(), axis.GetEnd())).
     * @details Default is a no-op. @ref SeriesFillRenderer uses this as the
     *          bottom of the filled area. Call after @ref Begin from the plot.
     */
    virtual void SetBaselinePlotY(double plotNormY)
    {
        (void)plotNormY;
    }

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
 * @class SeriesFillRenderer
 * @brief Fills the area under each continuous polyline segment to a baseline.
 *
 * @details Points are buffered per segment. On @ref Break / @ref End the
 *          polygon is closed down to @ref SetBaselinePlotY (default 0 in plot
 *          space, i.e. bottom of the full plot when the baseline is not set).
 *          Fill uses the series foreground as brush; the curve is stroked on top.
 */
class SeriesFillRenderer : public SeriesRenderer
{
public:
    void Begin(wxDC& dc, const Series& series, const wxRect& plot) override;
    void AddPoint(const wxRealPoint& plotPt) override;
    void Break() override;
    void End() override;
    void SetBaselinePlotY(double plotNormY) override;

private:
    void FlushSegment();

    wxColour m_colour;
    double   m_baselinePlotY = 0.0;
    std::vector<wxPoint> m_segment;
};

/**
 * @brief Create a renderer for the given style.
 * @param style Desired stroke style.
 * @return New renderer; caller owns the lifetime (destroy after @ref SeriesRenderer::End).
 * @note Intended for stack/@c unique_ptr use around a single series draw pass.
 */
std::unique_ptr<SeriesRenderer> CreateSeriesRenderer(SeriesRenderStyle style);

#endif // SERIESRENDERER_H

