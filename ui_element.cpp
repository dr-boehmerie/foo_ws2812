#include "stdafx.h"

//#include <libPPUI/win32_utility.h>
#include <libPPUI/win32_op.h> // WIN32_OP()
//#include <libPPUI/wtl-pp.h> // CCheckBox
//#include <helpers/atl-misc.h> // ui_element_impl
#include <helpers/BumpableElem.h>

class CWS2812ElemWindow : public ui_element_instance, public CWindowImpl<CWS2812ElemWindow>, private play_callback_impl_base {
public:
	// ATL window class declaration. Replace class name with your own when reusing code.
	DECLARE_WND_CLASS_EX(TEXT("{606FCA51-7651-4665-9344-56B517ECDF5A}"),CS_VREDRAW | CS_HREDRAW,(-1));

	void initialize_window(HWND parent) {
		WIN32_OP(Create(parent, 0, 0, 0, WS_EX_STATICEDGE) != NULL);
		m_parent = parent;
		m_state = "";
		m_timerId = -1;
		m_timerInterval = 100;
		m_xyRc = { 0, 0, 100, 100 };

		// ask the global visualisation manager to create a stream for us
		if (m_visStream == nullptr) {
			static_api_ptr_t<visualisation_manager>()->create_stream(m_visStream, visualisation_manager::KStreamFlagNewFFT);

			// I probably should test this
			if (m_visStream != nullptr) {
				// stereo is preferred
				m_visStream->set_channel_mode(visualisation_stream_v2::channel_mode_default);
			}
		}
	}

	BEGIN_MSG_MAP(ui_element_dummy)
		MESSAGE_HANDLER(WM_LBUTTONDOWN,OnLButtonDown)
		MSG_WM_CREATE(OnCreate)
		MSG_WM_TIMER(OnTimer)
		MSG_WM_ERASEBKGND(OnEraseBkgnd)
		MSG_WM_PAINT(OnPaint)
		MSG_WM_SIZE(OnSize)
	END_MSG_MAP()

	CWS2812ElemWindow(ui_element_config::ptr,ui_element_instance_callback_ptr p_callback);
	HWND get_wnd() {return *this;}
	void set_configuration(ui_element_config::ptr config) {m_config = config;}
	ui_element_config::ptr get_configuration() {return m_config;}
	static GUID g_get_guid() {
		// {60B214CF-006C-4080-A311-C24541260A58}
		static const GUID guid_ws2812elemwnd = { 0x60b214cf, 0x6c, 0x4080,{ 0xa3, 0x11, 0xc2, 0x45, 0x41, 0x26, 0xa, 0x58 } };
		return guid_ws2812elemwnd;
	}
	static GUID g_get_subclass() {return ui_element_subclass_utility;}
	static void g_get_name(pfc::string_base & out) {out = "WS2812 UI Element";}
	static ui_element_config::ptr g_get_default_configuration() {return ui_element_config::g_create_empty(g_get_guid());}
	static const char * g_get_description() {return "This is the UI Element for WS2812 output plugin.";}

	void notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size);
private:
	LRESULT OnLButtonDown(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnCreate(LPCREATESTRUCT);
	LRESULT OnTimer(UINT);
	LRESULT OnSize(UINT, CSize);
	void OnPaint(CDCHandle);
	BOOL OnEraseBkgnd(CDCHandle);

	void UpdateXY(void);

	ui_element_config::ptr m_config;
	HWND m_parent;
	HDC m_xyDc;
	CRect m_xyRc;
	CRect m_textRc;
	int m_timerId;
	UINT32 m_timerInterval;

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

	pfc::string_formatter m_state;

	titleformat_object::ptr m_script;

	static_api_ptr_t<playback_control> m_playback_control;

	service_ptr_t<visualisation_stream_v3> m_visStream;


protected:
	// this must be declared as protected for ui_element_impl_withpopup<> to work.
	const ui_element_instance_callback_ptr m_callback;
};
void CWS2812ElemWindow::notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size) {
	if (p_what == ui_element_notify_colors_changed || p_what == ui_element_notify_font_changed) {
		// we use global colors and fonts - trigger a repaint whenever these change.
		Invalidate();
	}
}
CWS2812ElemWindow::CWS2812ElemWindow(ui_element_config::ptr config,ui_element_instance_callback_ptr p_callback) : m_callback(p_callback), m_config(config) {
}

BOOL CWS2812ElemWindow::OnEraseBkgnd(CDCHandle dc)
{
	CRect rc; WIN32_OP_D( GetClientRect(&rc) );
	CBrush brush;
	WIN32_OP_D( brush.CreateSolidBrush( m_callback->query_std_color(ui_color_background) ) != NULL );
	WIN32_OP_D( dc.FillRect(&rc, brush) );
	return TRUE;
}

void CWS2812ElemWindow::OnPaint(CDCHandle)
{
	CPaintDC dc(*this);

	dc.SetTextColor( m_callback->query_std_color(ui_color_text) );
	dc.SetBkMode(TRANSPARENT);

	SelectObjectScope fontScope(dc, (HGDIOBJ) m_callback->query_font_ex(ui_font_default) );
	const UINT format = DT_NOPREFIX | /*DT_CENTER | DT_VCENTER |*/ DT_SINGLELINE;
	CRect rc;
	WIN32_OP_D( GetClientRect(&rc) );
//	WIN32_OP_D( dc.DrawText(_T("This is a sample element."), -1, &rc, format) > 0 );

	rc.MoveToXY(10, 10);
	// string8 needs to be converted to wchar
	WCHAR wtemp[100];
	mbstowcs_s(NULL, wtemp, m_state.c_str(), m_state.length());
	WIN32_OP_D(dc.DrawText(wtemp, -1, &rc, format) > 0);
#if 0
	POINT points[4] = { {10, 10}, {12, 20}, {40, 50}, {100, 30} };
	BYTE  types[4] = { PT_MOVETO, PT_LINETO, PT_LINETO, PT_LINETO };

	WIN32_OP_D( dc.PolyDraw(points, types, 4) );
#endif
}

LRESULT CWS2812ElemWindow::OnCreate(LPCREATESTRUCT para)
{
	CRect rc;
	LONG length;

	// Calculate the starting point.  
	WIN32_OP_D(GetClientRect(&rc));

	length = rc.Height();
	if (length > rc.Width())
		length = rc.Width();

	// 10% off in every direction
	m_xyRc.left = rc.left + length / 10;
	m_xyRc.top = rc.top + length / 10;

	m_textRc.left = rc.left;
	m_textRc.top = rc.top;
	m_textRc.right = rc.left + length;
	m_textRc.bottom = rc.top + length / 10;

	length -= length / 5;
	m_xyRc.right = m_xyRc.left + length;
	m_xyRc.bottom = m_xyRc.top + length;

	// Initialize the private DC.
	m_xyDc = GetDC();
//	WIN32_OP_D(SetViewportOrgEx(m_xyDc, m_xyRc.left, m_xyRc.top, NULL));

	return 0L;
}

LRESULT CWS2812ElemWindow::OnTimer(UINT id)
{
	if (id == 1) {
		UpdateXY();
	}
	return 0L;
}

LRESULT CWS2812ElemWindow::OnSize(UINT wParam, CSize size)
{
	CRect rc;
	LONG length;

	switch (wParam)
	{
	case SIZE_MINIMIZED:
		// Stop the timer if the window is minimized.
		KillTimer(1);
		m_timerId = -1;
		break;

	case SIZE_RESTORED:
		// update xy rect
		WIN32_OP_D(GetClientRect(&rc));

		length = rc.Height();
		if (length > rc.Width())
			length = rc.Width();

		// 10% off in every direction
		m_xyRc.left = rc.left + length / 10;
		m_xyRc.top = rc.top + length / 10;

		m_textRc.left = rc.left;
		m_textRc.top = rc.top;
		m_textRc.right = rc.left + length;
		m_textRc.bottom = rc.top + length / 10;

		length -= length / 5;
		m_xyRc.right = m_xyRc.left + length;
		m_xyRc.bottom = m_xyRc.top + length;

	//	WIN32_OP_D(SetViewportOrgEx(m_xyDc, m_xyRc.left, m_xyRc.top, NULL));

		// Fall through to the next case.
	case SIZE_MAXIMIZED:
		// Start the timer if it had been stopped.  
		if (m_timerId == -1)
			SetTimer(m_timerId = 1, m_timerInterval, NULL);
		break;
	}

	return 0L;
}

LRESULT CWS2812ElemWindow::OnLButtonDown(UINT, WPARAM, LPARAM, BOOL&) {
	/*m_callback->request_replace(this);*/
	return 0;
}

void CWS2812ElemWindow::update() {
	BOOL redraw = false;

	if (m_script.is_empty()) {
		pfc::string8 pattern = "%codec% | %bitrate% kbps | %samplerate% Hz | %channels% | %playback_time%[ / %length%]$if(%ispaused%, | paused,)";
		//	uGetDlgItemText(*this, IDC_PATTERN, pattern);
		static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(m_script, pattern);
	}
	pfc::string_formatter state;
	if (m_playback_control->playback_format_title(NULL, state, m_script, NULL, playback_control::display_level_all)) {
		//Succeeded already.
	}
	else if (m_playback_control->is_playing()) {
		//Starting playback but not done opening the first track yet.
		state = "Opening...";
		redraw = true;
	}
	else {
		state = "Stopped.";
		redraw = true;
	}
//	uSetDlgItemText(*this, IDC_STATE, state);
	m_state = state;

	if (::IsWindowVisible(get_wnd())) {
	//	CPaintDC dc(*this);
	//	PAINTSTRUCT ps;

	//	BeginPaint(&ps);

		CRect rc = m_textRc;
	//	WIN32_OP_D(GetClientRect(&rc));

		if (redraw) {
			CBrush brush;
			WIN32_OP_D(brush.CreateSolidBrush(m_callback->query_std_color(ui_color_background)) != NULL);
			WIN32_OP_D(FillRect(m_xyDc, &rc, brush));
		}
		SetTextColor(m_xyDc, m_callback->query_std_color(ui_color_text));
		SetBkMode(m_xyDc, OPAQUE);

		SelectObjectScope fontScope(m_xyDc, (HGDIOBJ)m_callback->query_font_ex(ui_font_default));
		const UINT format = DT_NOPREFIX | /*DT_CENTER | DT_VCENTER |*/ DT_SINGLELINE;

		rc.MoveToXY(10, 10);

		// string8 needs to be converted to wchar
		WCHAR wtemp[100];
		WIN32_OP_D(mbstowcs_s(NULL, wtemp, m_state.c_str(), m_state.length()) == 0);
		WIN32_OP_D(DrawText(m_xyDc, wtemp, -1, &rc, format) > 0);

	//	UpdateXY();
	//	EndPaint(&ps);
	}
	else if (0)
	{
		LRESULT returnCode;
		HWND bnHwnd = get_wnd();

		if (::IsWindow(bnHwnd)) {

			returnCode = ::SendMessage(bnHwnd, WM_PAINT, FALSE, 0);
		}
	}
}

void CWS2812ElemWindow::UpdateXY(void)
{
	if (::IsWindowVisible(get_wnd())) {
		if (m_visStream != nullptr && m_playback_control->is_playing()) {
			double	abs_time;

			// get current track time
			if (m_visStream->get_absolute_time(abs_time)) {
				audio_chunk_fast_impl	chunk;

				// read samples before and after the current track time
				double		length = (double)m_timerInterval * 1e-3;	//audioLength;

				if (abs_time >= length) {
					// enough time has passed -> double the length
					abs_time -= length;
					length *= 2;
				}

				// audio data
				if (m_visStream->get_chunk_absolute(chunk, abs_time, length)) {
					// number of channels, should be 1 or 2
					unsigned int	channels = chunk.get_channel_count();
					// number of samples in the chunk, fft_size / 2
					t_size			samples = chunk.get_sample_count();
					unsigned int	sample_rate = chunk.get_sample_rate();
					// peak sample value
					audio_sample	peak = 1.0;	//chunk.get_peak();

					int				x0, y0, n;
					int				length;
					audio_sample	m;
					int				x, y;
					COLORREF		fg = RGB(255, 0, 0);
					COLORREF		bg = m_callback->query_std_color(ui_color_background);

					// calc output window
					length = m_xyRc.Height();

					// 10% off in every direction
					x0 = m_xyRc.left;	// m_xyRc.left + length / 10;
					y0 = m_xyRc.top;	// m_xyRc.top + length / 10;
				//	length -= length / 5;
					m = (audio_sample)length / 2;
					n = (int)m;

					// update viewport
				//	WIN32_OP_D(SetViewportOrgEx(m_xyDc, m_xyRc.left, m_xyRc.top, NULL));

					// clear background
					CBrush brush;
					CRect rcbg = m_xyRc;
				//	rcbg.SetRect(x0, y0, x0 + length, y0 + length);
					WIN32_OP_D(brush.CreateSolidBrush(bg) != NULL);
					WIN32_OP_D(FillRect(m_xyDc, &rcbg, brush));

					// center position
					x0 += n;
					y0 += n;

					// convert samples
					const audio_sample *psample = chunk.get_data();
					if (psample != nullptr) {
						if (channels == 2) {
							for (t_size sample_index = 0; sample_index < samples; sample_index += 2) {
								audio_sample sx = psample[sample_index + 0];
								audio_sample sy = psample[sample_index + 1];

								// calc pixel position (mirror y)
								x = x0 + (int)(m * sx);
								y = y0 - (int)(m * sy);

								SetPixel(m_xyDc, x, y, fg);
							}
						}
					}
				}
			}
		}
	}
}

// ui_element_impl_withpopup autogenerates standalone version of our component and proper menu commands. Use ui_element_impl instead if you don't want that.
class ui_element_myimpl : public ui_element_impl_withpopup<CWS2812ElemWindow> {};

static service_factory_single_t<ui_element_myimpl> g_ui_element_myimpl_factory;
