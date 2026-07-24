#ifndef AXIS_H
#define AXIS_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/string.h"
#endif


class Axis
{
// enums

public:

    enum AxisType
    {
        None,
        Horizontal,
        Vertical
    };

public:

    Axis(const wxString& name, AxisType axisType)
    {
        m_name = name;
        m_axisType = axisType;
    }

    AxisType GetAxisType() const {
        return m_axisType;
    }

    void SetVisible(bool cond)
    {
        m_visible = cond;
    }

    bool    GetVisible() const {
        return m_visible;
    }

    int     GetWidth() const {
        return m_width;
    }

    void SetWidth(int value) {
        m_width = value;
    }

    void SetForegrond(const wxColor& color)
    {
        m_foreground = color;
    }

    const wxColor& GetForeground() const {
        return m_foreground;
    }



    double  GetStart() const {
        return m_start;
    }

    double GetEnd() const {
        return m_end;
    }

    void SetRange(double start, double end)
    {
        m_rangeStart = start;
        m_rangeEnd = end;
    }

    double GetRangeStart() const {
        return m_rangeStart;
    }

    double GetRangeEnd() const {
        return m_rangeEnd;
    }

    void SetPosition(double pos) {
        m_pos = pos;
    }

    double GetPosition() const {
        return m_pos;
    }

protected:

    // must be unique among all axes
    wxString m_name;

    AxisType m_axisType;
    bool     m_visible;
    int      m_width;
    wxColor  m_foreground;

    double m_start = 0.0;
    double m_end = 1.0;
    double m_pos = 0;

    double m_rangeStart = 0;
    double m_rangeEnd = 100.0;
};



#endif


