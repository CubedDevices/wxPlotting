/////////////////////////////////////////////////////////////////////////////
// Name:        wxPlottingRealTimeDemo.cpp
// Purpose:     Real-time plotting demo (worker thread + sine/cosine)
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
#include "wxPlottingRealTimeDemo.h"
#include "wxPlotting.h"

#include "wx/sizer.h"
#include "wx/statusbr.h"
#include "wx/msgdlg.h"
#include "wx/spinctrl.h"
#include "wx/button.h"
#include "wx/stattext.h"
#include "wx/panel.h"

#include <chrono>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ----------------------------------------------------------------------------
// application
// ----------------------------------------------------------------------------

wxIMPLEMENT_APP(wxPlottingRealTimeApp);

bool wxPlottingRealTimeApp::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

    auto* frame = new wxPlottingRealTimeFrame();
    frame->Show(true);
    return true;
}

// ----------------------------------------------------------------------------
// frame
// ----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(wxPlottingRealTimeFrame, wxFrame)
    EVT_BUTTON(ID_START, wxPlottingRealTimeFrame::OnStart)
    EVT_BUTTON(ID_STOP,  wxPlottingRealTimeFrame::OnStop)
    EVT_BUTTON(ID_PAUSE, wxPlottingRealTimeFrame::OnPause)
    EVT_BUTTON(ID_RESET, wxPlottingRealTimeFrame::OnReset)
    EVT_SPINCTRL(ID_RATE, wxPlottingRealTimeFrame::OnRate)
    EVT_CLOSE(wxPlottingRealTimeFrame::OnClose)
wxEND_EVENT_TABLE()

wxPlottingRealTimeFrame::wxPlottingRealTimeFrame()
    : wxFrame(nullptr, wxID_ANY, wxT("wxPlotting Real-Time Demo"),
              wxDefaultPosition, wxSize(900, 560))
{
    CreateStatusBar(1);
    SetStatusText(wxT("Stopped — press Start to stream sine/cosine samples"));

    auto* root = new wxPanel(this);
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    BuildControls(root);
    m_plot = new wxPlotting(root, wxID_ANY);
    BuildPlot();

    // Control strip on top; plot fills the rest.
    auto* ctrlRow = new wxBoxSizer(wxHORIZONTAL);
    ctrlRow->Add(m_btnStart, 0, wxALL, 4);
    ctrlRow->Add(m_btnStop, 0, wxALL, 4);
    ctrlRow->Add(m_btnPause, 0, wxALL, 4);
    ctrlRow->Add(m_btnReset, 0, wxALL, 4);
    ctrlRow->AddSpacer(12);
    ctrlRow->Add(new wxStaticText(root, wxID_ANY, wxT("Rate (ms):")),
                 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
    ctrlRow->Add(m_rateSpin, 0, wxALL, 4);

    rootSizer->Add(ctrlRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 4);
    rootSizer->Add(m_plot, 1, wxEXPAND | wxALL, 4);
    root->SetSizer(rootSizer);

    auto* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(root, 1, wxEXPAND);
    SetSizer(frameSizer);

    m_btnStop->Enable(false);
    m_btnPause->Enable(false);

    Centre();
}

wxPlottingRealTimeFrame::~wxPlottingRealTimeFrame()
{
    StopWorker();
}

void wxPlottingRealTimeFrame::BuildControls(wxWindow* parent)
{
    m_btnStart = new wxButton(parent, ID_START, wxT("&Start"));
    m_btnStop  = new wxButton(parent, ID_STOP,  wxT("S&top"));
    m_btnPause = new wxButton(parent, ID_PAUSE, wxT("&Pause"));
    m_btnReset = new wxButton(parent, ID_RESET, wxT("&Reset"));

    m_rateSpin = new wxSpinCtrl(parent, ID_RATE, wxT("50"),
                                wxDefaultPosition, wxSize(80, -1),
                                wxSP_ARROW_KEYS, 1, 5000, 50);
    m_rateMs.store(50);
}

void wxPlottingRealTimeFrame::BuildPlot()
{
    // Horizontal axis is sample index (not wall-clock time). Rate only
    // controls how often the worker appends; X is always n, n+1, n+2, …
    Axis sampleAxis("Sample", Axis::Horizontal);
    sampleAxis.Visible = true;
    sampleAxis.Foreground = wxColour(0, 200, 0);
    sampleAxis.LabelColor = sampleAxis.Foreground;
    sampleAxis.LabelVisible = true;
    sampleAxis.Label = wxT("Sample");
    sampleAxis.SetLocation(0.0);
    sampleAxis.SetPosition(0.0, 1.0);
    sampleAxis.SetRange(0.0, static_cast<double>(kSampleWindow));
    sampleAxis.SetData(0.0, static_cast<double>(kSampleWindow));
    sampleAxis.TickVisible = true;
    sampleAxis.TickLabelVisible = true;
    sampleAxis.TickRange = 20.0;
    sampleAxis.TickLabelFormat = wxT("%.0f");
    sampleAxis.TickColor = sampleAxis.Foreground;
    sampleAxis.GridVisible = true;
    sampleAxis.GridColor = wxColour(60, 60, 60);
    sampleAxis.GridStyle = wxPENSTYLE_DOT;

    Axis sinAxis("Sine", Axis::Vertical);
    sinAxis.Visible = true;
    sinAxis.Foreground = wxColour(255, 80, 80);
    sinAxis.LabelColor = sinAxis.Foreground;
    sinAxis.LabelVisible = true;
    sinAxis.Label = wxT("Sine");
    sinAxis.LabelRotated = true;
    sinAxis.SetLocation(0.0);
    sinAxis.SetPosition(0.52, 1.0);
    sinAxis.SetRange(-1.2, 1.2);
    sinAxis.SetData(-1.2, 1.2);
    sinAxis.TickVisible = true;
    sinAxis.TickLabelVisible = true;
    sinAxis.TickRange = 0.5;
    sinAxis.TickLabelFormat = wxT("%.1f");
    sinAxis.TickColor = sinAxis.Foreground;
    sinAxis.GridVisible = true;
    sinAxis.GridColor = wxColour(80, 40, 40);
    sinAxis.GridStyle = wxPENSTYLE_DOT;

    Axis cosAxis("Cosine", Axis::Vertical);
    cosAxis.Visible = true;
    cosAxis.Foreground = wxColour(80, 160, 255);
    cosAxis.LabelColor = cosAxis.Foreground;
    cosAxis.LabelVisible = true;
    cosAxis.Label = wxT("Cosine");
    cosAxis.LabelRotated = true;
    cosAxis.SetLocation(0.0);
    cosAxis.SetPosition(0.0, 0.48);
    cosAxis.SetRange(-1.2, 1.2);
    cosAxis.SetData(-1.2, 1.2);
    cosAxis.TickVisible = true;
    cosAxis.TickLabelVisible = true;
    cosAxis.TickRange = 0.5;
    cosAxis.TickLabelFormat = wxT("%.1f");
    cosAxis.TickColor = cosAxis.Foreground;
    cosAxis.GridVisible = true;
    cosAxis.GridColor = wxColour(40, 40, 80);
    cosAxis.GridStyle = wxPENSTYLE_DOT;

    m_plot->AddAxis(sampleAxis);
    m_plot->AddAxis(sinAxis);
    m_plot->AddAxis(cosAxis);

    Series sine("SineWave", Series::Doubles);
    sine.Visible = true;
    sine.Foreground = wxColour(255, 80, 80);
    sine.HorizontalAxis = wxT("Sample");
    sine.VerticalAxis = wxT("Sine");
    sine.RenderStyle = SeriesRenderStyle::Line;

    Series cosine("CosineWave", Series::Doubles);
    cosine.Visible = true;
    cosine.Foreground = wxColour(80, 160, 255);
    cosine.HorizontalAxis = wxT("Sample");
    cosine.VerticalAxis = wxT("Cosine");
    cosine.RenderStyle = SeriesRenderStyle::Line;

    m_plot->AddSeries(sine);
    m_plot->AddSeries(cosine);
    m_plot->UpdatePlot();
}

void wxPlottingRealTimeFrame::OnStart(wxCommandEvent& WXUNUSED(event))
{
    if ( m_run.load() && m_pause.load() )
    {
        m_pause.store(false);
        SetStatusText(wxT("Running"));
        m_btnPause->SetLabel(wxT("&Pause"));
        return;
    }

    StartWorker();
    SetStatusText(wxString::Format(wxT("Running — %d ms between samples"),
                                   m_rateMs.load()));
    m_btnStart->Enable(false);
    m_btnStop->Enable(true);
    m_btnPause->Enable(true);
    m_btnPause->SetLabel(wxT("&Pause"));
}

void wxPlottingRealTimeFrame::OnStop(wxCommandEvent& WXUNUSED(event))
{
    StopWorker();
    SetStatusText(wxT("Stopped"));
    m_btnStart->Enable(true);
    m_btnStop->Enable(false);
    m_btnPause->Enable(false);
    m_btnPause->SetLabel(wxT("&Pause"));
}

void wxPlottingRealTimeFrame::OnPause(wxCommandEvent& WXUNUSED(event))
{
    if ( !m_run.load() )
        return;

    const bool nowPaused = !m_pause.load();
    m_pause.store(nowPaused);
    if ( nowPaused )
    {
        SetStatusText(wxT("Paused"));
        m_btnPause->SetLabel(wxT("Resu&me"));
        m_btnStart->Enable(true); // Start also resumes
    }
    else
    {
        SetStatusText(wxString::Format(wxT("Running — %d ms between samples"),
                                       m_rateMs.load()));
        m_btnPause->SetLabel(wxT("&Pause"));
        m_btnStart->Enable(false);
    }
}

void wxPlottingRealTimeFrame::OnReset(wxCommandEvent& WXUNUSED(event))
{
    const bool wasRunning = m_run.load();
    StopWorker();
    ResetData();
    if ( wasRunning )
        StartWorker();

    m_btnStart->Enable(!m_run.load() || m_pause.load());
    m_btnStop->Enable(m_run.load());
    m_btnPause->Enable(m_run.load());
    m_btnPause->SetLabel(wxT("&Pause"));
    m_pause.store(false);

    SetStatusText(wasRunning
        ? wxString::Format(wxT("Reset — running at %d ms"), m_rateMs.load())
        : wxT("Reset — stopped"));
}

void wxPlottingRealTimeFrame::OnRate(wxSpinEvent& event)
{
    int ms = event.GetValue();
    if ( ms < 1 )
        ms = 1;
    m_rateMs.store(ms);
    if ( m_run.load() && !m_pause.load() )
    {
        SetStatusText(wxString::Format(wxT("Running — %d ms between samples"), ms));
    }
}

void wxPlottingRealTimeFrame::OnClose(wxCloseEvent& event)
{
    StopWorker();
    event.Skip();
}

void wxPlottingRealTimeFrame::StartWorker()
{
    if ( m_run.load() )
        return;

    m_pause.store(false);
    m_run.store(true);
    m_worker = std::thread(&wxPlottingRealTimeFrame::WorkerMain, this);
}

void wxPlottingRealTimeFrame::StopWorker()
{
    if ( !m_run.load() && !m_worker.joinable() )
        return;

    m_run.store(false);
    m_pause.store(false);
    if ( m_worker.joinable() )
        m_worker.join();
}

void wxPlottingRealTimeFrame::ResetData()
{
    m_sampleIndex = 0;

    if ( Series* s = m_plot->GetSeries(wxT("SineWave")) )
        s->ClearPoints();
    if ( Series* s = m_plot->GetSeries(wxT("CosineWave")) )
        s->ClearPoints();

    if ( Axis* sample = m_plot->GetAxis(wxT("Sample")) )
    {
        sample->SetRange(0.0, static_cast<double>(kSampleWindow));
        sample->SetData(0.0, static_cast<double>(kSampleWindow));
    }

    m_plot->UpdatePlot(true);
}

void wxPlottingRealTimeFrame::RefreshPlotView(std::int64_t sampleIndex)
{
    if ( !m_plot )
        return;

    Axis* sampleAxis = m_plot->GetAxis(wxT("Sample"));
    if ( sampleAxis )
    {
        // Trailing window in sample counts (not wall time).
        const double n = static_cast<double>(sampleIndex);
        const double win = static_cast<double>(kSampleWindow);
        if ( sampleIndex <= kSampleWindow )
        {
            sampleAxis->SetRange(0.0, win);
            sampleAxis->SetData(0.0, n > 0.0 ? n : win);
        }
        else
        {
            sampleAxis->SetRange(n - win, n);
            sampleAxis->SetData(0.0, n);
        }
    }

    m_plot->UpdatePlot();
}

void wxPlottingRealTimeFrame::WorkerMain()
{
    while ( m_run.load() )
    {
        const int sleepMs = m_rateMs.load();
        if ( !m_pause.load() )
        {
            const std::int64_t n = m_sampleIndex;
            // Phase from sample index only — independent of wall-clock rate.
            const double phase =
                2.0 * M_PI * (static_cast<double>(n) / static_cast<double>(kSamplesPerCycle));
            const double ySin = std::sin(phase);
            const double yCos = std::cos(phase);
            const double x = static_cast<double>(n);

            // Thread-safe point APIs; UI thread only paints.
            if ( Series* s = m_plot->GetSeries(wxT("SineWave")) )
                s->AppendPoint(wxRealPoint(x, ySin));
            if ( Series* s = m_plot->GetSeries(wxT("CosineWave")) )
                s->AppendPoint(wxRealPoint(x, yCos));

            const std::int64_t next = n + 1;
            m_sampleIndex = next;

            // Schedule repaint on the UI thread (never call UpdatePlot here).
            CallAfter([this, next]() { RefreshPlotView(next); });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs > 0 ? sleepMs : 1));
    }
}
