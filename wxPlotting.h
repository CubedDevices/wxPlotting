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
// wxMemoryDC at a fixed drawing size (virtual canvas).  Scrollbars appear
// when the viewport is smaller than the drawing size.  Grow the canvas later
// (e.g. as series data is added) with SetDrawingSize / SetDrawingHeight.
// ----------------------------------------------------------------------------

#include <list>
#include "axis.h"
#include "series.h"

class wxPlotting : public wxScrolledWindow
{
public:
    // Default virtual canvas size used when none is supplied.
    static const wxSize DefaultDrawingSize;

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

    // Fixed drawing (virtual canvas) size.  Independent of the window/viewport.
    // Changing this recreates the MemoryDC buffers and updates scrollbars.
    void SetDrawingSize(const wxSize& size);
    wxSize GetDrawingSize() const { return m_drawingSize; }

    void SetDrawingWidth(int width);
    void SetDrawingHeight(int height);
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

    // Buffer management
    void RecreateBuffer();
    void Render();
    void UpdateScrollbars();

    wxSize DoGetBestClientSize() const override;

private:
    std::list<Axis> m_axisList;
    std::list<Series> m_seriesList;

    wxBitmap m_buffer;
    wxBitmap m_plotBuffer;
    int m_margin = 50;

    // Virtual canvas size for the full drawing (including margins).
    wxSize m_drawingSize;

    wxDECLARE_DYNAMIC_CLASS(wxPlotting);
    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(wxPlotting);
};

#endif // WXPLOTTING_H
