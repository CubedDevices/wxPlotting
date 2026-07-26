/////////////////////////////////////////////////////////////////////////////
// Name:        plotcoords.h
// Purpose:     Shared plot-space to screen mapping for draw and hit-test
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PLOTCOORDS_H
#define PLOTCOORDS_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/gdicmn.h"
#endif

/**
 * @file plotcoords.h
 * @brief Shared coordinate helpers for @ref wxPlotting and series renderers.
 */

/**
 * @brief Map axis-converted coordinates into canvas/logical pixels.
 * @param plot Plot data rectangle (@c m_plot).
 * @param plotPt @c x/@c y from @ref Axis::ConvertValue (normalized plot space).
 * @return Pixel position including @a plot origin; Y increases upward in data.
 */
inline wxPoint PlotToScreen(const wxRect& plot, const wxRealPoint& plotPt)
{
    wxPoint pt;
    pt.x = plot.x + wxRound(plotPt.x * plot.GetWidth());
    pt.y = plot.y + plot.GetHeight()
         - wxRound(plotPt.y * plot.GetHeight());
    return pt;
}

#endif // PLOTCOORDS_H
