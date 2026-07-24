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

    // Rebuild the off-screen buffer and refresh the control.
    void UpdatePlot();

    void SetMargin(int margin);
    int  GetMargin() const;

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
    wxBitmap m_plotBuffer;
    int m_margin = 50;

    // Floor for the canvas; scrollbars only when client < this.
    wxSize m_minDrawingSize;
    // Effective canvas: max(client, min) per axis (includes margins).
    wxSize m_drawingSize;

    wxDECLARE_DYNAMIC_CLASS(wxPlotting);
    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(wxPlotting);
};

#endif // WXPLOTTING_H
