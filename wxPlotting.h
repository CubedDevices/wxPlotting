/////////////////////////////////////////////////////////////////////////////
// Name:        wxPlotting.h
// Purpose:     Owner-drawn plotting control (MemoryDC double-buffer template)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef WXPLOTTING_H
#define WXPLOTTING_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/control.h"
    #include "wx/bitmap.h"
    #include "wx/dcmemory.h"
#endif

// ----------------------------------------------------------------------------
// wxPlotting
//
// Owner-drawn wxControl that renders into an off-screen wxBitmap via
// wxMemoryDC, then blits that buffer to the screen on paint.  Override
// DrawPlot() (or fill it in) when real plot content is ready.
// ----------------------------------------------------------------------------

class wxPlotting : public wxControl
{
public:
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

    // Rebuild the off-screen buffer and refresh the control.
    void UpdatePlot();

    // Access the off-screen buffer (valid after the first size/paint).
    const wxBitmap& GetBuffer() const { return m_buffer; }

protected:
    // Draw plot content into the provided memory DC.
    // size is the client size of the control (and of the selected bitmap).
    // Default implementation fills a dark background and draws a placeholder.
    virtual void DrawPlot(wxMemoryDC& dc, const wxSize& size);

    // Event handlers
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnEraseBackground(wxEraseEvent& event);

    // Buffer management
    void RecreateBuffer();
    void Render();

    wxSize DoGetBestClientSize() const override;

private:
    wxBitmap m_buffer;

    wxDECLARE_DYNAMIC_CLASS(wxPlotting);
    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(wxPlotting);
};

#endif // WXPLOTTING_H
