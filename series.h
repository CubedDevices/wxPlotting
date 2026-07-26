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

// ----------------------------------------------------------------------------
// Series
//
// Point data is protected by a mutex so a worker thread can update samples
// while the UI thread snapshots them for rendering.  Prefer the accessors
// (AppendPoint / SetPoints / GetPoints / WithPoints) over direct storage.
//
// After mutating points off the UI thread, schedule a redraw on the main
// thread (e.g. CallAfter(&wxPlotting::UpdatePlot)).
// ----------------------------------------------------------------------------

class Series
{
public:
    // Contiguous storage — better for bulk copy under lock and for drawing.
    typedef std::vector<wxRealPoint> Points;

    enum SeriesType
    {
        None,
        Doubles,
        Date,
        Time
    };

public:
    Series(const wxString& name, SeriesType seriesType)
        : Visible(true),
          m_name(name),
          m_seriesType(seriesType)
    {
    }

    // Each instance owns its own mutex; copies share no lock state.
    Series(const Series& other)
        : Visible(other.Visible),
          Foreground(other.Foreground),
          HorizontalAxis(other.HorizontalAxis),
          VerticalAxis(other.VerticalAxis),
          m_name(other.m_name),
          m_seriesType(other.m_seriesType)
    {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_data = other.m_data;
    }

    Series& operator=(const Series& other)
    {
        if ( this == &other )
            return *this;

        // Copy metadata without holding both locks.
        const bool visible = other.Visible;
        const wxColor foreground = other.Foreground;
        const wxString horz = other.HorizontalAxis;
        const wxString vert = other.VerticalAxis;
        const wxString name = other.m_name;
        const SeriesType type = other.m_seriesType;

        Points dataCopy;
        {
            std::lock_guard<std::mutex> lock(other.m_mutex);
            dataCopy = other.m_data;
        }

        Visible = visible;
        Foreground = foreground;
        HorizontalAxis = horz;
        VerticalAxis = vert;
        m_name = name;
        m_seriesType = type;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_data = std::move(dataCopy);
        }
        return *this;
    }

    Series(Series&& other) noexcept
        : Visible(other.Visible),
          Foreground(std::move(other.Foreground)),
          HorizontalAxis(std::move(other.HorizontalAxis)),
          VerticalAxis(std::move(other.VerticalAxis)),
          m_name(std::move(other.m_name)),
          m_seriesType(other.m_seriesType)
    {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_data = std::move(other.m_data);
    }

    Series& operator=(Series&& other) noexcept
    {
        if ( this == &other )
            return *this;

        Visible = other.Visible;
        Foreground = std::move(other.Foreground);
        HorizontalAxis = std::move(other.HorizontalAxis);
        VerticalAxis = std::move(other.VerticalAxis);
        m_name = std::move(other.m_name);
        m_seriesType = other.m_seriesType;

        Points dataMove;
        {
            std::lock_guard<std::mutex> lock(other.m_mutex);
            dataMove = std::move(other.m_data);
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_data = std::move(dataMove);
        }
        return *this;
    }

    SeriesType GetSeriesType() const {
        return m_seriesType;
    }

    const wxString& GetName() const {
        return m_name;
    }

    // --- Thread-safe point access ------------------------------------------

    // Replace all points (moves when possible).
    void SetPoints(Points points)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data = std::move(points);
    }

    void ClearPoints()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data.clear();
    }

    void AppendPoint(const wxRealPoint& pt)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data.push_back(pt);
    }

    void AppendPoints(const Points& points)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data.insert(m_data.end(), points.begin(), points.end());
    }

    // Copy of current points for safe use outside the lock (e.g. rendering).
    Points GetPoints() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_data;
    }

    size_t GetPointCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_data.size();
    }

    // Invoke fn(const Points&) or fn(Points&) while holding the data lock.
    // Keep work inside fn short — the render thread may be waiting.
    template <typename Fn>
    auto WithPoints(Fn&& fn) -> decltype(fn(std::declval<Points&>()))
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return fn(m_data);
    }

    template <typename Fn>
    auto WithPoints(Fn&& fn) const -> decltype(fn(std::declval<const Points&>()))
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return fn(m_data);
    }

    // Non-point configuration (typically set on the UI thread only).
    bool     Visible;
    wxColor  Foreground;
    wxString HorizontalAxis;
    wxString VerticalAxis;

private:
    // must be unique among all series on a plot
    wxString m_name;
    SeriesType m_seriesType;

    Points m_data;
    mutable std::mutex m_mutex;
};

#endif
