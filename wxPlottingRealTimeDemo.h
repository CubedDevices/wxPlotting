/////////////////////////////////////////////////////////////////////////////
// Name:        wxPlottingRealTimeDemo.h
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
#ifndef WXPLOTTINGREALTIMEDEMO_H
#define WXPLOTTINGREALTIMEDEMO_H

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/frame.h"
    #include "wx/button.h"
    #include "wx/spinctrl.h"
    #include "wx/stattext.h"
    #include "wx/panel.h"
#endif

#include <atomic>
#include <thread>

class wxPlotting;

/**
 * @file wxPlottingRealTimeDemo.h
 * @brief Minimal real-time demo for @ref wxPlotting (worker thread + streaming series).
 */

/**
 * @class wxPlottingRealTimeApp
 * @brief Application entry for the real-time plotting demo.
 */
class wxPlottingRealTimeApp : public wxApp
{
public:
    bool OnInit() override;
};

wxDECLARE_APP(wxPlottingRealTimeApp);

/**
 * @class wxPlottingRealTimeFrame
 * @brief Hosts a plot updated by a background thread with Start/Stop/Pause/Reset.
 *
 * @details One horizontal Time axis and two vertical value axes (Sine / Cosine).
 *          A worker thread appends samples at a user-configurable interval (ms)
 *          and schedules @ref wxPlotting::UpdatePlot on the UI thread.
 */
class wxPlottingRealTimeFrame : public wxFrame
{
public:
    wxPlottingRealTimeFrame();
    ~wxPlottingRealTimeFrame() override;

private:
    enum
    {
        ID_START = wxID_HIGHEST + 1,
        ID_STOP,
        ID_PAUSE,
        ID_RESET,
        ID_RATE
    };

    void BuildPlot();
    void BuildControls(wxWindow* parent);

    void OnStart(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnPause(wxCommandEvent& event);
    void OnReset(wxCommandEvent& event);
    void OnRate(wxSpinEvent& event);
    void OnClose(wxCloseEvent& event);

    /** Start worker if not running; clear pause. */
    void StartWorker();
    /** Signal stop and join the worker. */
    void StopWorker();
    /** Clear series, reset time, refresh axes/plot (UI thread). */
    void ResetData();
    /** Scroll Time range to a trailing window and repaint (UI thread). */
    void RefreshPlotView(double timeSec);

    void WorkerMain();

    wxPlotting*  m_plot = nullptr;
    wxButton*    m_btnStart = nullptr;
    wxButton*    m_btnStop = nullptr;
    wxButton*    m_btnPause = nullptr;
    wxButton*    m_btnReset = nullptr;
    wxSpinCtrl*  m_rateSpin = nullptr;

    std::thread       m_worker;
    std::atomic<bool> m_run{false};
    std::atomic<bool> m_pause{false};
    std::atomic<int>  m_rateMs{50};

    /** Next sample X (seconds). Written by worker; reset only while stopped. */
    double m_timeSec = 0.0;

    static constexpr double kTimeWindowSec = 10.0;
    static constexpr double kWaveHz = 0.5; // 0.5 Hz sine/cosine

    wxDECLARE_EVENT_TABLE();
};

#endif // WXPLOTTINGREALTIMEDEMO_H
