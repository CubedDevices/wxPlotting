/////////////////////////////////////////////////////////////////////////////
// Name:        seriesrenderer.cpp
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
#include "seriesrenderer.h"
#include "series.h"

#include "wx/pen.h"
#include "wx/brush.h"

namespace
{

// One Polyline per chunk. A per-sample DrawLine is a GDI call each; on Windows
// that dominates a large capture, and the H/V fast path is unavailable once
// the memory DC is scaled (high DPI). DrawLines is a single Polyline.
void StrokePolyline(wxDC& dc, const std::vector<wxPoint>& path)
{
    const int n = static_cast<int>(path.size());
    if ( n < 2 )
        return;

    const int chunk = 32768;
    int start = 0;
    while ( start < n - 1 )
    {
        int count = n - start;
        if ( count > chunk )
            count = chunk;
        dc.DrawLines(count, &path[static_cast<size_t>(start)]);
        start += count - 1;
    }
}

int AbsDelta(int a, int b)
{
    return a > b ? a - b : b - a;
}

} // namespace

// ----------------------------------------------------------------------------
// SeriesLineRenderer
// ----------------------------------------------------------------------------

void SeriesLineRenderer::Begin(wxDC& dc, const Series& series, const wxRect& plot)
{
    m_dc = &dc;
    m_plot = plot;
    m_open = false;
    m_path.clear();
    wxPen pen(series.Foreground);
    dc.SetPen(pen);
}

void SeriesLineRenderer::Append(int x, int y)
{
    if ( m_path.empty() || m_path.back().x != x || m_path.back().y != y )
        m_path.push_back(wxPoint(x, y));
}

void SeriesLineRenderer::EmitColumn()
{
    if ( !m_open )
        return;

    // Visit the column's vertical extent, then leave from the last sample so
    // the segment into the next column starts where the data left off.
    int yFirst = m_colMinY;
    int ySecond = m_colMaxY;
    if ( !m_path.empty() &&
         AbsDelta(m_path.back().y, m_colMaxY) < AbsDelta(m_path.back().y, m_colMinY) )
    {
        yFirst = m_colMaxY;
        ySecond = m_colMinY;
    }

    Append(m_colX, yFirst);
    Append(m_colX, ySecond);
    Append(m_colX, m_colLastY);
    m_open = false;
}

void SeriesLineRenderer::AddPoint(const wxRealPoint& plotPt)
{
    if ( !m_dc )
        return;

    const wxPoint screen = ToScreen(plotPt);

    if ( !m_open )
    {
        Append(screen.x, screen.y);
        m_colX = screen.x;
        m_colMinY = m_colMaxY = m_colLastY = screen.y;
        m_open = true;
        return;
    }

    if ( screen.x == m_colX )
    {
        if ( screen.y < m_colMinY )
            m_colMinY = screen.y;
        if ( screen.y > m_colMaxY )
            m_colMaxY = screen.y;
        m_colLastY = screen.y;
        return;
    }

    EmitColumn();
    m_colX = screen.x;
    m_colMinY = m_colMaxY = m_colLastY = screen.y;
    m_open = true;
}

void SeriesLineRenderer::Stroke()
{
    if ( m_dc )
        StrokePolyline(*m_dc, m_path);
    m_path.clear();
}

void SeriesLineRenderer::Break()
{
    EmitColumn();
    Stroke();
}

void SeriesLineRenderer::End()
{
    EmitColumn();
    Stroke();
    m_dc = nullptr;
}

// ----------------------------------------------------------------------------
// SeriesStairStepRenderer
// ----------------------------------------------------------------------------

void SeriesStairStepRenderer::Begin(wxDC& dc, const Series& series, const wxRect& plot)
{
    m_dc = &dc;
    m_plot = plot;
    m_open = false;
    m_path.clear();
    wxPen pen(series.Foreground);
    dc.SetPen(pen);
}

void SeriesStairStepRenderer::Append(int x, int y)
{
    if ( m_path.empty() || m_path.back().x != x || m_path.back().y != y )
        m_path.push_back(wxPoint(x, y));
}

void SeriesStairStepRenderer::AppendVerticalExtent()
{
    if ( !m_open )
        return;

    if ( m_colMinY != m_colMaxY )
    {
        Append(m_colX, m_colMinY);
        Append(m_colX, m_colMaxY);
    }
    Append(m_colX, m_colEndY);
}

void SeriesStairStepRenderer::AddPoint(const wxRealPoint& plotPt)
{
    if ( !m_dc )
        return;

    const wxPoint screen = ToScreen(plotPt);

    if ( !m_open )
    {
        Append(screen.x, screen.y);
        m_colX = screen.x;
        m_colMinY = m_colMaxY = m_colEndY = screen.y;
        m_open = true;
        return;
    }

    if ( screen.x == m_colX )
    {
        if ( screen.y < m_colMinY )
            m_colMinY = screen.y;
        if ( screen.y > m_colMaxY )
            m_colMaxY = screen.y;
        m_colEndY = screen.y;
        return;
    }

    // Cover this column, then hold the last level across to the new X.
    // The step to the new Y is part of the new column (flushed later).
    AppendVerticalExtent();
    const int holdY = m_colEndY;
    Append(screen.x, holdY);

    m_colX = screen.x;
    m_colEndY = screen.y;
    m_colMinY = wxMin(holdY, screen.y);
    m_colMaxY = wxMax(holdY, screen.y);
}

void SeriesStairStepRenderer::Stroke()
{
    if ( m_dc )
        StrokePolyline(*m_dc, m_path);
    m_path.clear();
}

void SeriesStairStepRenderer::Break()
{
    AppendVerticalExtent();
    m_open = false;
    Stroke();
}

void SeriesStairStepRenderer::End()
{
    AppendVerticalExtent();
    m_open = false;
    Stroke();
    m_dc = nullptr;
}

// ----------------------------------------------------------------------------
// SeriesFillRenderer
// ----------------------------------------------------------------------------

void SeriesFillRenderer::Begin(wxDC& dc, const Series& series, const wxRect& plot)
{
    m_dc = &dc;
    m_plot = plot;
    m_colour = series.Foreground;
    m_baselinePlotY = 0.0;
    m_segment.clear();
    dc.SetPen(wxPen(m_colour));
    dc.SetBrush(wxBrush(m_colour));
}

void SeriesFillRenderer::SetBaselinePlotY(double plotNormY)
{
    m_baselinePlotY = plotNormY;
}

void SeriesFillRenderer::AddPoint(const wxRealPoint& plotPt)
{
    if ( !m_dc )
        return;

    m_segment.push_back(ToScreen(plotPt));
}

void SeriesFillRenderer::FlushSegment()
{
    if ( !m_dc || m_segment.empty() )
    {
        m_segment.clear();
        return;
    }

    // Baseline Y in screen pixels (lower edge of the vertical axis band).
    const int baseY = ToScreen(wxRealPoint(0.0, m_baselinePlotY)).y;

    if ( m_segment.size() == 1 )
    {
        // Degenerate: stem from sample down to baseline.
        m_dc->SetPen(wxPen(m_colour));
        m_dc->DrawLine(m_segment[0], wxPoint(m_segment[0].x, baseY));
        m_segment.clear();
        return;
    }

    // Polygon: curve points, then along baseline back to the first X.
    std::vector<wxPoint> poly;
    poly.reserve(m_segment.size() + 2);
    poly.insert(poly.end(), m_segment.begin(), m_segment.end());
    poly.push_back(wxPoint(m_segment.back().x, baseY));
    poly.push_back(wxPoint(m_segment.front().x, baseY));

    m_dc->SetPen(*wxTRANSPARENT_PEN);
    m_dc->SetBrush(wxBrush(m_colour));
    m_dc->DrawPolygon(static_cast<int>(poly.size()), poly.data());

    // Stroke the curve on top so the edge stays visible.
    m_dc->SetPen(wxPen(m_colour));
    m_dc->SetBrush(*wxTRANSPARENT_BRUSH);
    StrokePolyline(*m_dc, m_segment);

    m_segment.clear();
}

void SeriesFillRenderer::Break()
{
    FlushSegment();
}

void SeriesFillRenderer::End()
{
    FlushSegment();
    m_dc = nullptr;
}

// ----------------------------------------------------------------------------
// SeriesPointsRenderer
// ----------------------------------------------------------------------------

void SeriesPointsRenderer::Begin(wxDC& dc, const Series& series, const wxRect& plot)
{
    m_dc = &dc;
    m_plot = plot;
    m_colour = series.Foreground;
    m_radius = 2;
    dc.SetPen(wxPen(m_colour));
    dc.SetBrush(wxBrush(m_colour));
}

void SeriesPointsRenderer::AddPoint(const wxRealPoint& plotPt)
{
    if ( !m_dc )
        return;

    const wxPoint screen = ToScreen(plotPt);
    // Small filled disc; no segments between samples.
    m_dc->DrawCircle(screen, m_radius);
}

void SeriesPointsRenderer::Break()
{
    // Nothing to flush — points are independent.
}

void SeriesPointsRenderer::End()
{
    m_dc = nullptr;
}

// ----------------------------------------------------------------------------
// factory
// ----------------------------------------------------------------------------

std::unique_ptr<SeriesRenderer> CreateSeriesRenderer(const Series& series)
{
    switch ( series.RenderStyle )
    {
        case SeriesRenderStyle::StairStep:
            return std::unique_ptr<SeriesRenderer>(new SeriesStairStepRenderer());

        case SeriesRenderStyle::Fill:
            return std::unique_ptr<SeriesRenderer>(new SeriesFillRenderer());

        case SeriesRenderStyle::Points:
            return std::unique_ptr<SeriesRenderer>(new SeriesPointsRenderer());

        case SeriesRenderStyle::Custom:
            if ( series.CustomRendererFactory )
                return series.CustomRendererFactory();
            return nullptr;

        case SeriesRenderStyle::Line:
        default:
            return std::unique_ptr<SeriesRenderer>(new SeriesLineRenderer());
    }
}

