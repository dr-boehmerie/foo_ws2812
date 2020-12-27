/*
 * control_dialog.cpp : Implentation of the main user interface dialog box.
 *
 * Based on playback_state.cpp from the foo_sample plugin.
 *
 * Copyright (c) 2016, 2017, Michael Boehme
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met :
 *
 * (1) Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 * (2) Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and / or other materials provided with the
 *     distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "stdafx.h"
#include "resource.h"
//#include "Windowsx.h"
//#include "afxcmn.h"

#include <libPPUI/win32_utility.h>
#include <libPPUI/win32_op.h> // WIN32_OP()
#include <libPPUI/wtl-pp.h> // CCheckBox
#include <helpers/atl-misc.h> // ui_element_impl

#include <helpers/WindowPositionUtils.h>

#include "foo_ws2812.h"


class CWS2812ControlDialog : public CDialogImpl<CWS2812ControlDialog>, private play_callback_impl_base {
public:
	enum { IDD = IDD_CONTROL_DIALOG };

	BEGIN_MSG_MAP(CWS2812ControlDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
//		COMMAND_HANDLER_EX(IDC_PATTERN, EN_CHANGE, OnPatternChange)
		MSG_WM_HSCROLL(OnHScroll)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
		COMMAND_HANDLER_EX(IDC_PLAY, BN_CLICKED, OnPlayClicked)
		COMMAND_HANDLER_EX(IDC_PAUSE, BN_CLICKED, OnPauseClicked)
		COMMAND_HANDLER_EX(IDC_STOP, BN_CLICKED, OnStopClicked)
		COMMAND_HANDLER_EX(IDC_PREV, BN_CLICKED, OnPrevClicked)
		COMMAND_HANDLER_EX(IDC_NEXT, BN_CLICKED, OnNextClicked)
		COMMAND_HANDLER_EX(IDC_RAND, BN_CLICKED, OnRandClicked)
		COMMAND_HANDLER_EX(IDC_SAVE_CFG, BN_CLICKED, OnSaveCfgClicked)
		COMMAND_HANDLER_EX(IDC_START_OUTPUT, BN_CLICKED, OnStartOutputClicked)
		COMMAND_HANDLER_EX(IDC_STOP_OUTPUT, BN_CLICKED, OnStopOutputClicked)
		COMMAND_HANDLER_EX(IDC_LOG_FREQUENCY, BN_CLICKED, OnLogFreqChange)
		COMMAND_HANDLER_EX(IDC_LOG_AMPLITUDE, BN_CLICKED, OnLogAmplChange)
		COMMAND_HANDLER_EX(IDC_USE_PEAKS, BN_CLICKED, OnPeaksChange)
		COMMAND_HANDLER_EX(IDC_STYLE_0, BN_CLICKED, OnStyleClicked)
		COMMAND_HANDLER_EX(IDC_STYLE_1, BN_CLICKED, OnStyleClicked)
		COMMAND_HANDLER_EX(IDC_STYLE_2, BN_CLICKED, OnStyleClicked)
		COMMAND_HANDLER_EX(IDC_STYLE_3, BN_CLICKED, OnStyleClicked)
		COMMAND_HANDLER_EX(IDC_STYLE_4, BN_CLICKED, OnStyleClicked)
		COMMAND_HANDLER_EX(IDC_STYLE_5, BN_CLICKED, OnStyleClicked)
		COMMAND_HANDLER_EX(IDC_STYLE_6, BN_CLICKED, OnStyleClicked)
		COMMAND_HANDLER_EX(IDC_STYLE_7, BN_CLICKED, OnStyleClicked)
		COMMAND_HANDLER_EX(IDC_STYLE_8, BN_CLICKED, OnStyleClicked)
//		MSG_WM_CONTEXTMENU(OnContextMenu)
	END_MSG_MAP()
private:

	// Playback callback methods.
	void on_playback_starting(play_control::t_track_command p_command, bool p_paused) { update(); }
	void on_playback_new_track(metadb_handle_ptr p_track) { update(); }
	void on_playback_stop(play_control::t_stop_reason p_reason) { update(); }
	void on_playback_seek(double p_time) { update(); }
	void on_playback_pause(bool p_state) { update(); }
	void on_playback_edited(metadb_handle_ptr p_track) { update(); }
	void on_playback_dynamic_info(const file_info & p_info) { update(); }
	void on_playback_dynamic_info_track(const file_info & p_info) { update(); }
	void on_playback_time(double p_time) { update(); }
	void on_volume_change(float p_new_val) {}

	void update();

	void UpdateFrequencyAmplitudeSlider();

//	void OnPatternChange(UINT, int, CWindow);
	void OnCancel(UINT, int, CWindow);

	void EnableButton(int id, bool enable);

	void OnPlayClicked(UINT, int, CWindow) { m_playback_control->start(); }
	void OnStopClicked(UINT, int, CWindow) { m_playback_control->stop(); }
	void OnPauseClicked(UINT, int, CWindow) { m_playback_control->toggle_pause(); }
	void OnPrevClicked(UINT, int, CWindow) { m_playback_control->start(playback_control::track_command_prev); }
	void OnNextClicked(UINT, int, CWindow) { m_playback_control->start(playback_control::track_command_next); }
	void OnRandClicked(UINT, int, CWindow) { m_playback_control->start(playback_control::track_command_rand); }

	void OnOutputClicked(UINT, int, CWindow) { ToggleOutput(); }

	void OnLogFreqChange(UINT, int, CWindow);
	void OnLogAmplChange(UINT, int, CWindow);
	void OnPeaksChange(UINT, int, CWindow);
	void OnSaveCfgClicked(UINT, int, CWindow);

	void OnStyleClicked(UINT, int, CWindow);

	void OnStartOutputClicked(UINT, int, CWindow);
	void OnStopOutputClicked(UINT, int, CWindow);

//	void OnContextMenu(CWindow wnd, CPoint point);

	BOOL OnInitDialog(CWindow, LPARAM);
//	void OnHScroll(UINT, UINT, CScrollBar);
	BOOL OnHScroll(int x, short y, HWND wnd);

	LPCTSTR m_text_brightness = L"Brightness";
	LPCTSTR m_text_interval = L"Update Interval";
	LPCTSTR m_text_freq_min = L"Min. Frequency";
	LPCTSTR m_text_freq_max = L"Max. Frequency";
	LPCTSTR m_text_ampl_min = L"Min. Amplitude";
	LPCTSTR m_text_ampl_max = L"Max. Amplitude";
	LPCTSTR m_text_offset = L"Offset";
	LPCTSTR m_text_amplitude = L"Amplitude";
	LPCTSTR m_text_gain = L"Gain";

	unsigned int m_interval_step = 10;

	CTrackBarCtrl m_slider_brightness;
	CTrackBarCtrl m_slider_interval;
	CTrackBarCtrl m_slider_freq_min;
	CTrackBarCtrl m_slider_freq_max;
	CTrackBarCtrl m_slider_ampl_min;
	CTrackBarCtrl m_slider_ampl_max;

	titleformat_object::ptr m_script;

	static_api_ptr_t<playback_control> m_playback_control;
};

void CWS2812ControlDialog::OnLogFreqChange(UINT code, int id, CWindow hwnd)
{
	CCheckBox	pCheckBox = GetDlgItem(IDC_LOG_FREQUENCY);
	unsigned int val;

	if (pCheckBox.GetCheck())
		val = 1;
	else
		val = 0;

	SetScaling(val, -1, -1);
//	SetCfgLogFrequency(val);
}

void CWS2812ControlDialog::OnLogAmplChange(UINT code, int id, CWindow hwnd)
{
	CCheckBox	pCheckBox = GetDlgItem(IDC_LOG_AMPLITUDE);
	unsigned int val;

	if (pCheckBox.GetCheck())
		val = 1;
	else
		val = 0;

	SetScaling(-1, val, -1);
//	SetCfgLogAmplitude(val);
}

void CWS2812ControlDialog::OnPeaksChange(UINT code, int id, CWindow hwnd)
{
	CCheckBox	pCheckBox = GetDlgItem(IDC_USE_PEAKS);
	unsigned int val;

	if (pCheckBox.GetCheck())
		val = 1;
	else
		val = 0;

	SetScaling(-1, -1, val);
//	SetCfgPeakValues(val);
}

#if 0
void CWS2812ControlDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
{
	CTrackBarCtrl* pSlider = reinterpret_cast<CTrackBarCtrl*>(pScrollBar);

	// Check which slider sent the notification  
	if (pSlider == &m_slider_brightness)
	{
	}
	else if (pSlider == &m_slider_interval)
	{
	}

	// Check what happened  
	switch (nSBCode)
	{
	case TB_LINEUP:
	case TB_LINEDOWN:
	case TB_PAGEUP:
	case TB_PAGEDOWN:
	case TB_THUMBPOSITION:
	case TB_TOP:
	case TB_BOTTOM:
	case TB_THUMBTRACK:
	case TB_ENDTRACK:
	default:
		break;
	}

	//...  
}
#else
BOOL CWS2812ControlDialog::OnHScroll(int nSBCode, short nPos, HWND hwnd)
//void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar) {
{
	//CScrollBar	*pScrollbar = (CScrollBar *)hwnd;

	//int pos = pScrollbar->GetScrollPos();

	WCHAR	text[100];
	bool	ledTest = GetLedTestMode();

	// 0 <= nPos <= 100
	if (hwnd == m_slider_brightness) {
		int val = m_slider_brightness.GetPos();
		unsigned int brightness;

		//_stprintf_s(text, L"Brightness: %i %i %i %%", nSBCode, nPos, val);
		_stprintf_s(text, L"%s: %i %%", m_text_brightness, val);
		SetDlgItemText(IDC_TXT_BRIGHTNESS, text);

		SetBrightness(val);

		if (nSBCode != TB_THUMBTRACK) {
			if (GetBrightness(brightness))
				SetCfgBrightness(brightness);
		}
	}
	else if (hwnd == m_slider_interval) {
		int val = m_slider_interval.GetPos() * m_interval_step;
		unsigned int interval;

		//_stprintf_s(text, L"Update Interval: %i %i %i ms", nSBCode, nPos, val);
		_stprintf_s(text, L"%s: %i ms", m_text_interval, val);
		SetDlgItemText(IDC_TXT_INTERVAL, text);

		SetInterval(val);

		if (nSBCode != TB_THUMBTRACK) {
			if (GetInterval(interval))
				SetCfgUpdateInterval(interval);
		}
	}
	else if (hwnd == m_slider_freq_min) {
		int val = m_slider_freq_min.GetPos();

		if (ledTest) {
			_stprintf_s(text, L"Red: %i", val);
			SetDlgItemText(IDC_TXT_FREQ_MIN, text);

			SetLedTestVal(0, val);
		}
		else {
			_stprintf_s(text, L"%s: %i Hz", m_text_freq_min, val);
			SetDlgItemText(IDC_TXT_FREQ_MIN, text);

			SetFrequencyMinMax(val, INT_MAX);

			if (nSBCode != TB_THUMBTRACK)
				SaveFrequencyMinMax();
		}
	}
	else if (hwnd == m_slider_freq_max) {
		int val = m_slider_freq_max.GetPos();

		if (ledTest) {
			_stprintf_s(text, L"Green: %i", val);
			SetDlgItemText(IDC_TXT_FREQ_MAX, text);

			SetLedTestVal(1, val);
		}
		else {
			_stprintf_s(text, L"%s: %i Hz", m_text_freq_max, val);
			SetDlgItemText(IDC_TXT_FREQ_MAX, text);

			SetFrequencyMinMax(INT_MIN, val);

			if (nSBCode != TB_THUMBTRACK)
				SaveFrequencyMinMax();
		}
	}
	else if (hwnd == m_slider_ampl_min) {
		int val = m_slider_ampl_min.GetPos();

		if (ledTest) {
			_stprintf_s(text, L"Blue: %i", val);
			SetDlgItemText(IDC_TXT_AMPL_MIN, text);

			SetLedTestVal(2, val);
		}
		else {
			if (GetMinAmplitudeIsOffset()) {
				// this value is used as offset
				_stprintf_s(text, L"%s: %.2f", m_text_offset, (double)val / (double)ws2812::offset_oscilloscope_div);
			}
			else {
				_stprintf_s(text, L"%s: %i dB", m_text_ampl_min, val);
			}
			SetDlgItemText(IDC_TXT_AMPL_MIN, text);

			SetAmplitudeMinMax(val, INT_MAX);

			if (nSBCode != TB_THUMBTRACK)
				SaveAmplitudeMinMax();
		}
	}
	else if (hwnd == m_slider_ampl_max) {
		int val = m_slider_ampl_max.GetPos();

		if (ledTest) {
			_stprintf_s(text, L"White: %i", val);
			SetDlgItemText(IDC_TXT_AMPL_MAX, text);

			SetLedTestVal(3, val);
		}
		else {
			if (GetMaxAmplitudeIsGain()) {
				// this value is used as gain
				_stprintf_s(text, L"%s: %.2f", m_text_gain, (double)val / (double)ws2812::gain_oscilloscope_div);
			}
			else {
				_stprintf_s(text, L"%s: %i dB", m_text_ampl_max, val);
			}
			SetDlgItemText(IDC_TXT_AMPL_MAX, text);

			SetAmplitudeMinMax(INT_MIN, val);

			if (nSBCode != TB_THUMBTRACK)
				SaveAmplitudeMinMax();
		}
	}

	if (nSBCode == TB_ENDTRACK) {
		// mouse up
		// nPos == 0

	}
	else if (nSBCode == TB_PAGEDOWN) {
		// decrement
		// nPos == 0

	}
	else if (nSBCode == TB_PAGEUP) {
		// increment
		// nPos == 0

	}
	else if (nSBCode == TB_THUMBTRACK) {
		// position changed with mouse
		// current position in nPos

	}
	else {

	}

	return TRUE;
}

#endif


void CWS2812ControlDialog::UpdateFrequencyAmplitudeSlider(void)
{
	WCHAR text[100];
	int fmin = 0, fmax = 0;
	int amin = 0, amax = 0;

	if (GetLedTestMode()) {
		int min = 0;
		int max = 255;
		int val = 0;

		m_slider_freq_min.SetRange(min, max, TRUE);
		m_slider_freq_min.SetTicFreq((max - min) / 10);
		m_slider_freq_min.SetPos(val);

		m_slider_freq_max.SetRange(min, max, TRUE);
		m_slider_freq_max.SetTicFreq((max - min) / 10);
		m_slider_freq_max.SetPos(val);

		m_slider_ampl_min.SetRange(min, max, TRUE);
		m_slider_ampl_min.SetTicFreq((max - min) / 10);
		m_slider_ampl_min.SetPos(val);

		m_slider_ampl_max.SetRange(min, max, TRUE);
		m_slider_ampl_max.SetTicFreq((max - min) / 10);
		m_slider_ampl_max.SetPos(val);

		_stprintf_s(text, L"Red: %i", val);
		SetDlgItemText(IDC_TXT_FREQ_MIN, text);

		_stprintf_s(text, L"Green: %i", val);
		SetDlgItemText(IDC_TXT_FREQ_MAX, text);

		_stprintf_s(text, L"Blue: %i", val);
		SetDlgItemText(IDC_TXT_AMPL_MIN, text);

		_stprintf_s(text, L"White: %i", val);
		SetDlgItemText(IDC_TXT_AMPL_MAX, text);
	}
	else {
		// frequency and amplitude scaling
		GetFrequencyMinMax(fmin, fmax);

		m_slider_freq_min.SetRange(ws2812::frequency_min, ws2812::frequency_max, TRUE);
		m_slider_freq_min.SetTicFreq((ws2812::frequency_max - ws2812::frequency_min) / 10);
		m_slider_freq_min.SetPos(fmin);

		_stprintf_s(text, L"%s: %u Hz", m_text_freq_min, fmin);
		SetDlgItemText(IDC_TXT_FREQ_MIN, text);

		m_slider_freq_max.SetRange(ws2812::frequency_min, ws2812::frequency_max, TRUE);
		m_slider_freq_max.SetTicFreq((ws2812::frequency_max - ws2812::frequency_min) / 10);
		m_slider_freq_max.SetPos(fmax);

		_stprintf_s(text, L"%s: %u Hz", m_text_freq_max, fmax);
		SetDlgItemText(IDC_TXT_FREQ_MAX, text);

		GetAmplitudeMinMax(amin, amax);

		if (GetMinAmplitudeIsOffset()) {
			// Oscilloscope: offset: -100 ... 100 -> -1.0 ... 1.0
			m_slider_ampl_min.SetRange(ws2812::offset_oscilloscope_min, ws2812::offset_oscilloscope_max, TRUE);
			m_slider_ampl_min.SetTicFreq((ws2812::offset_oscilloscope_max - ws2812::offset_oscilloscope_min) / 10);
			m_slider_ampl_min.SetPos(amin);

			_stprintf_s(text, L"%s: %.2f", m_text_offset, (double)amin / (double)ws2812::offset_oscilloscope_div);
			SetDlgItemText(IDC_TXT_AMPL_MIN, text);
		}
		else {
			m_slider_ampl_min.SetRange(ws2812::amplitude_min, ws2812::amplitude_max, TRUE);
			m_slider_ampl_min.SetTicFreq((ws2812::amplitude_max - ws2812::amplitude_min) / 10);
			m_slider_ampl_min.SetPos(amin);

			_stprintf_s(text, L"%s: %i dB", m_text_ampl_min, amin);
			SetDlgItemText(IDC_TXT_AMPL_MIN, text);
		}

		if (GetMaxAmplitudeIsGain()) {
			// gain: 1 ... 100 -> 0.1 ... 10.0
			m_slider_ampl_max.SetRange(ws2812::gain_oscilloscope_min, ws2812::gain_oscilloscope_max, TRUE);
			m_slider_ampl_max.SetTicFreq((ws2812::gain_oscilloscope_max - ws2812::gain_oscilloscope_min) / 10);
			m_slider_ampl_max.SetPos(amax);

			_stprintf_s(text, L"%s: %.2f", m_text_gain, (double)amax / (double)ws2812::gain_oscilloscope_div);
			SetDlgItemText(IDC_TXT_AMPL_MAX, text);

			// frequency limits not applicable
			m_slider_freq_min.EnableWindow(false);
			m_slider_freq_max.EnableWindow(false);
		}
		else {
			m_slider_ampl_max.SetRange(ws2812::amplitude_min, ws2812::amplitude_max, TRUE);
			m_slider_ampl_max.SetTicFreq((ws2812::amplitude_max - ws2812::amplitude_min) / 10);
			m_slider_ampl_max.SetPos(amax);

			_stprintf_s(text, L"%s: %i dB", m_text_ampl_max, amax);
			SetDlgItemText(IDC_TXT_AMPL_MAX, text);

			// frequency limits enabled
			m_slider_freq_min.EnableWindow(true);
			m_slider_freq_max.EnableWindow(true);
		}
	}
}

void CWS2812ControlDialog::OnStyleClicked(UINT code, int id, CWindow hwnd)
{
	LRESULT r = SendDlgItemMessage(id, BM_GETCHECK, 0, 0);

	unsigned int style = 0;
	const char *colors = nullptr;

	switch (id)
	{
	case IDC_STYLE_0:	style = 0;		break;
	case IDC_STYLE_1:	style = 1;		break;
	case IDC_STYLE_2:	style = 2;		break;
	case IDC_STYLE_3:	style = 3;		break;
	case IDC_STYLE_4:	style = 4;		break;
	case IDC_STYLE_5:	style = 5;		break;
	case IDC_STYLE_6:	style = 6;		break;
	case IDC_STYLE_7:	style = 7;		break;
	case IDC_STYLE_8:	style = 8;		break;
	default:
		r = false;
		break;
	}

	if (r) {
		// save selected line style
		SetStyle(style);

		if (GetLineStyle(style)) {
			// init color tab
			switch (static_cast<ws2812_style>(style))
			{
			default:
			case ws2812_style::spectrum_simple:			colors = GetCfgSpectrumColors();		break;
			case ws2812_style::spectrum_green_red_bars:	colors = GetCfgSpectrumBarColors();		break;
			case ws2812_style::spectrum_fire_lines:		colors = GetCfgSpectrumFireColors();	break;
			case ws2812_style::spectrogram_horizontal:	colors = GetCfgSpectrogramColors();		break;
			case ws2812_style::spectrogram_vertical:	colors = GetCfgSpectrogramColors();		break;
			case ws2812_style::oscilloscope_yt:			colors = GetCfgOscilloscopeColors();	break;
			case ws2812_style::oscilloscope_xy:			colors = GetCfgOscilloscopeColors();	break;
			case ws2812_style::oscillogram_horizontal:	colors = GetCfgOscilloscopeColors();	break;
			case ws2812_style::oscillogram_vertical:	colors = GetCfgOscilloscopeColors();	break;
			}

			if (colors)
				InitColorTab(colors);

			SetCfgLineStyle(style);
		}
	}

	// frequency and amplitude scaling changes
	UpdateFrequencyAmplitudeSlider();

}

void CWS2812ControlDialog::EnableButton(int id, bool enable)
{
	LRESULT returnCode;
	HWND bnHwnd = GetDlgItem(id);

	if (::IsWindow(bnHwnd)) {
		LONG style = ::GetWindowLong(bnHwnd, GWL_STYLE);

		if (enable)
			style &= ~WS_DISABLED;
		else
			style |= WS_DISABLED;

		::SetWindowLong(bnHwnd, GWL_STYLE, style);

		returnCode = ::SendMessage(bnHwnd, WM_ENABLE, FALSE, 0);
	}
}

void CWS2812ControlDialog::OnStartOutputClicked(UINT code, int id, CWindow hwnd)
{
	if (StartOutput()) {
		// enable "Stop" button
		EnableButton(IDC_STOP_OUTPUT, true);

		// disable "Start" button
		EnableButton(IDC_START_OUTPUT, false);
	}
	else {

	}
}

void CWS2812ControlDialog::OnStopOutputClicked(UINT code, int id, CWindow hwnd)
{
	if (StopOutput()) {
		// disable "Stop" button
		EnableButton(IDC_STOP_OUTPUT, false);

		// enable "Start" button
		EnableButton(IDC_START_OUTPUT, true);
	}
	else {
		// disable "Stop" button
		EnableButton(IDC_STOP_OUTPUT, false);

		// enable "Start" button
		EnableButton(IDC_START_OUTPUT, true);
	}
}

void CWS2812ControlDialog::OnSaveCfgClicked(UINT, int, CWindow)
{

}

void CWS2812ControlDialog::OnCancel(UINT, int, CWindow) {
	DestroyWindow();
}

//void CWS2812ControlDialog::OnPatternChange(UINT, int, CWindow) {
//	m_script.release(); // pattern has changed, force script recompilation
//	update();
//}

BOOL CWS2812ControlDialog::OnInitDialog(CWindow, LPARAM) {
	update();
//	SetDlgItemText(IDC_PATTERN, _T("%codec% | %bitrate% kbps | %samplerate% Hz | %channels% | %playback_time%[ / %length%]$if(%ispaused%, | paused,)"));
	::ShowWindowCentered(*this, GetParent()); // Function declared in SDK helpers.

	// get slider controls
	m_slider_brightness = GetDlgItem(IDC_SLIDER_BRIGHTNESS);
	m_slider_interval = GetDlgItem(IDC_SLIDER_INTERVAL);
	m_slider_freq_min = GetDlgItem(IDC_SLIDER_FREQ_MIN);
	m_slider_freq_max = GetDlgItem(IDC_SLIDER_FREQ_MAX);
	m_slider_ampl_min = GetDlgItem(IDC_SLIDER_AMPL_MIN);
	m_slider_ampl_max = GetDlgItem(IDC_SLIDER_AMPL_MAX);

	unsigned int	style;
	unsigned int	val;
	LRESULT			r;
	WCHAR			text[100];

	style = 0;
	GetLineStyle(style);
	switch (style)
	{
	case 0:		r = SendDlgItemMessage(IDC_STYLE_0, BM_SETCHECK, BST_CHECKED, 0);		break;
	case 1:		r = SendDlgItemMessage(IDC_STYLE_1, BM_SETCHECK, BST_CHECKED, 0);		break;
	case 2:		r = SendDlgItemMessage(IDC_STYLE_2, BM_SETCHECK, BST_CHECKED, 0);		break;
	case 3:		r = SendDlgItemMessage(IDC_STYLE_3, BM_SETCHECK, BST_CHECKED, 0);		break;
	case 4:		r = SendDlgItemMessage(IDC_STYLE_4, BM_SETCHECK, BST_CHECKED, 0);		break;
	case 5:		r = SendDlgItemMessage(IDC_STYLE_5, BM_SETCHECK, BST_CHECKED, 0);		break;
	case 6:		r = SendDlgItemMessage(IDC_STYLE_6, BM_SETCHECK, BST_CHECKED, 0);		break;
	case 7:		r = SendDlgItemMessage(IDC_STYLE_7, BM_SETCHECK, BST_CHECKED, 0);		break;
	case 8:		r = SendDlgItemMessage(IDC_STYLE_8, BM_SETCHECK, BST_CHECKED, 0);		break;
	default:
		break;
	}

	int logFreq = 0, logAmpl = 0, peaks = 0;
	GetScaling(logFreq, logAmpl, peaks);

	if (logFreq > 0) {
		r = SendDlgItemMessage(IDC_LOG_FREQUENCY, BM_SETCHECK, BST_CHECKED, 0);
	}
	else {
		r = SendDlgItemMessage(IDC_LOG_FREQUENCY, BM_SETCHECK, BST_UNCHECKED, 0);
	}

	if (logAmpl > 0) {
		r = SendDlgItemMessage(IDC_LOG_AMPLITUDE, BM_SETCHECK, BST_CHECKED, 0);
	}
	else {
		r = SendDlgItemMessage(IDC_LOG_AMPLITUDE, BM_SETCHECK, BST_UNCHECKED, 0);
	}

	if (peaks > 0) {
		r = SendDlgItemMessage(IDC_USE_PEAKS, BM_SETCHECK, BST_CHECKED, 0);
	}
	else {
		r = SendDlgItemMessage(IDC_USE_PEAKS, BM_SETCHECK, BST_UNCHECKED, 0);
	}

	val = 0;
	GetInterval(val);
	m_slider_interval.SetRange(ws2812::timerInterval_min / m_interval_step, ws2812::timerInterval_max / m_interval_step, TRUE);
	m_slider_interval.SetTicFreq((ws2812::timerInterval_max - ws2812::timerInterval_min) / (10 * m_interval_step));
	m_slider_interval.SetPos(val / m_interval_step);

	_stprintf_s(text, L"%s: %u ms", m_text_interval, val);
	SetDlgItemText(IDC_TXT_INTERVAL, text);

	val = 0;
	GetBrightness(val);
	m_slider_brightness.SetRange(ws2812::brightness_min, ws2812::brightness_max, TRUE);
	m_slider_brightness.SetTicFreq((ws2812::brightness_max - ws2812::brightness_min) / 10);
	m_slider_brightness.SetPos(val);

	_stprintf_s(text, L"%s: %u %%", m_text_brightness, val);
	SetDlgItemText(IDC_TXT_BRIGHTNESS, text);

	// frequency and amplitude scaling
	UpdateFrequencyAmplitudeSlider();

	if (GetOutputState()) {
		// output is running
		// enable "Stop" button
		EnableButton(IDC_STOP_OUTPUT, true);

		// disable "Start" button
		EnableButton(IDC_START_OUTPUT, false);
	}
	else {
		// output is stopped
		// disable "Stop" button
		EnableButton(IDC_STOP_OUTPUT, false);

		// enable "Start" button
		EnableButton(IDC_START_OUTPUT, true);
	}

	return TRUE;
}

void CWS2812ControlDialog::update() {
	if (m_script.is_empty()) {
		pfc::string8 pattern = "%codec% | %bitrate% kbps | %samplerate% Hz | %channels% | %playback_time%[ / %length%]$if(%ispaused%, | paused,)";
	//	uGetDlgItemText(*this, IDC_PATTERN, pattern);
		static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(m_script, pattern);
	}
	pfc::string_formatter state;
	if (m_playback_control->playback_format_title(NULL, state, m_script, NULL, playback_control::display_level_all)) {
		//Succeeded already.
		uint64_t errCnt, busyCnt, okCnt;
		if (GetStatistics(errCnt, busyCnt, okCnt)) {
			pfc::string8 statistics = "";

			char text[200] = "";
			//sprintf_s(text, " | errCnt %llu; busy %llu; ok %llu", errCnt, busyCnt, okCnt);
			sprintf_s (text, " | Ok %llu | Err %llu", okCnt, errCnt);

			statistics.set_string(text);

			state += statistics;
		}
	}
	else if (m_playback_control->is_playing()) {
		//Starting playback but not done opening the first track yet.
		state = "Opening...";
	}
	else {
		state = "Stopped.";
	}
	uSetDlgItemText(*this, IDC_STATE, state);
}

#if 0
void CWS2812ControlDialog::OnContextMenu(CWindow wnd, CPoint point) {
	try {
		if (wnd == GetDlgItem(IDC_CONTEXTMENU)) {

			// handle the context menu key case - center the menu
			if (point == CPoint(-1, -1)) {
				CRect rc;
				WIN32_OP(wnd.GetWindowRect(&rc));
				point = rc.CenterPoint();
			}

			metadb_handle_list items;

			{ // note: we would normally just use contextmenu_manager::init_context_now_playing(), but we go the "make the list ourselves" route to demonstrate how to invoke the menu for arbitrary items.
				metadb_handle_ptr item;
				if (m_playback_control->get_now_playing(item)) items += item;
			}

			CMenuDescriptionHybrid menudesc(*this); //this class manages all the voodoo necessary for descriptions of our menu items to show in the status bar.

			static_api_ptr_t<contextmenu_manager> api;
			CMenu menu;
			WIN32_OP(menu.CreatePopupMenu());
			enum {
				ID_TESTCMD = 1,
				ID_CM_BASE,
			};
			menu.AppendMenu(MF_STRING, ID_TESTCMD, _T("Test command"));
			menudesc.Set(ID_TESTCMD, "This is a test command.");
			menu.AppendMenu(MF_SEPARATOR);

			if (items.get_count() > 0) {
				api->init_context(items, 0);
				api->win32_build_menu(menu, ID_CM_BASE, ~0);
				menudesc.SetCM(api.get_ptr(), ID_CM_BASE, ~0);
			}
			else {
				menu.AppendMenu(MF_STRING | MF_GRAYED | MF_DISABLED, (UINT)0, _T("No items selected"));
			}

			int cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, menudesc, 0);
			if (cmd > 0) {
				if (cmd >= ID_CM_BASE) {
					api->execute_by_id(cmd - ID_CM_BASE);
				}
				else switch (cmd) {
				case ID_TESTCMD:
					popup_message::g_show("Blah!", "Test");
					break;
				}
			}

		}
	}
	catch (std::exception const & e) {
		console::complain("Context menu failure", e); //rare
	}
}
#endif

void RunWS2812ControlDialog() {
	try {
		// ImplementModelessTracking registers our dialog to receive dialog messages thru main app loop's IsDialogMessage().
		// CWindowAutoLifetime creates the window in the constructor (taking the parent window as a parameter) and deletes the object when the window has been destroyed (through WTL's OnFinalMessage).
		new CWindowAutoLifetime<ImplementModelessTracking<CWS2812ControlDialog> >(core_api::get_main_window());
	}
	catch (std::exception const & e) {
		popup_message::g_complain("Dialog creation failure", e);
	}
}
