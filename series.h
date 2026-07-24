#ifndef SERIES_H
#define SERIES_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/string.h"
#endif


class Series
{
public:


    typedef std::list<wxRealPoint> Points;

    enum SeriesType
    {
        None,
        Doubles,
        Date,
        Time
    };

public:

    Series(const wxString& name, SeriesType seriesType)
    {
        m_name = name;
        m_seriesType = seriesType;
    }

    SeriesType GetSeriesType() const {
        return m_seriesType;
    }

    Points Data; 
    bool Visible;
    wxColor Foreground;
    wxString HorizontalAxis;
    wxString VerticalAxis;


 private:

    wxString m_name;
    SeriesType m_seriesType;

};











#endif