/////////////////////////////////////////////////////////////////////////////
// Name:        series.h
// Purpose:     Thread-safe data series used by wxPlotting
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
#ifndef SERIES_H
#define SERIES_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/string.h"
#include "wx/colour.h"
#include "wx/gdicmn.h"  // wxRealPoint
#endif

#include <mutex>
#include <utility>
#include <vector>

#include "seriesrenderer.h"

/**
 * @file series.h
 * @brief Thread-safe data series used by @ref wxPlotting.
 */

/**
 * @class Series
 * @brief Named polyline of samples bound to a horizontal and vertical axis.
 *
 * @details
 * **Point data is mutex-protected** so a worker thread can append or replace
 * samples while the UI thread snapshots them for rendering.
 *
 * Prefer:
 * - @ref AppendPoint / @ref AppendPoints / @ref SetPoints for updates
 * - @ref GetPoints for a safe copy (used by the plot when drawing)
 * - @ref WithPoints for short locked batch edits
 *
 * After mutating points off the UI thread, schedule a redraw on the main
 * thread, e.g. @c plot->CallAfter([plot]{ plot->UpdatePlot(); }).
 *
 * @note @ref Visible, @ref Foreground, and axis name fields are not locked;
 *       change them on the UI thread.
 *
 * **Binding**
 * Set @ref HorizontalAxis and @ref VerticalAxis to the @ref Axis::GetName
 * values of axes already added to the plot. Unknown names cause the series
 * to be skipped at draw time.
 */
class Series
{
public:
    /**
     * @brief Contiguous sample storage (efficient for lock-and-copy draws).
     * @details Each point is typically @c (x_data, y_data) in the units of
     *          the bound horizontal and vertical axes.
     */
    typedef std::vector<wxRealPoint> Points;

    /**
     * @enum SeriesType
     * @brief Semantic type of the series (extensibility; drawing uses doubles).
     */
    enum SeriesType
    {
        None,     ///< Unspecified.
        Doubles,  ///< Numeric X/Y (default drawing path).
        Date,     ///< Reserved for date X values.
        Time      ///< Reserved for time X values.
    };

public:
    /**
     * @brief Construct a series with a unique name and type.
     * @param name Unique name among all series on a plot.
     * @param seriesType Semantic type (usually @ref Doubles).
     * @details Defaults: @ref Visible = true, @ref Foreground = white.
     */
    Series(const wxString& name, SeriesType seriesType)
        : Visible(true),
          Foreground(*wxWHITE),
          m_name(name),
          m_seriesType(seriesType)
    {
    }

    /**
     * @brief Copy constructor.
     * @details Copies metadata and point data under the source lock.
     *          Each instance owns an independent mutex.
     */
    Series(const Series& other)
        : Visible(other.Visible),
          Foreground(other.Foreground),
          RenderStyle(other.RenderStyle),
          CustomRendererFactory(other.CustomRendererFactory),
          HorizontalAxis(other.HorizontalAxis),
          VerticalAxis(other.VerticalAxis),
          m_name(other.m_name),
          m_seriesType(other.m_seriesType),
          m_maxPoints(other.m_maxPoints)
    {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_data = other.m_data;
    }

    /**
     * @brief Copy assignment.
     * @return *this
     */
    Series& operator=(const Series& other)
    {
        if ( this == &other )
            return *this;

        const bool visible = other.Visible;
        const wxColor foreground = other.Foreground;
        const SeriesRenderStyle renderStyle = other.RenderStyle;
        const SeriesRendererFactory customFactory = other.CustomRendererFactory;
        const wxString horz = other.HorizontalAxis;
        const wxString vert = other.VerticalAxis;
        const wxString name = other.m_name;
        const SeriesType type = other.m_seriesType;
        const size_t maxPoints = other.m_maxPoints;

        Points dataCopy;
        {
            std::lock_guard<std::mutex> lock(other.m_mutex);
            dataCopy = other.m_data;
        }

        Visible = visible;
        Foreground = foreground;
        RenderStyle = renderStyle;
        CustomRendererFactory = customFactory;
        HorizontalAxis = horz;
        VerticalAxis = vert;
        m_name = name;
        m_seriesType = type;
        m_maxPoints = maxPoints;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_data = std::move(dataCopy);
            EnforceMaxPoints();
        }
        return *this;
    }

    /**
     * @brief Move constructor.
     * @details Moves metadata and point data; each instance keeps its own mutex.
     */
    Series(Series&& other) noexcept
        : Visible(other.Visible),
          Foreground(std::move(other.Foreground)),
          RenderStyle(other.RenderStyle),
          CustomRendererFactory(std::move(other.CustomRendererFactory)),
          HorizontalAxis(std::move(other.HorizontalAxis)),
          VerticalAxis(std::move(other.VerticalAxis)),
          m_name(std::move(other.m_name)),
          m_seriesType(other.m_seriesType),
          m_maxPoints(other.m_maxPoints)
    {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_data = std::move(other.m_data);
    }

    /**
     * @brief Move assignment.
     * @return *this
     */
    Series& operator=(Series&& other) noexcept
    {
        if ( this == &other )
            return *this;

        Visible = other.Visible;
        Foreground = std::move(other.Foreground);
        RenderStyle = other.RenderStyle;
        CustomRendererFactory = std::move(other.CustomRendererFactory);
        HorizontalAxis = std::move(other.HorizontalAxis);
        VerticalAxis = std::move(other.VerticalAxis);
        m_name = std::move(other.m_name);
        m_seriesType = other.m_seriesType;
        m_maxPoints = other.m_maxPoints;

        Points dataMove;
        {
            std::lock_guard<std::mutex> lock(other.m_mutex);
            dataMove = std::move(other.m_data);
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_data = std::move(dataMove);
            EnforceMaxPoints();
        }
        return *this;
    }

    /**
     * @brief Get the semantic series type.
     */
    SeriesType GetSeriesType() const {
        return m_seriesType;
    }

    /**
     * @brief Get the unique series name.
     * @return Name used by @ref wxPlotting::GetSeries.
     */
    const wxString& GetName() const {
        return m_name;
    }

    // --- Thread-safe point access ------------------------------------------

    /**
     * @brief Maximum samples retained in the series buffer.
     * @details @c 0 means unlimited. When greater than zero, any mutation that
     *          would exceed the limit drops the oldest samples from the front
     *          of the buffer (FIFO). Default is @c 0.
     * @note Changing the limit via @ref SetMaxPoints trims immediately if needed.
     * @see SetMaxPoints, GetMaxPoints
     */
    void SetMaxPoints(size_t maxPoints)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_maxPoints = maxPoints;
        EnforceMaxPoints();
    }

    /**
     * @brief Get the sample buffer limit (@c 0 = unlimited).
     * @see SetMaxPoints
     */
    size_t GetMaxPoints() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_maxPoints;
    }

    /**
     * @brief Replace all samples (moves when possible).
     * @param points New sample vector; emptied after the call if moved from.
     * @details Enforces @ref GetMaxPoints (drops oldest if over limit).
     * @note Thread-safe. Call @ref wxPlotting::UpdatePlot on the UI thread after.
     */
    void SetPoints(Points points)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data = std::move(points);
        EnforceMaxPoints();
    }

    /**
     * @brief Remove all samples.
     * @note Thread-safe.
     */
    void ClearPoints()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data.clear();
    }

    /**
     * @brief Append one sample.
     * @param pt Point in data coordinates (@c x = horizontal axis, @c y = vertical).
     * @details Enforces @ref GetMaxPoints after the append.
     * @note Thread-safe.
     */
    void AppendPoint(const wxRealPoint& pt)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data.push_back(pt);
        EnforceMaxPoints();
    }

    /**
     * @brief Append multiple samples.
     * @param points Samples to append in order.
     * @details Enforces @ref GetMaxPoints after the append.
     * @note Thread-safe.
     */
    void AppendPoints(const Points& points)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data.insert(m_data.end(), points.begin(), points.end());
        EnforceMaxPoints();
    }

    /**
     * @brief Return a snapshot copy of all samples.
     * @return Copy of the point buffer (safe to use without holding the lock).
     * @details Used by the plot when drawing so GDI work is not done under the mutex.
     * @note Thread-safe. Cost is O(n) in the number of points.
     */
    Points GetPoints() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_data;
    }

    /**
     * @brief Return the current sample count.
     * @note Thread-safe.
     */
    size_t GetPointCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_data.size();
    }

    /**
     * @brief Invoke a functor while holding the data lock (mutable).
     * @tparam Fn Callable accepting @c Points&.
     * @param fn Work to perform; keep it short so render/update is not blocked.
     * @return Whatever @a fn returns.
     * @warning Do not call @ref wxPlotting::UpdatePlot or nested long UI work
     *          from @a fn while holding the lock.
     */
    template <typename Fn>
    auto WithPoints(Fn&& fn) -> decltype(fn(std::declval<Points&>()))
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // Enforce after the functor returns (works for void and non-void).
        struct EnforceOnExit
        {
            Series* self;
            ~EnforceOnExit() { if ( self ) self->EnforceMaxPoints(); }
        } enforce{this};
        return fn(m_data);
    }

    /**
     * @brief Invoke a functor while holding the data lock (const).
     * @tparam Fn Callable accepting @c const Points&.
     * @param fn Work to perform; keep it short.
     * @return Whatever @a fn returns.
     */
    template <typename Fn>
    auto WithPoints(Fn&& fn) const -> decltype(fn(std::declval<const Points&>()))
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return fn(m_data);
    }

    /**
     * @brief When @c false, the series is not drawn.
     * @note Not mutex-protected; set on the UI thread.
     */
    bool     Visible;

    /**
     * @brief Stroke colour for the series polyline.
     * @note Not mutex-protected; set on the UI thread.
     */
    wxColor  Foreground;

    /**
     * @brief How samples are stroked when the series is drawn.
     * @details A short-lived @ref SeriesRenderer is created for this style for
     *          each draw pass (@ref CreateSeriesRenderer). For
     *          @ref SeriesRenderStyle::Custom, set @ref CustomRendererFactory.
     * @see SeriesRenderStyle, SeriesRenderer, CustomRendererFactory
     * @note Not mutex-protected; set on the UI thread.
     */
    SeriesRenderStyle RenderStyle = SeriesRenderStyle::Line;

    /**
     * @brief Factory for @ref SeriesRenderStyle::Custom.
     * @details Invoked on each draw pass to create a host-supplied
     *          @ref SeriesRenderer (e.g. a subclass that draws markers or
     *          specialized strokes). The plot owns the returned instance for
     *          that pass only. Leave empty for built-in styles.
     * @note Not mutex-protected; set on the UI thread.
     * @see SeriesRendererFactory, SeriesRenderStyle::Custom
     */
    SeriesRendererFactory CustomRendererFactory;

    /**
     * @brief Name of the horizontal @ref Axis this series uses for X mapping.
     * @note Must match @ref Axis::GetName of an axis on the same plot.
     */
    wxString HorizontalAxis;

    /**
     * @brief Name of the vertical @ref Axis this series uses for Y mapping.
     * @note Must match @ref Axis::GetName of an axis on the same plot.
     */
    wxString VerticalAxis;

private:
    /**
     * @brief Drop oldest samples when @ref m_maxPoints is exceeded.
     * @details Caller must hold @ref m_mutex. No-op when limit is @c 0.
     */
    void EnforceMaxPoints()
    {
        if ( m_maxPoints == 0 )
            return;
        if ( m_data.size() <= m_maxPoints )
            return;
        const size_t drop = m_data.size() - m_maxPoints;
        m_data.erase(m_data.begin(), m_data.begin() + static_cast<std::ptrdiff_t>(drop));
    }

    wxString m_name;           ///< Unique series identity.
    SeriesType m_seriesType;   ///< Semantic type.
    size_t   m_maxPoints = 0;  ///< 0 = unlimited; else max retained samples.

    Points m_data;             ///< Sample buffer (guarded by @ref m_mutex).
    mutable std::mutex m_mutex;///< Protects @ref m_data only.
};

#endif

