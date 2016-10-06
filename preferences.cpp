#include "stdafx.h"
#include "resource.h"

// Sample preferences interface: two meaningless configuration settings accessible through a preferences page and one accessible through advanced preferences.

#include "foo_ws2812.h"

#define CALC_TAB_ELEMENTS(_TAB_)	(sizeof(_TAB_)/sizeof(_TAB_[0]))

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
// {556A3C73-B652-4FAB-BCB2-1340AAE742F6}
static const GUID guid_cfg_startLed = { 0x556a3c73, 0xb652, 0x4fab,{ 0xbc, 0xb2, 0x13, 0x40, 0xaa, 0xe7, 0x42, 0xf6 } };
// {D4F9F718-E38B-4847-93F0-C6BC6008987C}
static const GUID guid_cfg_ledDirection = { 0xd4f9f718, 0xe38b, 0x4847,{ 0x93, 0xf0, 0xc6, 0xbc, 0x60, 0x8, 0x98, 0x7c } };
// {3B6337F1-538D-480A-84A3-5527FCE35A83}
static const GUID guid_cfg_lineStyle = { 0x3b6337f1, 0x538d, 0x480a,{ 0x84, 0xa3, 0x55, 0x27, 0xfc, 0xe3, 0x5a, 0x83 } };
// {5F9CF2D6-331D-4EE1-9845-BB3E72CD08D3}
static const GUID guid_cfg_logFrequency = { 0x5f9cf2d6, 0x331d, 0x4ee1,{ 0x98, 0x45, 0xbb, 0x3e, 0x72, 0xcd, 0x8, 0xd3 } };
// {62F8A4AB-3A4F-41FA-B4C1-112BCE13FEFF}
static const GUID guid_cfg_logAmplitude = { 0x62f8a4ab, 0x3a4f, 0x41fa,{ 0xb4, 0xc1, 0x11, 0x2b, 0xce, 0x13, 0xfe, 0xff } };
// {B3E9A0FF-00CA-4948-A35E-F56D8B8D8CF2}
static const GUID guid_cfg_peakValues = { 0xb3e9a0ff, 0xca, 0x4948,{ 0xa3, 0x5e, 0xf5, 0x6d, 0x8b, 0x8d, 0x8c, 0xf2 } };


// This GUID identifies our Advanced Preferences branch (replace with your own when reusing code).
// {6FABE4A9-2C7F-4FA9-9598-7480C8324250}
static const GUID guid_advconfig_branch = { 0x6fabe4a9, 0x2c7f, 0x4fa9,{ 0x95, 0x98, 0x74, 0x80, 0xc8, 0x32, 0x42, 0x50 } };
// This GUID identifies our Advanced Preferences setting (replace with your own when reusing code) as well as this setting's storage within our component's configuration file.
// {B462CBE2-7561-4AC9-B8DE-9297EE1FD7A0}
static const GUID guid_cfg_bogoSetting3 = { 0xb462cbe2, 0x7561, 0x4ac9,{ 0xb8, 0xde, 0x92, 0x97, 0xee, 0x1f, 0xd7, 0xa0 } };


enum {
	default_cfg_matrixRows = 8,
	default_cfg_matrixCols = 8,
	default_cfg_brightness = 25,
	default_cfg_updateInterval = 250,
	default_cfg_comPort = 5,
	default_cfg_startLed = 0,
	default_cfg_ledDirection = 0,

	default_cfg_lineStyle = 0,
	default_cfg_logFrequency = 1,
	default_cfg_logAmplitude = 1,
	default_cfg_peakValues = 1,

	default_cfg_bogoSetting3 = 42,
};

// TODO language dependend strings, number of entries should depend on enum start_led
LPCTSTR		cfg_startLedStr[4] = { L"Top Left", L"Top Right", L"Bottom Left", L"Bottom Right" };
LRESULT		cfg_startLedId[4];

// TODO number of entries should depend on enum led_dir
LPCTSTR		cfg_ledDirStr[2] = { L"Common", L"Alternating" };
LRESULT		cfg_ledDirId[2];

// TODO number of entries should depend on enum line_style
LPCTSTR		cfg_lineStyleStr[5] = { L"Simple", L"Green/Red", L"Fire", L"Spectrogram", L"Oscilloscpe" };
LRESULT		cfg_lineStyleId[5];


static cfg_uint cfg_matrixRows(guid_cfg_matrixRows, default_cfg_matrixRows);
static cfg_uint cfg_matrixCols(guid_cfg_matrixCols, default_cfg_matrixCols);
static cfg_uint cfg_brightness(guid_cfg_brightness, default_cfg_brightness);
static cfg_uint cfg_updateInterval(guid_cfg_updateInterval, default_cfg_updateInterval);
static cfg_uint cfg_comPort(guid_cfg_comPort, default_cfg_comPort);
static cfg_uint cfg_startLed(guid_cfg_startLed, default_cfg_startLed);
static cfg_uint cfg_ledDirection(guid_cfg_ledDirection, default_cfg_ledDirection);

static cfg_uint cfg_lineStyle(guid_cfg_lineStyle, default_cfg_lineStyle);
static cfg_uint cfg_logFrequency(guid_cfg_logFrequency, default_cfg_logFrequency);
static cfg_uint cfg_logAmplitude(guid_cfg_logAmplitude, default_cfg_logAmplitude);
static cfg_uint cfg_peakValues(guid_cfg_peakValues, default_cfg_peakValues);

static advconfig_branch_factory g_advconfigBranch("WS2812 Output", guid_advconfig_branch, advconfig_branch::guid_branch_tools, 0);

static advconfig_integer_factory cfg_bogoSetting3("Bogo setting 3", guid_cfg_bogoSetting3, guid_advconfig_branch, 0, default_cfg_bogoSetting3, 0 /*minimum value*/, 9999 /*maximum value*/);

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback)
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
	BEGIN_MSG_MAP(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_MATRIX_ROWS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_MATRIX_COLS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_BRIGHTNESS, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_UPDATE_INTERVAL, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_COM_PORT, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_START_LED, CBN_SELCHANGE, OnCBChange)
		COMMAND_HANDLER_EX(IDC_LED_DIR, CBN_SELCHANGE, OnCBChange)
		COMMAND_HANDLER_EX(IDC_LINE_STYLE, CBN_SELCHANGE, OnCBChange)
		COMMAND_HANDLER_EX(IDC_LOG_FREQ, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_LOG_AMPL, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_PEAK_VALS, BN_CLICKED, OnEditChange)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	void OnCBChange(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;

	bool cbChanged;

};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	SetDlgItemInt(IDC_MATRIX_ROWS, cfg_matrixRows, FALSE);
	SetDlgItemInt(IDC_MATRIX_COLS, cfg_matrixCols, FALSE);
	SetDlgItemInt(IDC_BRIGHTNESS, cfg_brightness, FALSE);
	SetDlgItemInt(IDC_UPDATE_INTERVAL, cfg_updateInterval, FALSE);
	SetDlgItemInt(IDC_COM_PORT, cfg_comPort, FALSE);
//	SetDlgItemInt(IDC_START_LED, cfg_startLed, FALSE);
//	SetDlgItemInt(IDC_LED_DIR, cfg_ledDirection, FALSE);

//	SetDlgItemInt(IDC_LINE_STYLE, cfg_lineStyle, FALSE);
	CheckDlgButton(IDC_LOG_FREQ, cfg_logFrequency);
	CheckDlgButton(IDC_LOG_AMPL, cfg_logAmplitude);
	CheckDlgButton(IDC_PEAK_VALS, cfg_peakValues);

	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_startLedId); n++) {
		WCHAR	str[64];

		StrCpy(str, cfg_startLedStr[n]);
		cfg_startLedId[n] = SendDlgItemMessage(IDC_START_LED, CB_ADDSTRING, 0, (DWORD)str);
	}
	SendDlgItemMessage(IDC_START_LED, CB_SETCURSEL, cfg_startLedId[cfg_startLed], 0);

	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_ledDirId); n++) {
		WCHAR	str[64];

		StrCpy(str, cfg_ledDirStr[n]);
		cfg_ledDirId[n] = SendDlgItemMessage(IDC_LED_DIR, CB_ADDSTRING, 0, (DWORD)str);
	}
	SendDlgItemMessage(IDC_LED_DIR, CB_SETCURSEL, cfg_ledDirId[cfg_ledDirection], 0);

	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_lineStyleId); n++) {
		WCHAR	str[64];

		StrCpy(str, cfg_lineStyleStr[n]);
		cfg_lineStyleId[n] = SendDlgItemMessage(IDC_LINE_STYLE, CB_ADDSTRING, 0, (DWORD)str);
	}
	SendDlgItemMessage(IDC_LINE_STYLE, CB_SETCURSEL, cfg_lineStyleId[cfg_lineStyle], 0);

	cbChanged = false;

	return FALSE;
}

void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	// not much to do here
	OnChanged();
}

void CMyPreferences::OnCBChange(UINT, int, CWindow) {
	cbChanged = true;
	OnChanged();
}

t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CMyPreferences::reset() {
	SetDlgItemInt(IDC_MATRIX_ROWS, default_cfg_matrixRows, FALSE);
	SetDlgItemInt(IDC_MATRIX_COLS, default_cfg_matrixCols, FALSE);
	SetDlgItemInt(IDC_BRIGHTNESS, default_cfg_brightness, FALSE);
	SetDlgItemInt(IDC_UPDATE_INTERVAL, default_cfg_updateInterval, FALSE);
	SetDlgItemInt(IDC_COM_PORT, default_cfg_comPort, FALSE);
//	SetDlgItemInt(IDC_START_LED, default_cfg_startLed, FALSE);
//	SetDlgItemInt(IDC_LED_DIR, default_cfg_ledDirection, FALSE);

//	SetDlgItemInt(IDC_LINE_STYLE, default_cfg_lineStyle, FALSE);
	CheckDlgButton(IDC_LOG_FREQ, default_cfg_logFrequency);
	CheckDlgButton(IDC_LOG_AMPL, default_cfg_logAmplitude);
	CheckDlgButton(IDC_PEAK_VALS, default_cfg_peakValues);

	SendDlgItemMessage(IDC_START_LED, CB_SETCURSEL, cfg_startLedId[default_cfg_startLed], 0);
	SendDlgItemMessage(IDC_LED_DIR, CB_SETCURSEL, cfg_startLedId[default_cfg_ledDirection], 0);
	SendDlgItemMessage(IDC_LINE_STYLE, CB_SETCURSEL, cfg_startLedId[default_cfg_lineStyle], 0);
	cbChanged = true;

	OnChanged();
}

void CMyPreferences::apply() {
	bool	isActive;
	LRESULT	r;

	// Min/Max ???
	cfg_matrixRows = GetDlgItemInt(IDC_MATRIX_ROWS, NULL, FALSE);
	cfg_matrixCols = GetDlgItemInt(IDC_MATRIX_COLS, NULL, FALSE);
	cfg_brightness = GetDlgItemInt(IDC_BRIGHTNESS, NULL, FALSE);
	cfg_updateInterval = GetDlgItemInt(IDC_UPDATE_INTERVAL, NULL, FALSE);
	cfg_comPort = GetDlgItemInt(IDC_COM_PORT, NULL, FALSE);
//	cfg_startLed = GetDlgItemInt(IDC_START_LED, NULL, FALSE);
//	cfg_ledDirection = GetDlgItemInt(IDC_LED_DIR, NULL, FALSE);

//	cfg_lineStyle = GetDlgItemInt(IDC_LINE_STYLE, NULL, FALSE);
	cfg_logFrequency = IsDlgButtonChecked(IDC_LOG_FREQ);
	cfg_logAmplitude = IsDlgButtonChecked(IDC_LOG_AMPL);
	cfg_peakValues = IsDlgButtonChecked(IDC_PEAK_VALS);

	r = SendDlgItemMessage(IDC_START_LED, CB_GETCURSEL, 0, 0);
	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_startLedId); n++) {
		if (r == cfg_startLedId[n]) {
			cfg_startLed = n;
			break;
		}
	}
	r = SendDlgItemMessage(IDC_LED_DIR, CB_GETCURSEL, 0, 0);
	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_ledDirId); n++) {
		if (r == cfg_ledDirId[n]) {
			cfg_ledDirection = n;
			break;
		}
	}
	r = SendDlgItemMessage(IDC_LINE_STYLE, CB_GETCURSEL, 0, 0);
	for (UINT n = 0; n < CALC_TAB_ELEMENTS(cfg_lineStyleId); n++) {
		if (r == cfg_lineStyleId[n]) {
			cfg_lineStyle = n;
			break;
		}
	}

	cbChanged = false;


	// stop output
	isActive = StopOutput();

	// write configuration values into the driver
	ConfigMatrix(cfg_matrixRows, cfg_matrixCols, cfg_startLed, cfg_ledDirection);
	SetBrightness(cfg_brightness);
	SetInterval(cfg_updateInterval);
	SetComPort(cfg_comPort);

	SetLineStyle(cfg_lineStyle);
	SetScaling(cfg_logFrequency, cfg_logAmplitude, cfg_peakValues);

	// restart the output
	if (isActive)
		StartOutput();

	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	bool changed = false;

	changed |= GetDlgItemInt(IDC_MATRIX_ROWS, NULL, FALSE) != cfg_matrixRows;
	changed |= GetDlgItemInt(IDC_MATRIX_COLS, NULL, FALSE) != cfg_matrixCols;
	changed |= GetDlgItemInt(IDC_BRIGHTNESS, NULL, FALSE) != cfg_brightness;
	changed |= GetDlgItemInt(IDC_UPDATE_INTERVAL, NULL, FALSE) != cfg_updateInterval;
	changed |= GetDlgItemInt(IDC_COM_PORT, NULL, FALSE) != cfg_comPort;
//	changed |= GetDlgItemInt(IDC_START_LED, NULL, FALSE) != cfg_startLed;
//	changed |= GetDlgItemInt(IDC_LED_DIR, NULL, FALSE) != cfg_ledDirection;

//	changed |= GetDlgItemInt(IDC_LINE_STYLE, NULL, FALSE) != cfg_lineStyle;
	changed |= IsDlgButtonChecked(IDC_LOG_FREQ) != cfg_logFrequency;
	changed |= IsDlgButtonChecked(IDC_LOG_AMPL) != cfg_logAmplitude;
	changed |= IsDlgButtonChecked(IDC_PEAK_VALS) != cfg_peakValues;

	changed |= cbChanged;

	return changed;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
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

static preferences_page_factory_t<preferences_page_myimpl> g_preferences_page_myimpl_factory;


unsigned int GetCfgComPort()
{
	return cfg_comPort;
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
