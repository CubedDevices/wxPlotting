#ifndef AXIS_H
#define AXIS_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/string.h"
#include "wx/font.h"
#include "wx/pen.h"
#include "wx/colour.h"
#endif

/**
 * @file axis.h
 * @brief Axis definition used by @ref wxPlotting.
 */

/**
 * @class Axis
 * @brief Describes one horizontal or vertical axis on a plot.
 *
 * @details
 * An axis has:
 * - A unique @ref GetName "name" used by series to bind channels
 * - A data @ref SetRange "range" and normalized @ref SetPosition "position"
 *   along the plot (0–1)
 * - Optional ticks, tick labels, grids, and a title label
 *
 * Configure on the UI thread, then call @ref wxPlotting::UpdatePlot.
 *
 * **Coordinate model**
 * - Series samples and @ref SetRange share the same unit space (e.g. seconds).
 * - @ref Scale multiplies values for display across the whole axis: tick labels,
 *   measurement cursors, and the interpreted meaning of the range
 *   (e.g. Scale = 1000 shows milliseconds while data/range stay in seconds).
 * - Plot placement uses unscaled range and samples (@ref ConvertValue).
 * - @ref SetPosition / @ref SetLocation use normalized plot space [0, 1].
 */
class Axis
{
public:

    /**
     * @enum AxisType
     * @brief Orientation of the axis within the plot.
     */
    enum AxisType
    {
        None,        ///< Unspecified / unused.
        Horizontal,  ///< X-style axis (value increases with plot width).
        Vertical     ///< Y-style axis (value increases with plot height, bottom-up).
    };

    /**
     * @enum LabelSide
     * @brief Which side of the axis line receives titles and tick geometry.
     *
     * | Axis type   | @c BelowOrLeft     | @c AboveOrRight    |
     * |-------------|--------------------|--------------------|
     * | Horizontal  | below the line     | above the line     |
     * | Vertical    | left of the line   | right of the line  |
     */
    enum LabelSide
    {
        BelowOrLeft = 0,  ///< Below (H) or left (V).
        AboveOrRight = 1  ///< Above (H) or right (V).
    };

public:

    /**
     * @brief Construct an axis with a unique name and orientation.
     * @param name Unique name among all axes on a plot (also default title).
     * @param axisType Horizontal or vertical.
     * @details Initializes safe visual defaults (white stroke/text on dark plot).
     */
    Axis(const wxString& name, AxisType axisType)
        : Visible(true),
          Width(1),
          Foreground(*wxWHITE),
          TickVisible(false),
          TickColor(*wxLIGHT_GREY),
          TickSide(BelowOrLeft),
          TickRange(1.0),
          TickFont(wxFontInfo(10).Family(wxFONTFAMILY_MODERN)),
          TickLabelVisible(false),
          TickLabelFormat("%g"),
          Label(name),
          LabelVisible(true),
          LabelColor(*wxWHITE),
          LabelSide(BelowOrLeft),
          LabelFont(wxFontInfo(14).Family(wxFONTFAMILY_MODERN)),
          LabelRotated(false),
          GridVisible(false),
          GridColor(wxColour(80, 80, 80)),
          GridStyle(wxPENSTYLE_SOLID),
          GridWidth(1),
          Scale(1.0),
          m_name(name),
          m_axisType(axisType)
    {
    }

    /**
     * @brief Get the axis orientation.
     * @return @ref Horizontal, @ref Vertical, or @ref None.
     */
    AxisType GetAxisType() const {
        return m_axisType;
    }

    /**
     * @brief Get the unique axis name.
     * @return Name used by @ref wxPlotting::GetAxis and series bindings.
     */
    const wxString& GetName() const {
        return m_name;
    }

    bool     Visible;      ///< When @c false, the axis (and its ticks/grids) are not drawn.

    int      Width;        ///< Axis line pen width in pixels.
    wxColor  Foreground;   ///< Axis line colour.

    bool     TickVisible;  ///< Draw tick marks at @ref TickRange intervals.
    wxColor  TickColor;    ///< Colour of tick marks and tick labels.
    LabelSide TickSide;    ///< Side of the axis line for ticks and tick labels.
    double   TickRange;    ///< Spacing between ticks in data units (must be > 0).

    /**
     * @brief Font for numeric tick labels.
     * @details Point size is logical; the plot MemoryDC uses the window DPI scale.
     */
    wxFont   TickFont;

    bool     TickLabelVisible;  ///< Draw formatted values at each tick (always upright).

    /**
     * @brief printf-style format for tick values (one floating argument).
     * @details Examples: @c "%g", @c "%.0f", @c "%.2f", @c "%+0.1f".
     *          Empty falls back to @c "%g" when formatting via @ref FormatTickValue.
     *          Formatting is applied after @ref Scale (display units).
     */
    wxString TickLabelFormat;

    /**
     * @brief Format an axis value for ticks/cursors (applies @ref Scale first).
     * @param value Value in range / sample units (same space as @ref SetRange).
     * @return Formatted string of @c value * Scale; empty format falls back to @c "%g".
     */
    wxString FormatTickValue(double value) const
    {
        const double display = ApplyScale(value);
        if ( TickLabelFormat.IsEmpty() )
            return wxString::Format(wxT("%g"), display);
        return wxString::Format(TickLabelFormat, display);
    }

    wxString Label;         ///< Axis title text (defaults to the axis name).
    bool     LabelVisible;  ///< Draw the axis title.
    wxColor  LabelColor;    ///< Axis title colour.
    LabelSide LabelSide;    ///< Side of the axis line for the title.
    wxFont   LabelFont;     ///< Axis title font.

    /**
     * @brief When @c true (default), vertical axis titles are drawn rotated 90°.
     * @details Horizontal axis titles are always upright. Set to @c false for
     *          long vertical titles that should remain horizontal (may need a
     *          larger left/right margin).
     */
    bool     LabelRotated;

    bool     GridVisible;   ///< Draw grid lines through tick positions.
    wxColor  GridColor;     ///< Grid line colour.

    /**
     * @brief Grid pen style (any cross-platform @c wxPENSTYLE_* value).
     * @see wxPENSTYLE_SOLID, wxPENSTYLE_DOT, wxPENSTYLE_SHORT_DASH, …
     */
    wxPenStyle GridStyle;

    int      GridWidth;     ///< Grid line width in pixels.

    /**
     * @brief Display multiplier for the whole axis (range, ticks, cursors).
     * @details Does not change plot placement: samples and @ref SetRange stay
     *          in the same unit space. @ref FormatTickValue multiplies by Scale
     *          so labels/cursors show the scaled meaning (e.g. Scale = 1000 for
     *          milliseconds while range remains in seconds). Default is @c 1.0.
     * @note Not mutex-protected; set on the UI thread, then @ref wxPlotting::UpdatePlot.
     */
    double   Scale;

    /**
     * @brief Set the axis extent in normalized plot space.
     * @param start Normalized start along the axis (typically 0.0).
     * @param end Normalized end along the axis (typically 1.0).
     * @details For a vertical axis, this is the vertical span of the axis line
     *          in plot units; for horizontal, the horizontal span.
     */
    void SetPosition(double start, double end)
    {
        m_start = start;
        m_end = end;
    }

    /**
     * @brief Get the normalized start of the axis span.
     */
    double GetStart() const {
        return m_start;
    }

    /**
     * @brief Get the normalized end of the axis span.
     */
    double GetEnd() const {
        return m_end;
    }

    /**
     * @brief Set the value range mapped onto the axis (sample / data units).
     * @param start Value at the axis start (e.g. 0 seconds, -3.3 V).
     * @param end Value at the axis end (e.g. 256 seconds, +3.3 V).
     * @details Same unit space as series samples. @ref Scale only affects
     *          displayed tick/cursor numbers, not this range or plot placement.
     * @warning If @a start equals @a end, @ref ConvertValue returns the
     *          midpoint of the axis span (no divide-by-zero).
     */
    void SetRange(double start, double end)
    {
        m_rangeStart = start;
        m_rangeEnd = end;
    }

    /**
     * @brief Get the range start (sample / data units).
     */
    double GetRangeStart() const {
        return m_rangeStart;
    }

    /**
     * @brief Get the range end (sample / data units).
     */
    double GetRangeEnd() const {
        return m_rangeEnd;
    }

    /**
     * @brief Apply @ref Scale for display (ticks, cursors).
     * @param value Value in range / sample units.
     * @return @c value * Scale.
     */
    double ApplyScale(double value) const
    {
        return value * Scale;
    }

    /**
     * @brief True if the sample lies within [@ref GetRangeStart, @ref GetRangeEnd].
     * @param realValue Series sample (same units as the range; Scale not applied).
     * @details Supports inverted ranges (@c end < @c start).
     */
    bool IsValueInRange(double realValue) const
    {
        if ( m_rangeStart <= m_rangeEnd )
            return realValue >= m_rangeStart && realValue <= m_rangeEnd;
        return realValue >= m_rangeEnd && realValue <= m_rangeStart;
    }

    /**
     * @brief Set where the axis line sits in the perpendicular direction.
     * @param pos Normalized location in [0, 1].
     * @details Horizontal axes: 0 = bottom of plot, 1 = top.
     *          Vertical axes: 0 = left of plot, 1 = right.
     */
    void SetLocation(double pos) {
        m_location = pos;
    }

    /**
     * @brief Get the normalized perpendicular location of the axis line.
     */
    double GetLocation() const {
        return m_location;
    }

    /**
     * @brief Map a range/sample value into normalized plot coordinates.
     * @param rangeValue Value in the same space as @ref SetRange (ticks, grids).
     * @return Position in the axis span [@ref GetStart, @ref GetEnd].
     * @details Does not apply @ref Scale (placement only). Prefer
     *          @ref ConvertValue for series samples (same mapping).
     */
    double ConvertRangeValue(double rangeValue) const
    {
        const double rangeSpan = m_rangeEnd - m_rangeStart;
        if ( rangeSpan == 0.0 )
            return (m_start + m_end) * 0.5;

        const double axisPoint = (rangeValue - m_rangeStart) / rangeSpan;
        return m_start + (axisPoint * (m_end - m_start));
    }

    /**
     * @brief Map a series sample into normalized plot coordinates.
     * @param realValue Sample as stored on the series (same units as @ref SetRange).
     * @return Position in the axis span [@ref GetStart, @ref GetEnd].
     * @details Does not apply @ref Scale; display scaling is only in
     *          @ref FormatTickValue. If the range width is zero, returns the
     *          midpoint of the axis span.
     */
    double ConvertValue(double realValue) const
    {
        return ConvertRangeValue(realValue);
    }

    /**
     * @brief Inverse of @ref ConvertValue: plot-normalized position → range units.
     * @param plotNorm Position in the axis span [@ref GetStart, @ref GetEnd].
     * @return Value in sample / range units ([@ref GetRangeStart, @ref GetRangeEnd]).
     * @details Pass the result to @ref FormatTickValue for a cursor readout
     *          (Scale is applied there).
     */
    double InverseConvertValue(double plotNorm) const
    {
        const double axisSpan = m_end - m_start;
        if ( axisSpan == 0.0 )
            return (m_rangeStart + m_rangeEnd) * 0.5;

        const double axisPoint = (plotNorm - m_start) / axisSpan;
        return m_rangeStart + axisPoint * (m_rangeEnd - m_rangeStart);
    }

protected:

    wxString m_name;       ///< Unique axis identity.
    AxisType m_axisType;   ///< Orientation.

    double m_start = 0.0;      ///< Normalized axis span start.
    double m_end = 1.0;        ///< Normalized axis span end.
    double m_location = 0;     ///< Normalized perpendicular location.

    double m_rangeStart = 0;   ///< Data range start.
    double m_rangeEnd = 100.0; ///< Data range end.
};

#endif
