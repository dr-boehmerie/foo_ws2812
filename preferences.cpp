/*
 * preferences.cpp : Implements the settings page found in preferences.
 *
 * Based on preferences.cpp from the foo_sample plugin.
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
#include "Windowsx.h"

// Sample preferences interface: two meaningless configuration settings accessible through a preferences page and one accessible through advanced preferences.

#include "foo_ws2812.h"


// These GUIDs identify the variables within our component's configuration file.
// {FDCD346D-3B1B-413C-9CAE-3A221AA6993B}
static const GUID guid_cfg_matrixRows = { 0xfdcd346d, 0x3b1b, 0x413c,{ 0x9c, 0xae, 0x3a, 0x22, 0x1a, 0xa6, 0x99, 0x3b } };
// {BF7C1CF7-8DD7-4394-822B-04D42047ED6F}
static const GUID guid_cfg_matrixCols = { 0xbf7c1cf7, 0x8dd7, 0x4394,{ 0x82, 0x2b, 0x4, 0xd4, 0x20, 0x47, 0xed, 0x6f } };
// {577DAA96-EE1F-4C2A-8EB6-A908B7686480}
static const GUID guid_cfg_brightness = { 0x577daa96, 0xee1f, 0x4c2a,{ 0x8e, 0xb6, 0xa9, 0x8, 0xb7, 0x68, 0x64, 0x80 } };
// {AF4A9553-2AF3-4735-AE1D-0683F9C94DF6}
static const GUID guid_cfg_updateInterval = { 0xaf4a9553, 0x2af3, 0x4735,{ 0xae, 0x1d, 0x6, 0x83, 0xf9, 0xc9, 0x4d, 0xf6 } };
// {142E4943-994C-4F2E-B992-81597C77AFD9}
static const GUID guid_cfg_comPort = { 0x142e4943, 0x994c, 0x4f2e,{ 0xb9, 0x92, 0x81, 0x59, 0x7c, 0x77, 0xaf, 0xd9 } };
// {01B573BD-9C6F-4211-85F8-82D999CE5A80}
static const GUID guid_cfg_comBaudrate = { 0x1b573bd, 0x9c6f, 0x4211,{ 0x85, 0xf8, 0x82, 0xd9, 0x99, 0xce, 0x5a, 0x80 } };
// {556A3C73-B652-4FAB-BCB2-1340AAE742F6}
static const GUID guid_cfg_startLed = { 0x556a3c73, 0xb652, 0x4fab,{ 0xbc, 0xb2, 0x13, 0x40, 0xaa, 0xe7, 0x42, 0xf6 } };
// {D4F9F718-E38B-4847-93F0-C6BC6008987C}
static const GUID guid_cfg_ledDirection = { 0xd4f9f718, 0xe38b, 0x4847,{ 0x93, 0xf0, 0xc6, 0xbc, 0x60, 0x8, 0x98, 0x7c } };
// {5DA05EE9-A90D-4F65-B0A8-6BAA5F0EF400}
static const GUID guid_cfg_ledColors = { 0x5da05ee9, 0xa90d, 0x4f65,{ 0xb0, 0xa8, 0x6b, 0xaa, 0x5f, 0xe, 0xf4, 0x0 } };
// {3B6337F1-538D-480A-84A3-5527FCE35A83}
static const GUID guid_cfg_lineStyle = { 0x3b6337f1, 0x538d, 0x480a,{ 0x84, 0xa3, 0x55, 0x27, 0xfc, 0xe3, 0x5a, 0x83 } };
// {5F9CF2D6-331D-4EE1-9845-BB3E72CD08D3}
static const GUID guid_cfg_logFrequency = { 0x5f9cf2d6, 0x331d, 0x4ee1,{ 0x98, 0x45, 0xbb, 0x3e, 0x72, 0xcd, 0x8, 0xd3 } };
// {62F8A4AB-3A4F-41FA-B4C1-112BCE13FEFF}
static const GUID guid_cfg_logAmplitude = { 0x62f8a4ab, 0x3a4f, 0x41fa,{ 0xb4, 0xc1, 0x11, 0x2b, 0xce, 0x13, 0xfe, 0xff } };
// {B3E9A0FF-00CA-4948-A35E-F56D8B8D8CF2}
static const GUID guid_cfg_peakValues = { 0xb3e9a0ff, 0xca, 0x4948,{ 0xa3, 0x5e, 0xf5, 0x6d, 0x8b, 0x8d, 0x8c, 0xf2 } };
// {CC865229-9DA4-431F-BC9A-3F7B03B51E03}
static const GUID guid_cfg_spectrumColors = { 0xcc865229, 0x9da4, 0x431f,{ 0xbc, 0x9a, 0x3f, 0x7b, 0x3, 0xb5, 0x1e, 0x3 } };
// {396CC84D-1E1B-403D-9916-5F9DB1932654}
static const GUID guid_cfg_spectrogramColors = { 0x396cc84d, 0x1e1b, 0x403d,{ 0x99, 0x16, 0x5f, 0x9d, 0xb1, 0x93, 0x26, 0x54 } };
// {0E749577-B1F0-45A2-AC8C-DC977750AAEE}
static const GUID guid_cfg_oscilloscopeColors = { 0xe749577, 0xb1f0, 0x45a2,{ 0xac, 0x8c, 0xdc, 0x97, 0x77, 0x50, 0xaa, 0xee } };
// {90A0F093-3114-4F1A-8AF7-301A80C86826}
static const GUID guid_cfg_spectrumBarColors = { 0x90a0f093, 0x3114, 0x4f1a,{ 0x8a, 0xf7, 0x30, 0x1a, 0x80, 0xc8, 0x68, 0x26 } };
// {7FA96B16-3590-4BB1-B547-1F5C3DD7E0EF}
static const GUID guid_cfg_spectrumFireColors = { 0x7fa96b16, 0x3590, 0x4bb1,{ 0xb5, 0x47, 0x1f, 0x5c, 0x3d, 0xd7, 0xe0, 0xef } };
// {CCC8A326-E6CE-4DEF-B14C-4395BD4B1965}
static const GUID guid_cfg_spectrumAmplitudeMin = { 0xccc8a326, 0xe6ce, 0x4def,{ 0xb1, 0x4c, 0x43, 0x95, 0xbd, 0x4b, 0x19, 0x65 } };
// {CC47051B-9783-4168-A57B-E7FC6BE4A482}
static const GUID guid_cfg_spectrumAmplitudeMax = { 0xcc47051b, 0x9783, 0x4168,{ 0xa5, 0x7b, 0xe7, 0xfc, 0x6b, 0xe4, 0xa4, 0x82 } };
// {DE0564D4-631C-49FF-99E8-0C7ADF04CC6F}
static const GUID guid_cfg_spectrogramAmplitudeMin = { 0xde0564d4, 0x631c, 0x49ff,{ 0x99, 0xe8, 0xc, 0x7a, 0xdf, 0x4, 0xcc, 0x6f } };
// {1EF1856E-DBD6-4B06-B89A-C09B842EC362}
static const GUID guid_cfg_spectrogramAmplitudeMax = { 0x1ef1856e, 0xdbd6, 0x4b06,{ 0xb8, 0x9a, 0xc0, 0x9b, 0x84, 0x2e, 0xc3, 0x62 } };
// {986AEF83-3597-4643-AF91-30FE49603FA2}
static const GUID guid_cfg_oscilloscopeOffset = { 0x986aef83, 0x3597, 0x4643,{ 0xaf, 0x91, 0x30, 0xfe, 0x49, 0x60, 0x3f, 0xa2 } };
// {98275E99-75F1-4E8C-845A-165080F40A9F}
static const GUID guid_cfg_oscilloscopeAmplitude = { 0x98275e99, 0x75f1, 0x4e8c,{ 0x84, 0x5a, 0x16, 0x50, 0x80, 0xf4, 0xa, 0x9f } };
// {74F6E873-A006-453B-91BE-D94262708142}
static const GUID guid_cfg_spectrumFrequencyMin = { 0x74f6e873, 0xa006, 0x453b,{ 0x91, 0xbe, 0xd9, 0x42, 0x62, 0x70, 0x81, 0x42 } };
// {5DD613E0-C796-4075-81EB-343F33B95217}
static const GUID guid_cfg_spectrumFrequencyMax = { 0x5dd613e0, 0xc796, 0x4075,{ 0x81, 0xeb, 0x34, 0x3f, 0x33, 0xb9, 0x52, 0x17 } };
// {82924D91-2EBC-48C0-AE88-6E2EC8BB46A4}
static const GUID guid_cfg_spectrogramFrequencyMin = { 0x82924d91, 0x2ebc, 0x48c0,{ 0xae, 0x88, 0x6e, 0x2e, 0xc8, 0xbb, 0x46, 0xa4 } };
// {3C273D16-C12B-4FA6-9C8F-D3BD04AAFDF5}
static const GUID guid_cfg_spectrogramFrequencyMax = { 0x3c273d16, 0xc12b, 0x4fa6,{ 0x9c, 0x8f, 0xd3, 0xbd, 0x4, 0xaa, 0xfd, 0xf5 } };
// {D0E61672-47F0-4C3F-AD57-38B41A7F0F73}
static const GUID guid_cfg_ledStripeSof = { 0xd0e61672, 0x47f0, 0x4c3f, { 0xad, 0x57, 0x38, 0xb4, 0x1a, 0x7f, 0xf, 0x73 } };
// {46D11564-6E28-459A-89D7-B410A80796F1}
static const GUID guid_cfg_currentLimit = { 0x46d11564, 0x6e28, 0x459a, { 0x89, 0xd7, 0xb4, 0x10, 0xa8, 0x7, 0x96, 0xf1 } };


// This GUID identifies our Advanced Preferences branch (replace with your own when reusing code).
// {6FABE4A9-2C7F-4FA9-9598-7480C8324250}
static const GUID guid_advconfig_branch = { 0x6fabe4a9, 0x2c7f, 0x4fa9,{ 0x95, 0x98, 0x74, 0x80, 0xc8, 0x32, 0x42, 0x50 } };


enum {
	default_cfg_matrixRows = ws2812::rows_def,
	default_cfg_matrixCols = ws2812::columns_def,
	default_cfg_brightness = ws2812::brightness_def,
	default_cfg_updateInterval = ws2812::timerInterval_def,
	default_cfg_comPort = ws2812::port_def,
	default_cfg_comBaudrate = static_cast<unsigned int>(ws2812_baudrate::e115200),
	default_cfg_startLed = static_cast<unsigned int>(ws2812_start_led::top_left),
	default_cfg_ledDirection = static_cast<unsigned int>(ws2812_led_direction::common),
	default_cfg_ledColors = static_cast<unsigned int>(ws2812_led_colors::eGRB),
	default_cfg_currentLimit = 0,

	default_cfg_lineStyle = static_cast<unsigned int>(ws2812_style::spectrum_simple),
	default_cfg_logFrequency = 1,
	default_cfg_logAmplitude = 1,
	default_cfg_peakValues = 1,

	default_cfg_spectrum_ampl_min = -50,
	default_cfg_spectrum_ampl_max = -15,

	default_cfg_spectrogram_ampl_min = -40,
	default_cfg_spectrogram_ampl_max = -5,

	default_cfg_oscilloscope_offset = 0,
	default_cfg_oscilloscope_amplitude = 100,

	default_cfg_spectrum_freq_min = 0,
	default_cfg_spectrum_freq_max = 22050,

	default_cfg_spectrogram_freq_min = 0,
	default_cfg_spectrogram_freq_max = 22050,
};

static const char * default_cfg_spectrumColors = "#FFFFFF";
static const char * default_cfg_spectrumBarColors = "#00FF00,#C8C800,#FF0000";
static const char * default_cfg_spectrumFireColors = "#00FF00,#C8C800,#FF0000";
static const char * default_cfg_spectrogramColors = "#000000,#00007F,#00C800,#C8C800,#FF0000";
static const char * default_cfg_oscilloscopeColors = "#000000,#FFFFFF";
static const char * default_cfg_ledStripeSof = "1";

// TODO language dependend strings, number of entries should depend on enum start_led
LPCTSTR		cfg_startLedStr[static_cast<unsigned int>(ws2812_start_led::eNo)] = { L"Top Left", L"Top Right", L"Bottom Left", L"Bottom Right" };
LRESULT		cfg_startLedId[static_cast<unsigned int>(ws2812_start_led::eNo)];

// TODO number of entries should depend on enum led_dir
LPCTSTR		cfg_ledDirStr[static_cast<unsigned int>(ws2812_led_direction::eNo)] = { L"Common", L"Alternating" };
LRESULT		cfg_ledDirId[static_cast<unsigned int>(ws2812_led_direction::eNo)];

// TODO number of entries should depend on enum led_colors
LPCTSTR		cfg_ledColorsStr[static_cast<unsigned int>(ws2812_led_colors::eNo)] = { L"GRB", L"BRG", L"RGB", L"GRBW", L"RGBW" };
LRESULT		cfg_ledColorsId[static_cast<unsigned int>(ws2812_led_colors::eNo)];

// TODO number of entries should depend on enum line_style
LPCTSTR		cfg_lineStyleStr[static_cast<unsigned int>(ws2812_style::eNo)] = { L"Simple", L"Bars", L"Fire", L"Spectrogram (hori)", L"Spectrogram (vert)", L"Oscilloscpe (Yt)", L"Oscilloscpe (XY)", L"Oscillogram (hori)", L"Oscillogram (vert)", L"LED Test" };
LRESULT		cfg_lineStyleId[static_cast<unsigned int>(ws2812_style::eNo)];

// TODO number of entries should depend on enum ws2812_baudrate
LPCTSTR		cfg_baudrateStr[static_cast<unsigned int>(ws2812_baudrate::eNo)] = { L"9600", L"14400", L"19200", L"38400", L"56000", L"57600", L"115200", L"128000", L"250000", L"256000", L"500000" };
LRESULT		cfg_baudrateId[static_cast<unsigned int>(ws2812_baudrate::eNo)];

static cfg_uint cfg_matrixRows(guid_cfg_matrixRows, default_cfg_matrixRows);
static cfg_uint cfg_matrixCols(guid_cfg_matrixCols, default_cfg_matrixCols);
static cfg_uint cfg_brightness(guid_cfg_brightness, default_cfg_brightness);
static cfg_uint cfg_updateInterval(guid_cfg_updateInterval, default_cfg_updateInterval);
static cfg_uint cfg_comPort(guid_cfg_comPort, default_cfg_comPort);
static cfg_uint cfg_comBaudrate(guid_cfg_comBaudrate, default_cfg_comBaudrate);
static cfg_uint cfg_startLed(guid_cfg_startLed, default_cfg_startLed);
static cfg_uint cfg_ledDirection(guid_cfg_ledDirection, default_cfg_ledDirection);
static cfg_uint cfg_ledColors(guid_cfg_ledColors, default_cfg_ledColors);

static cfg_uint cfg_lineStyle(guid_cfg_lineStyle, default_cfg_lineStyle);
static cfg_uint cfg_logFrequency(guid_cfg_logFrequency, default_cfg_logFrequency);
static cfg_uint cfg_logAmplitude(guid_cfg_logAmplitude, default_cfg_logAmplitude);
static cfg_uint cfg_peakValues(guid_cfg_peakValues, default_cfg_peakValues);

static cfg_int cfg_spectrumAmplMin(guid_cfg_spectrumAmplitudeMin, default_cfg_spectrum_ampl_min);
static cfg_int cfg_spectrumAmplMax(guid_cfg_spectrumAmplitudeMax, default_cfg_spectrum_ampl_max);

static cfg_int cfg_spectrogramAmplMin(guid_cfg_spectrogramAmplitudeMin, default_cfg_spectrogram_ampl_min);
static cfg_int cfg_spectrogramAmplMax(guid_cfg_spectrogramAmplitudeMax, default_cfg_spectrogram_ampl_max);

static cfg_int cfg_oscilloscopeOffset(guid_cfg_oscilloscopeOffset, default_cfg_oscilloscope_offset);
static cfg_int cfg_oscilloscopeAmplitude(guid_cfg_oscilloscopeAmplitude, default_cfg_oscilloscope_amplitude);

static cfg_int cfg_spectrumFreqMin(guid_cfg_spectrumFrequencyMin, default_cfg_spectrum_freq_min);
static cfg_int cfg_spectrumFreqMax(guid_cfg_spectrumFrequencyMax, default_cfg_spectrum_freq_max);

static cfg_int cfg_spectrogramFreqMin(guid_cfg_spectrogramFrequencyMin, default_cfg_spectrogram_freq_min);
static cfg_int cfg_spectrogramFreqMax(guid_cfg_spectrogramFrequencyMax, default_cfg_spectrogram_freq_max);

static cfg_string cfg_spectrumColors(guid_cfg_spectrumColors, default_cfg_spectrumColors);
static cfg_string cfg_spectrumBarColors(guid_cfg_spectrumBarColors, default_cfg_spectrumBarColors);
static cfg_string cfg_spectrumFireColors(guid_cfg_spectrumFireColors, default_cfg_spectrumFireColors);
static cfg_string cfg_spectrogramColors(guid_cfg_spectrogramColors, default_cfg_spectrogramColors);
static cfg_string cfg_oscilloscopeColors(guid_cfg_oscilloscopeColors, default_cfg_oscilloscopeColors);

static cfg_string cfg_ledStripeSof(guid_cfg_ledStripeSof, default_cfg_ledStripeSof);

static cfg_uint cfg_currentLimit(guid_cfg_currentLimit, default_cfg_currentLimit);

static advconfig_branch_factory g_advconfigBranch("WS2812 Output", guid_advconfig_branch, advconfig_branch::guid_branch_tools, 0);

class CWS2812Preferences : public CDialogImpl<CWS2812Preferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CWS2812Preferences(preferences_page_callback::ptr callback) : m_callback(callback)
	{}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.


	//dialog resource ID
	enum {IDD = IDD_MYPREFERENCES};
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(CWS2812Preferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_MATRIX_ROWS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_MATRIX_COLS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_BRIGHTNESS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_UPDATE_INTERVAL, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_COM_PORT, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_CURRENT_LIMIT, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_START_LED, CBN_SELCHANGE, OnCBChange)
		COMMAND_HANDLER_EX(IDC_LED_DIR, CBN_SELCHANGE, OnCBChange)
		COMMAND_HANDLER_EX(IDC_LED_COLORS, CBN_SELCHANGE, OnCBChange)
		COMMAND_HANDLER_EX(IDC_LINE_STYLE, CBN_SELCHANGE, OnCBChange)
		COMMAND_HANDLER_EX(IDC_COM_BAUDRATE, CBN_SELCHANGE, OnCBChange)
		COMMAND_HANDLER_EX(IDC_LOG_FREQ, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_LOG_AMPL, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_PEAK_VALS, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_TXT_SPECTRUM_COLORS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_TXT_SPECTRUM_BAR_COLORS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_TXT_SPECTRUM_FIRE_COLORS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_TXT_SPECTROGRAM_COLORS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_TXT_OSCILLOSCOPE_COLORS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_LED_STRIPE_SOF, EN_CHANGE, OnEditChange)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	void OnCBChange(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();
	void UpdateMatrixInfo();

	const preferences_page_callback::ptr m_callback;

	bool cbChanged;

};

BOOL CWS2812Preferences::OnInitDialog(CWindow, LPARAM) {
	// integers
	SetDlgItemInt(IDC_MATRIX_ROWS, cfg_matrixRows, FALSE);
	SetDlgItemInt(IDC_MATRIX_COLS, cfg_matrixCols, FALSE);
	SetDlgItemInt(IDC_BRIGHTNESS, cfg_brightness, FALSE);
	SetDlgItemInt(IDC_UPDATE_INTERVAL, cfg_updateInterval, FALSE);
	SetDlgItemInt(IDC_COM_PORT, cfg_comPort, FALSE);
	SetDlgItemInt(IDC_CURRENT_LIMIT, cfg_currentLimit, FALSE);

	// checkboxes
	CheckDlgButton(IDC_LOG_FREQ, cfg_logFrequency);
	CheckDlgButton(IDC_LOG_AMPL, cfg_logAmplitude);
	CheckDlgButton(IDC_PEAK_VALS, cfg_peakValues);

	// Comboboxes
	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_startLedId); n++) {
		WCHAR	str[64];

		if (cfg_startLedStr[n])
			StrCpy(str, cfg_startLedStr[n]);
		else
			StrCpy(str, L"?");
		cfg_startLedId[n] = SendDlgItemMessage(IDC_START_LED, CB_ADDSTRING, 0, (DWORD)str);
	}
	SendDlgItemMessage(IDC_START_LED, CB_SETCURSEL, cfg_startLedId[cfg_startLed], 0);

	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_ledDirId); n++) {
		WCHAR	str[64];

		if (cfg_ledDirStr[n])
			StrCpy(str, cfg_ledDirStr[n]);
		else
			StrCpy(str, L"?");
		cfg_ledDirId[n] = SendDlgItemMessage(IDC_LED_DIR, CB_ADDSTRING, 0, (DWORD)str);
	}
	SendDlgItemMessage(IDC_LED_DIR, CB_SETCURSEL, cfg_ledDirId[cfg_ledDirection], 0);

	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_ledColorsId); n++) {
		WCHAR	str[64];

		if (cfg_ledColorsStr[n])
			StrCpy(str, cfg_ledColorsStr[n]);
		else
			StrCpy(str, L"?");
		cfg_ledColorsId[n] = SendDlgItemMessage(IDC_LED_COLORS, CB_ADDSTRING, 0, (DWORD)str);
	}
	SendDlgItemMessage(IDC_LED_COLORS, CB_SETCURSEL, cfg_ledColorsId[cfg_ledColors], 0);

	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_lineStyleId); n++) {
		WCHAR	str[64];

		if (cfg_lineStyleStr[n])
			StrCpy(str, cfg_lineStyleStr[n]);
		else
			StrCpy(str, L"?");
		cfg_lineStyleId[n] = SendDlgItemMessage(IDC_LINE_STYLE, CB_ADDSTRING, 0, (DWORD)str);
	}
	SendDlgItemMessage(IDC_LINE_STYLE, CB_SETCURSEL, cfg_lineStyleId[cfg_lineStyle], 0);

	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_baudrateId); n++) {
		WCHAR	str[64];

		if (cfg_baudrateStr[n])
			StrCpy(str, cfg_baudrateStr[n]);
		else
			StrCpy(str, L"?");
		cfg_baudrateId[n] = SendDlgItemMessage(IDC_COM_BAUDRATE, CB_ADDSTRING, 0, (DWORD)str);
	}
	SendDlgItemMessage(IDC_COM_BAUDRATE, CB_SETCURSEL, cfg_baudrateId[cfg_comBaudrate], 0);


	// Colors
	pfc::string8 pattern;

	pattern = cfg_spectrumColors;
	uSetDlgItemText(*this, IDC_TXT_SPECTRUM_COLORS, pattern);

	pattern = cfg_spectrumBarColors;
	uSetDlgItemText(*this, IDC_TXT_SPECTRUM_BAR_COLORS, pattern);

	pattern = cfg_spectrumFireColors;
	uSetDlgItemText(*this, IDC_TXT_SPECTRUM_FIRE_COLORS, pattern);

	pattern = cfg_spectrogramColors;
	uSetDlgItemText(*this, IDC_TXT_SPECTROGRAM_COLORS, pattern);

	pattern = cfg_oscilloscopeColors;
	uSetDlgItemText(*this, IDC_TXT_OSCILLOSCOPE_COLORS, pattern);

	pattern = cfg_ledStripeSof;
	uSetDlgItemText(*this, IDC_LED_STRIPE_SOF, pattern);

	// update matrix info
	UpdateMatrixInfo();

	cbChanged = false;

	return FALSE;
}

void CWS2812Preferences::OnEditChange(UINT, int, CWindow) {
	// not much to do here
	OnChanged();
}

void CWS2812Preferences::OnCBChange(UINT, int, CWindow) {
	cbChanged = true;
	OnChanged();
}

t_uint32 CWS2812Preferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CWS2812Preferences::reset() {
	SetDlgItemInt(IDC_MATRIX_ROWS, default_cfg_matrixRows, FALSE);
	SetDlgItemInt(IDC_MATRIX_COLS, default_cfg_matrixCols, FALSE);
	SetDlgItemInt(IDC_BRIGHTNESS, default_cfg_brightness, FALSE);
	SetDlgItemInt(IDC_UPDATE_INTERVAL, default_cfg_updateInterval, FALSE);
	SetDlgItemInt(IDC_COM_PORT, default_cfg_comPort, FALSE);
	SetDlgItemInt(IDC_CURRENT_LIMIT, default_cfg_currentLimit, FALSE);

	CheckDlgButton(IDC_LOG_FREQ, default_cfg_logFrequency);
	CheckDlgButton(IDC_LOG_AMPL, default_cfg_logAmplitude);
	CheckDlgButton(IDC_PEAK_VALS, default_cfg_peakValues);

	SendDlgItemMessage(IDC_START_LED, CB_SETCURSEL, cfg_startLedId[default_cfg_startLed], 0);
	SendDlgItemMessage(IDC_LED_DIR, CB_SETCURSEL, cfg_ledDirId[default_cfg_ledDirection], 0);
	SendDlgItemMessage(IDC_LED_COLORS, CB_SETCURSEL, cfg_ledColorsId[default_cfg_ledColors], 0);
	SendDlgItemMessage(IDC_LINE_STYLE, CB_SETCURSEL, cfg_lineStyleId[default_cfg_lineStyle], 0);
	SendDlgItemMessage(IDC_COM_BAUDRATE, CB_SETCURSEL, cfg_baudrateId[default_cfg_comBaudrate], 0);

	uSetDlgItemText(*this, IDC_TXT_SPECTRUM_COLORS, default_cfg_spectrumColors);
	uSetDlgItemText(*this, IDC_TXT_SPECTRUM_BAR_COLORS, default_cfg_spectrumBarColors);
	uSetDlgItemText(*this, IDC_TXT_SPECTRUM_FIRE_COLORS, default_cfg_spectrumFireColors);
	uSetDlgItemText(*this, IDC_TXT_SPECTROGRAM_COLORS, default_cfg_spectrogramColors);
	uSetDlgItemText(*this, IDC_TXT_OSCILLOSCOPE_COLORS, default_cfg_oscilloscopeColors);

	uSetDlgItemText(*this, IDC_LED_STRIPE_SOF, default_cfg_ledStripeSof);

	// update matrix info
	UpdateMatrixInfo();

	cbChanged = true;

	OnChanged();
}

void CWS2812Preferences::apply() {
	bool	isActive;
	bool	changed = false;
	UINT	val;
	LRESULT	r;

	// Min/Max ???
	val = GetDlgItemInt(IDC_MATRIX_ROWS, NULL, FALSE);
	val = GetRowLimited(val);
	SetDlgItemInt(IDC_MATRIX_ROWS, val, FALSE);

	changed |= (cfg_matrixRows != val);
	cfg_matrixRows = val;

	val = GetDlgItemInt(IDC_MATRIX_COLS, NULL, FALSE);
	val = GetColumnsLimited(val);
	SetDlgItemInt(IDC_MATRIX_COLS, val, FALSE);

	changed |= (cfg_matrixCols != val);
	cfg_matrixCols = val;

	val = GetDlgItemInt(IDC_BRIGHTNESS, NULL, FALSE);
	val = GetBrightnessLimited(val);
	SetDlgItemInt(IDC_BRIGHTNESS, val, FALSE);

	changed |= (cfg_brightness != val);
	cfg_brightness = val;

	val = GetDlgItemInt(IDC_UPDATE_INTERVAL, NULL, FALSE);
	val = GetIntervalLimited(val);
	SetDlgItemInt(IDC_UPDATE_INTERVAL, val, FALSE);

	changed |= (cfg_updateInterval != val);
	cfg_updateInterval = val;

	val = GetDlgItemInt(IDC_COM_PORT, NULL, FALSE);
	changed |= (cfg_comPort != val);
	cfg_comPort = val;

	val = GetDlgItemInt(IDC_CURRENT_LIMIT, NULL, FALSE);
	changed |= (cfg_currentLimit != val);
	cfg_currentLimit = val;

	val = IsDlgButtonChecked(IDC_LOG_FREQ);
	changed |= (cfg_logFrequency != val);
	cfg_logFrequency = val;

	val = IsDlgButtonChecked(IDC_LOG_AMPL);
	changed |= (cfg_logAmplitude != val);
	cfg_logAmplitude = val;

	val = IsDlgButtonChecked(IDC_PEAK_VALS);
	changed |= (cfg_peakValues != val);
	cfg_peakValues = val;

	r = SendDlgItemMessage(IDC_START_LED, CB_GETCURSEL, 0, 0);
	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_startLedId); n++) {
		if (r == cfg_startLedId[n]) {
			changed |= (cfg_startLed != n);
			cfg_startLed = n;
			break;
		}
	}
	r = SendDlgItemMessage(IDC_LED_DIR, CB_GETCURSEL, 0, 0);
	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_ledDirId); n++) {
		if (r == cfg_ledDirId[n]) {
			changed |= (cfg_ledDirection != n);
			cfg_ledDirection = n;
			break;
		}
	}
	r = SendDlgItemMessage(IDC_LED_COLORS, CB_GETCURSEL, 0, 0);
	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_ledColorsId); n++) {
		if (r == cfg_ledColorsId[n]) {
			changed |= (cfg_ledColors != n);
			cfg_ledColors = n;
			break;
		}
	}

	r = SendDlgItemMessage(IDC_LINE_STYLE, CB_GETCURSEL, 0, 0);
//	r = ComboBox_GetCurSel(IDC_LINE_STYLE);
	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_lineStyleId); n++) {
		if (r == cfg_lineStyleId[n]) {
			changed |= (cfg_lineStyle != n);
			cfg_lineStyle = n;
			break;
		}
	}

	r = SendDlgItemMessage(IDC_COM_BAUDRATE, CB_GETCURSEL, 0, 0);
	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_baudrateId); n++) {
		if (r == cfg_baudrateId[n]) {
			changed |= (cfg_comBaudrate != n);
			cfg_comBaudrate = n;
			break;
		}
	}

	// strings
	pfc::string8 pattern;

	uGetDlgItemText(*this, IDC_TXT_SPECTRUM_COLORS, pattern);
	cfg_spectrumColors = pattern;

	uGetDlgItemText(*this, IDC_TXT_SPECTRUM_BAR_COLORS, pattern);
	cfg_spectrumBarColors = pattern;

	uGetDlgItemText(*this, IDC_TXT_SPECTRUM_FIRE_COLORS, pattern);
	cfg_spectrumFireColors = pattern;

	uGetDlgItemText(*this, IDC_TXT_SPECTROGRAM_COLORS, pattern);
	cfg_spectrogramColors = pattern;

	uGetDlgItemText(*this, IDC_TXT_OSCILLOSCOPE_COLORS, pattern);
	cfg_oscilloscopeColors = pattern;

	uGetDlgItemText(*this, IDC_LED_STRIPE_SOF, pattern);
	cfg_ledStripeSof = pattern;

	cbChanged = false;


	// stop output
	isActive = StopOutput();

	// write configuration values into the driver
	ConfigMatrix(cfg_matrixRows, cfg_matrixCols, cfg_startLed, cfg_ledDirection, cfg_ledColors);
	SetLedStripeSof(cfg_ledStripeSof);
	SetBrightness(cfg_brightness);
	SetCurrentLimit(cfg_currentLimit);
	SetInterval(cfg_updateInterval);
	SetComPort(cfg_comPort);
	SetComBaudrate(cfg_comBaudrate);

	SetStyle(cfg_lineStyle);
	SetScaling(cfg_logFrequency, cfg_logAmplitude, cfg_peakValues);

	const char *colors = nullptr;
	unsigned int style = cfg_lineStyle;
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

	// update the matrix info text
	UpdateMatrixInfo();

	// restart the output
	if (isActive)
		StartOutput();

	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CWS2812Preferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	bool changed = false;

	changed |= GetDlgItemInt(IDC_MATRIX_ROWS, NULL, FALSE) != cfg_matrixRows;
	changed |= GetDlgItemInt(IDC_MATRIX_COLS, NULL, FALSE) != cfg_matrixCols;
	changed |= GetDlgItemInt(IDC_BRIGHTNESS, NULL, FALSE) != cfg_brightness;
	changed |= GetDlgItemInt(IDC_UPDATE_INTERVAL, NULL, FALSE) != cfg_updateInterval;
	changed |= GetDlgItemInt(IDC_COM_PORT, NULL, FALSE) != cfg_comPort;
//	changed |= GetDlgItemInt(IDC_START_LED, NULL, FALSE) != cfg_startLed;
//	changed |= GetDlgItemInt(IDC_LED_DIR, NULL, FALSE) != cfg_ledDirection;
	changed |= GetDlgItemInt(IDC_CURRENT_LIMIT, NULL, FALSE) != cfg_currentLimit;

//	changed |= GetDlgItemInt(IDC_LINE_STYLE, NULL, FALSE) != cfg_lineStyle;
	changed |= IsDlgButtonChecked(IDC_LOG_FREQ) != cfg_logFrequency;
	changed |= IsDlgButtonChecked(IDC_LOG_AMPL) != cfg_logAmplitude;
	changed |= IsDlgButtonChecked(IDC_PEAK_VALS) != cfg_peakValues;

	pfc::string8 pattern;

	uGetDlgItemText(*this, IDC_TXT_SPECTRUM_COLORS, pattern);
	changed |= (uStringCompare(cfg_spectrumColors, pattern) != 0);

	uGetDlgItemText(*this, IDC_TXT_SPECTRUM_BAR_COLORS, pattern);
	changed |= (uStringCompare(cfg_spectrumBarColors, pattern) != 0);

	uGetDlgItemText(*this, IDC_TXT_SPECTRUM_FIRE_COLORS, pattern);
	changed |= (uStringCompare(cfg_spectrumFireColors, pattern) != 0);

	uGetDlgItemText(*this, IDC_TXT_SPECTROGRAM_COLORS, pattern);
	changed |= (uStringCompare(cfg_spectrogramColors, pattern) != 0);

	uGetDlgItemText(*this, IDC_TXT_OSCILLOSCOPE_COLORS, pattern);
	changed |= (uStringCompare(cfg_oscilloscopeColors, pattern) != 0);

	uGetDlgItemText(*this, IDC_LED_STRIPE_SOF, pattern);
	changed |= (uStringCompare(cfg_ledStripeSof, pattern) != 0);


	changed |= cbChanged;

	return changed;
}

void CWS2812Preferences::OnChanged() {
	// update the matrix info text
	UpdateMatrixInfo();
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

void CWS2812Preferences::UpdateMatrixInfo(void)
{
	UINT	rows, cols, ledNo, brightness;
	double	time, current;
	WCHAR	text[100];

	// get matrix configuration
	rows = GetDlgItemInt(IDC_MATRIX_ROWS, NULL, FALSE);
	cols = GetDlgItemInt(IDC_MATRIX_COLS, NULL, FALSE);
	brightness = GetDlgItemInt(IDC_BRIGHTNESS, NULL, FALSE);

	// count of LEDs
	ledNo = rows * cols;

	// led transfer time: reset plus 1.25us per led
	time = 50 + ceil((double)ledNo * 1.25);

	// maximum power consumption (max 60mA per LED)
	double ledCurrent = 3 * 0.020;

	if (cfg_ledColors >= static_cast<unsigned int>(ws2812_led_colors::eGRBW))
		ledCurrent = 4 * 0.020;

	current = ledCurrent * (double)ledNo * ((double)ws2812::brightness_max / 100);

	_stprintf_s(text, L"%u LEDs: %.2f A; %.0f us", ledNo, current, time);
	SetDlgItemText(IDC_MATRIX_INFO, text);
}

class preferences_page_ws2812 : public preferences_page_impl<CWS2812Preferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() {return "WS2812 Output";}
	GUID get_guid() {
		// This is our GUID. Replace with your own when reusing the code.
		// {4B176673-4152-497C-BFEE-BA2C5173C7A7}
		static const GUID guid = { 0x4b176673, 0x4152, 0x497c,{ 0xbf, 0xee, 0xba, 0x2c, 0x51, 0x73, 0xc7, 0xa7 } };

		return guid;
	}
	GUID get_parent_guid() {return guid_tools;}
};

static preferences_page_factory_t<preferences_page_ws2812> g_preferences_page_ws2812_factory;


unsigned int GetCfgComPort()
{
	return cfg_comPort;
}

unsigned int GetCfgComBaudrate()
{
	return cfg_comBaudrate;
}

unsigned int GetCfgMatrixRows()
{
	return cfg_matrixRows;
}

unsigned int GetCfgMatrixCols()
{
	return cfg_matrixCols;
}

unsigned int GetCfgBrightness()
{
	return cfg_brightness;
}

unsigned int GetCfgCurrentLimit()
{
	return cfg_currentLimit;
}

unsigned int GetCfgUpdateInterval()
{
	return cfg_updateInterval;
}

unsigned int GetCfgStartLed()
{
	return cfg_startLed;
}

unsigned int GetCfgLedDirection()
{
	return cfg_ledDirection;
}

unsigned int GetCfgLedColors()
{
	return cfg_ledColors;
}

unsigned int GetCfgLineStyle()
{
	return cfg_lineStyle;
}

unsigned int GetCfgLogFrequency()
{
	return cfg_logFrequency;
}

unsigned int GetCfgLogAmplitude()
{
	return cfg_logAmplitude;
}

unsigned int GetCfgPeakValues()
{
	return cfg_peakValues;
}

const char * GetCfgSpectrumColors()
{
	return cfg_spectrumColors;
}

const char * GetCfgSpectrumBarColors()
{
	return cfg_spectrumBarColors;
}

const char * GetCfgSpectrumFireColors()
{
	return cfg_spectrumFireColors;
}

const char * GetCfgSpectrogramColors()
{
	return cfg_spectrogramColors;
}

const char * GetCfgOscilloscopeColors()
{
	return cfg_oscilloscopeColors;
}

const char * GetCfgLedStripeSof()
{
	return cfg_ledStripeSof;
}

void GetCfgSpectrumAmplitudeMinMax(int &min, int &max)
{
	min = cfg_spectrumAmplMin;
	max = cfg_spectrumAmplMax;
}

void GetCfgSpectrogramAmplitudeMinMax(int &min, int &max)
{
	min = cfg_spectrogramAmplMin;
	max = cfg_spectrogramAmplMax;
}

void GetCfgOscilloscopeOffsetAmplitude(int &offset, int &amplitude)
{
	offset = cfg_oscilloscopeOffset;
	amplitude = cfg_oscilloscopeAmplitude;
}

void GetCfgSpectrumFrequencyMinMax(int &min, int &max)
{
	min = cfg_spectrumFreqMin;
	max = cfg_spectrumFreqMax;
}

void GetCfgSpectrogramFrequencyMinMax(int &min, int &max)
{
	min = cfg_spectrogramFreqMin;
	max = cfg_spectrogramFreqMax;
}



bool SetCfgComPort(unsigned int value)
{
	if (cfg_comPort != value) {
		cfg_comPort = value;
		return true;
	}
	else {
		return false;
	}
}

bool SetCfgComBaudrate(unsigned int value)
{
	if (cfg_comBaudrate != value) {
		cfg_comBaudrate = value;
		return true;
	}
	else {
		return false;
	}
}

bool SetCfgMatrixRows(unsigned int value)
{
	if (cfg_matrixRows != value) {
		cfg_matrixRows = value;
		return true;
	}
	else {
		return false;
	}
}

bool SetCfgMatrixCols(unsigned int value)
{
	if (cfg_matrixCols != value) {
		cfg_matrixCols = value;
		return true;
	}
	else {
		return false;
	}
}

bool SetCfgBrightness(unsigned int value)
{
	if (cfg_brightness != value) {
		cfg_brightness = value;
		return true;
	}
	else {
		return false;
	}
}

bool SetCfgCurrentLimit(unsigned int value)
{
	if (cfg_currentLimit != value) {
		cfg_currentLimit = value;
		return true;
	}
	else {
		return false;
	}
}

bool SetCfgUpdateInterval(unsigned int value)
{
	if (cfg_updateInterval != value) {
		cfg_updateInterval = value;
		return true;
	}
	else {
		return false;
	}
}

bool SetCfgStartLed(unsigned int value)
{
	if (cfg_startLed != value) {
		cfg_startLed = value;
		return true;
	}
	else {
		return false;
	}
}

bool SetCfgLedDirection(unsigned int value)
{
	if (cfg_ledDirection != value) {
		cfg_ledDirection = value;
		return true;
	}
	else {
		return false;
	}
}

bool SetCfgLedColors(unsigned int value)
{
	if (cfg_ledColors != value) {
		cfg_ledColors = value;
		return true;
	}
	else {
		return false;
	}
}

bool SetCfgLineStyle(unsigned int value)
{
	if (cfg_lineStyle != value) {
		cfg_lineStyle = value;
		return true;
	}
	else {
		return false;
	}
}

bool SetCfgLogFrequency(unsigned int value)
{
	if (cfg_logFrequency != value) {
		cfg_logFrequency = value;
		return true;
	}
	else {
		return false;
	}
}

bool SetCfgLogAmplitude(unsigned int value)
{
	if (cfg_logAmplitude != value) {
		cfg_logAmplitude = value;
		return true;
	}
	else {
		return false;
	}
}

bool SetCfgPeakValues(unsigned int value)
{
	if (cfg_peakValues != value) {
		cfg_peakValues = value;
		return true;
	}
	else {
		return false;
	}
}

bool SetCfgSpectrumAmplitudeMinMax(int min, int max)
{
	bool changed = false;

	changed |= (cfg_spectrumAmplMin != min);
	changed |= (cfg_spectrumAmplMax != max);

	if (changed) {
		cfg_spectrumAmplMin = min;
		cfg_spectrumAmplMax = max;
	}
	return changed;
}

bool SetCfgSpectrogramAmplitudeMinMax(int min, int max)
{
	bool changed = false;

	changed |= (cfg_spectrogramAmplMin != min);
	changed |= (cfg_spectrogramAmplMax != max);

	if (changed) {
		cfg_spectrogramAmplMin = min;
		cfg_spectrogramAmplMax = max;
	}
	return changed;
}

bool SetCfgOscilloscopeOffsetAmplitude(int offset, int amplitude)
{
	bool changed = false;

	changed |= (cfg_oscilloscopeOffset != offset);
	changed |= (cfg_oscilloscopeAmplitude != amplitude);

	if (changed) {
		cfg_oscilloscopeOffset = offset;
		cfg_oscilloscopeAmplitude = amplitude;
	}
	return changed;
}

bool SetCfgSpectrumFrequencyMinMax(int min, int max)
{
	bool changed = false;

	changed |= (cfg_spectrumFreqMin != min);
	changed |= (cfg_spectrumFreqMax != max);

	if (changed) {
		cfg_spectrumFreqMin = min;
		cfg_spectrumFreqMax = max;
	}
	return changed;
}

bool SetCfgSpectrogramFrequencyMinMax(int min, int max)
{
	bool changed = false;

	changed |= (cfg_spectrogramFreqMin != min);
	changed |= (cfg_spectrogramFreqMax != max);

	if (changed) {
		cfg_spectrogramFreqMin = min;
		cfg_spectrogramFreqMax = max;
	}
	return changed;
}