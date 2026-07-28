/////////////////////////////////////////////////////////////////////////////
// Name:        plotcoords.h
// Purpose:     Shared plot-space to screen mapping for draw and hit-test
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

