/////////////////////////////////////////////////////////////////////////////
// Name:        wxPlotting.h
// Purpose:     Owner-drawn plotting control (MemoryDC double-buffer template)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef WXPLOTTING_H
#define WXPLOTTING_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/scrolwin.h"
    #include "wx/bitmap.h"
    #include "wx/dcmemory.h"
#endif

// ----------------------------------------------------------------------------
// wxPlotting
//
// Owner-drawn scrolled window that renders into an off-screen wxBitmap via
// wxMemoryDC.  The canvas tracks the client area as it grows, and never
// shrinks below a minimum drawing size.  Scrollbars appear only when the
// client is smaller than that minimum (e.g. after raising the min height as
// series data is added).
// ----------------------------------------------------------------------------

#include <list>
#include "axis.h"
#include "series.h"

class wxPlotting : public wxScrolledWindow
{
public:
    // Default minimum canvas size used when none is supplied.
    static const wxSize DefaultMinDrawingSize;

    wxPlotting();
    wxPlotting(wxWindow* parent,
               wxWindowID id = wxID_ANY,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0,
               const wxString& name = wxASCII_STR("wxPlotting"));

    bool Create(wxWindow* parent,
                wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = wxASCII_STR("wxPlotting"));

    virtual ~wxPlotting();

    // Create a default plot
    void CreateDefaultPlot();

    // Axis / series collections (names must be unique within each list).
    // Add* returns false if the name is empty or already in use.
    bool AddAxis(const Axis& axis);
    bool AddSeries(const Series& series);

    // Lookup by name; returns nullptr if not found.
    Axis* GetAxis(const wxString& name);
    const Axis* GetAxis(const wxString& name) const;
    Series* GetSeries(const wxString& name);
    const Series* GetSeries(const wxString& name) const;

    // Rebuild the off-screen buffer and refresh the control.
    void UpdatePlot();

    // Plot margins (space outside m_plot for axis titles, ticks, labels).
    // SetMargin(n) sets all four sides equal; use SetMargins / per-side
    // setters when left/bottom need more room than top/right.
    void SetMargin(int margin);
    void SetMargins(int left, int right, int top, int bottom);
    void SetMarginLeft(int margin);
    void SetMarginRight(int margin);
    void SetMarginTop(int margin);
    void SetMarginBottom(int margin);

    int  GetMarginLeft() const   { return m_marginLeft; }
    int  GetMarginRight() const  { return m_marginRight; }
    int  GetMarginTop() const    { return m_marginTop; }
    int  GetMarginBottom() const { return m_marginBottom; }
    // Backward-compatible: returns left margin (prefer per-side getters).
    int  GetMargin() const       { return m_marginLeft; }

    // Minimum drawing size.  Actual canvas is max(client, min) per axis.
    // Scrollbars appear only when the client is smaller than this minimum.
    // Raise min height later as series data needs more vertical space.
    void SetMinDrawingSize(const wxSize& size);
    wxSize GetMinDrawingSize() const { return m_minDrawingSize; }

    void SetMinDrawingWidth(int width);
    void SetMinDrawingHeight(int height);
    int  GetMinDrawingWidth() const  { return m_minDrawingSize.x; }
    int  GetMinDrawingHeight() const { return m_minDrawingSize.y; }

    // Aliases: SetDrawing* adjusts the minimum; GetDrawing* returns the
    // current effective canvas (max of client and minimum).
    void SetDrawingSize(const wxSize& size) { SetMinDrawingSize(size); }
    void SetDrawingWidth(int width)         { SetMinDrawingWidth(width); }
    void SetDrawingHeight(int height)       { SetMinDrawingHeight(height); }

    wxSize GetDrawingSize() const { return m_drawingSize; }
    int  GetDrawingWidth() const  { return m_drawingSize.x; }
    int  GetDrawingHeight() const { return m_drawingSize.y; }

    // Access the off-screen buffer (valid after the first size/paint).
    const wxBitmap& GetBuffer() const { return m_buffer; }

protected:
    // Draw plot content into the provided memory DC.
    // size is the plot area size (drawing size minus margins).
    // Default implementation fills a dark background and draws axes.
    virtual void DrawPlot(wxMemoryDC& dc, const wxSize& size);

    // Event handlers
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnEraseBackground(wxEraseEvent& event);

    // Buffer / layout management
    void RecreateBuffer();
    void Render();
    // Returns true if the effective drawing size changed.
    bool SyncDrawingSizeToClient();

    wxSize DoGetBestClientSize() const override;

private:
    std::list<Axis> m_axisList;
    std::list<Series> m_seriesList;

    wxBitmap m_buffer;
    wxSize   m_plotSize;
    wxRect   m_plot;

    // Per-side margins (defaults leave extra room for left/bottom labels).
    int m_marginLeft   = 80;
    int m_marginRight  = 40;
    int m_marginTop    = 40;
    int m_marginBottom = 70;

    // Floor for the canvas; scrollbars only when client < this.
    wxSize m_minDrawingSize;
    // Effective canvas: max(client, min) per axis (includes margins).
    wxSize m_drawingSize;

    wxDECLARE_DYNAMIC_CLASS(wxPlotting);
    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(wxPlotting);
};

#endif // WXPLOTTING_H
