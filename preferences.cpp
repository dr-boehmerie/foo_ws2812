#include "stdafx.h"
#include "resource.h"

// Sample preferences interface: two meaningless configuration settings accessible through a preferences page and one accessible through advanced preferences.


// These GUIDs identify the variables within our component's configuration file.
//static const GUID guid_cfg_matrixRows = { 0xbd5c777, 0x735c, 0x440d, { 0x8c, 0x71, 0x49, 0xb6, 0xac, 0xff, 0xce, 0xb8 } };
// {FDCD346D-3B1B-413C-9CAE-3A221AA6993B}
static const GUID guid_cfg_matrixRows = { 0xfdcd346d, 0x3b1b, 0x413c,{ 0x9c, 0xae, 0x3a, 0x22, 0x1a, 0xa6, 0x99, 0x3b } };
//static const GUID guid_cfg_matrixCols = { 0x752f1186, 0x9f61, 0x4f91, { 0xb3, 0xee, 0x2f, 0x25, 0xb1, 0x24, 0x83, 0x5d } };
// {BF7C1CF7-8DD7-4394-822B-04D42047ED6F}
static const GUID guid_cfg_matrixCols = { 0xbf7c1cf7, 0x8dd7, 0x4394,{ 0x82, 0x2b, 0x4, 0xd4, 0x20, 0x47, 0xed, 0x6f } };

// This GUID identifies our Advanced Preferences branch (replace with your own when reusing code).
//static const GUID guid_advconfig_branch = { 0x28564ced, 0x4abf, 0x4f0c, { 0xa4, 0x43, 0x98, 0xda, 0x88, 0xe2, 0xcd, 0x39 } };
// {6FABE4A9-2C7F-4FA9-9598-7480C8324250}
static const GUID guid_advconfig_branch = { 0x6fabe4a9, 0x2c7f, 0x4fa9,{ 0x95, 0x98, 0x74, 0x80, 0xc8, 0x32, 0x42, 0x50 } };
// This GUID identifies our Advanced Preferences setting (replace with your own when reusing code) as well as this setting's storage within our component's configuration file.
//static const GUID guid_cfg_bogoSetting3 = { 0xf7008963, 0xed60, 0x4084,{ 0xa8, 0x5d, 0xd1, 0xcd, 0xc5, 0x51, 0x22, 0xca } };
// {B462CBE2-7561-4AC9-B8DE-9297EE1FD7A0}
static const GUID guid_cfg_bogoSetting3 = { 0xb462cbe2, 0x7561, 0x4ac9,{ 0xb8, 0xde, 0x92, 0x97, 0xee, 0x1f, 0xd7, 0xa0 } };


enum {
	default_cfg_matrixRows = 8,
	default_cfg_matrixCols = 30,
	default_cfg_bogoSetting3 = 42,
};

static cfg_uint cfg_matrixRows(guid_cfg_matrixRows, default_cfg_matrixRows);
static cfg_uint cfg_matrixCols(guid_cfg_matrixCols, default_cfg_matrixCols);

static advconfig_branch_factory g_advconfigBranch("WS2812 Output", guid_advconfig_branch, advconfig_branch::guid_branch_tools, 0);

static advconfig_integer_factory cfg_bogoSetting3("Bogo setting 3", guid_cfg_bogoSetting3, guid_advconfig_branch, 0, default_cfg_bogoSetting3, 0 /*minimum value*/, 9999 /*maximum value*/);

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

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
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;
};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	SetDlgItemInt(IDC_MATRIX_ROWS, cfg_matrixRows, FALSE);
	SetDlgItemInt(IDC_MATRIX_COLS, cfg_matrixCols, FALSE);
	return FALSE;
}

void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	// not much to do here
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
	OnChanged();
}

void CMyPreferences::apply() {
	cfg_matrixRows = GetDlgItemInt(IDC_MATRIX_ROWS, NULL, FALSE);
	cfg_matrixCols = GetDlgItemInt(IDC_MATRIX_COLS, NULL, FALSE);
	
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	return GetDlgItemInt(IDC_MATRIX_ROWS, NULL, FALSE) != cfg_matrixRows || GetDlgItemInt(IDC_MATRIX_COLS, NULL, FALSE) != cfg_matrixCols;
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
		//static const GUID guid = { 0x7702c93e, 0x24dc, 0x48ed, { 0x8d, 0xb1, 0x3f, 0x27, 0xb3, 0x8c, 0x7c, 0xc9 } };
		// {4B176673-4152-497C-BFEE-BA2C5173C7A7}
		static const GUID guid = { 0x4b176673, 0x4152, 0x497c,{ 0xbf, 0xee, 0xba, 0x2c, 0x51, 0x73, 0xc7, 0xa7 } };

		return guid;
	}
	GUID get_parent_guid() {return guid_tools;}
};

static preferences_page_factory_t<preferences_page_myimpl> g_preferences_page_myimpl_factory;
