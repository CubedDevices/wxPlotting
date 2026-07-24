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

    bool     Visible;

    int      Width;
    wxColor  Foreground;
    bool     TickMarks;
    wxColor  TickColor;
    bool     Labels;
    wxColor  LabelColor;
    double   Scale;


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


    double m_start = 0.0;
    double m_end = 1.0;
    double m_pos = 0;

    double m_rangeStart = 0;
    double m_rangeEnd = 100.0;
};



#endif


