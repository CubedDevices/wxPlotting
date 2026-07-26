/////////////////////////////////////////////////////////////////////////////
// Name:        plothit.h
// Purpose:     Hit-test result types for wxPlotting UI interaction
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PLOTHIT_H
#define PLOTHIT_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/gdicmn.h"
#endif

#include <vector>

/**
 * @file plothit.h
 * @brief Result types for @ref wxPlotting::HitTest.
 */

/**
 * @enum PlotHitKind
 * @brief Primary classification of a hit-test pick.
 *
 * @details Axis membership is always reported in @ref PlotHitResult::axes
 *          when the pick lies in one or more axes' valid data regions.
 *          @c kind reflects the most specific geometry also found (series
 *          stroke, axes-only, empty plot area, or none).
 */
enum class PlotHitKind
{
    /** Outside the plot data area and not on interactive geometry. */
    None,

    /**
     * Inside @c m_plot but not on a series stroke and not inside any axis
     * valid region (unusual if axes cover the plot).
     */
    PlotArea,

    /**
     * Inside one or more axis valid regions, and not on a series stroke.
     * @ref PlotHitResult::axes lists all such axes (often one horizontal
     * and one vertical).
     */
    Axis,

    /**
     * Near a series stroke (within tolerance).
     * @ref PlotHitResult::name is the series name; @ref PlotHitResult::axes
     * still lists axes covering that location.
     */
    Series
};

/**
 * @struct PlotHitResult
 * @brief Outcome of a plot hit-test query.
 *
 * @details
 * **Axes** use region containment (the full valid-data rectangle of each
 * axis) **or** a hit on that axis's title label (same placement as drawing).
 * A pick in a multi-lane plot typically yields both a horizontal and a
 * vertical axis in @ref axes; picking a label alone yields that axis.
 *
 * **Series** use distance to the stroked polyline / stair-step within
 * @a tolerancePx. Series hits still populate @ref axes for the same point.
 */
struct PlotHitResult
{
    /**
     * @brief Primary hit kind (series preferred over axes-only over plot area).
     */
    PlotHitKind kind = PlotHitKind::None;

    /**
     * @brief Primary name: series name when @ref kind is @ref PlotHitKind::Series.
     * @details Empty for axis-only / plot-area / none. Use @ref axes for axis names.
     */
    wxString name;

    /**
     * @brief All visible axes whose valid data region contains the pick.
     * @details Order follows the axis list (registration order). Typically
     *          includes one horizontal and one vertical axis when the cursor
     *          is over plotted data.
     */
    std::vector<wxString> axes;

    /**
     * @brief Distance in canvas pixels to the series stroke when @ref kind is Series.
     * @details Unused (0) for pure axis / plot-area hits (region containment).
     */
    double distancePx = 0.0;

    /**
     * @brief Index of the nearer sample endpoint on the closest series segment.
     * @details Valid when @ref kind is @ref PlotHitKind::Series.
     */
    size_t sampleIndex = 0;

    /**
     * @brief Data coordinates of the sample at @ref sampleIndex.
     * @details Valid when @ref kind is @ref PlotHitKind::Series.
     */
    wxRealPoint dataPoint;
};

#endif // PLOTHIT_H
