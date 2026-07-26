#ifndef AXIS_H
#define AXIS_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/string.h"
#include "wx/font.h"
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

    // Which side of the axis line receives the label.
    // Horizontal: BelowOrLeft = below, AboveOrRight = above.
    // Vertical:   BelowOrLeft = left,  AboveOrRight = right.
    enum LabelSide
    {
        BelowOrLeft = 0,
        AboveOrRight = 1
    };

public:

    Axis(const wxString& name, AxisType axisType)
    {
        m_name = name;
        m_axisType = axisType;
        Label = name;
    }

    AxisType GetAxisType() const {
        return m_axisType;
    }

    const wxString &GetName() const {
        return m_name;
    }

    bool     Visible;

    int      Width;
    wxColor  Foreground;

    bool     TickVisible = false;
    wxColor  TickColor;
    LabelSide TickSide = BelowOrLeft;
    double   TickRange = 1.0;
    wxFont   TickFont = wxFont(wxFontInfo(8).Family(wxFONTFAMILY_MODERN));
    // When true, draw the numeric value at each tick (always upright text).
    bool     TickLabelVisible = false;

    wxString Label;
    bool     LabelVisible = true;
    wxColor  LabelColor;
    LabelSide LabelSide = BelowOrLeft;
    wxFont   LabelFont = wxFont(wxFontInfo(12).Family(wxFONTFAMILY_MODERN));

    bool     GridVisible = true;
    wxColor  GridColor;

    double   Scale;

    void SetPosition(double start, double end)
    {
        m_start = start;
        m_end = end;
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

    void SetLocation(double pos) {
        m_location = pos;
    }

    double GetLocation() const {
        return m_location;
    }

    double ConvertValue(double realValue)
    {
        // find out location on axis
        double axisPoint = ((realValue - m_rangeStart) / ( m_rangeEnd - m_rangeStart));
        double plotPloint = m_start + ((axisPoint) * (m_end - m_start));
        return plotPloint;
    }

protected:

    // must be unique among all axes
    wxString m_name;
    AxisType m_axisType;


    double m_start = 0.0;
    double m_end = 1.0;
    double m_location = 0;

    double m_rangeStart = 0;
    double m_rangeEnd = 100.0;
};



#endif


