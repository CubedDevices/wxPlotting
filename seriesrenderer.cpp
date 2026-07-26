/////////////////////////////////////////////////////////////////////////////
// Name:        seriesrenderer.cpp
// Purpose:     Short-lived series stroke renderers for wxPlotting
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "seriesrenderer.h"
#include "series.h"

#include "wx/pen.h"


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
// factory
// ----------------------------------------------------------------------------

std::unique_ptr<SeriesRenderer> CreateSeriesRenderer(SeriesRenderStyle style)
{
    switch ( style )
    {
        case SeriesRenderStyle::StairStep:
            return std::unique_ptr<SeriesRenderer>(new SeriesStairStepRenderer());

        case SeriesRenderStyle::Line:
        default:
            return std::unique_ptr<SeriesRenderer>(new SeriesLineRenderer());
    }
}
