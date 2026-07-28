/////////////////////////////////////////////////////////////////////////////
// Name:        seriesrenderer.cpp
// Purpose:     Short-lived series stroke renderers for wxPlotting
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "seriesrenderer.h"
#include "series.h"

#include "wx/pen.h"
#include "wx/brush.h"


// ----------------------------------------------------------------------------
// SeriesLineRenderer
// ----------------------------------------------------------------------------

void SeriesLineRenderer::Begin(wxDC& dc, const Series& series, const wxRect& plot)
{
    m_dc = &dc;
    m_plot = plot;
    m_hasLast = false;
    wxPen pen(series.Foreground);
    dc.SetPen(pen);
}

void SeriesLineRenderer::AddPoint(const wxRealPoint& plotPt)
{
    if ( !m_dc )
        return;

    const wxPoint screen = ToScreen(plotPt);

    if ( m_hasLast )
        m_dc->DrawLine(m_lastScreen, screen);

    m_lastScreen = screen;
    m_hasLast = true;
}

void SeriesLineRenderer::Break()
{
    m_hasLast = false;
}

void SeriesLineRenderer::End()
{
    m_hasLast = false;
    m_dc = nullptr;
}

// ----------------------------------------------------------------------------
// SeriesStairStepRenderer
// ----------------------------------------------------------------------------

void SeriesStairStepRenderer::Begin(wxDC& dc, const Series& series, const wxRect& plot)
{
    m_dc = &dc;
    m_plot = plot;
    m_hasLast = false;
    wxPen pen(series.Foreground);
    dc.SetPen(pen);
}

void SeriesStairStepRenderer::AddPoint(const wxRealPoint& plotPt)
{
    if ( !m_dc )
        return;

    const wxPoint screen = ToScreen(plotPt);

    if ( m_hasLast )
    {
        // Hold previous Y until new X, then step to new Y (sample-and-hold).
        const wxPoint corner(screen.x, m_lastScreen.y);
        m_dc->DrawLine(m_lastScreen, corner);
        m_dc->DrawLine(corner, screen);
    }

    m_lastScreen = screen;
    m_hasLast = true;
}

void SeriesStairStepRenderer::Break()
{
    m_hasLast = false;
}

void SeriesStairStepRenderer::End()
{
    m_hasLast = false;
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
    for ( size_t i = 1; i < m_segment.size(); ++i )
        m_dc->DrawLine(m_segment[i - 1], m_segment[i]);

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
// factory
// ----------------------------------------------------------------------------

std::unique_ptr<SeriesRenderer> CreateSeriesRenderer(SeriesRenderStyle style)
{
    switch ( style )
    {
        case SeriesRenderStyle::StairStep:
            return std::unique_ptr<SeriesRenderer>(new SeriesStairStepRenderer());

        case SeriesRenderStyle::Fill:
            return std::unique_ptr<SeriesRenderer>(new SeriesFillRenderer());

        case SeriesRenderStyle::Line:
        default:
            return std::unique_ptr<SeriesRenderer>(new SeriesLineRenderer());
    }
}
