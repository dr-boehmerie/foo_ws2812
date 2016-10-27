#include "stdafx.h"

//static const GUID g_mainmenu_group_id = { 0x44963e7a, 0x4b2a, 0x4588, { 0xb0, 0x17, 0xa8, 0x69, 0x18, 0xcb, 0x8a, 0xa5 } };
// {064011FA-CFE4-4F47-A735-3FAE1D6FE5D3}
static const GUID g_mainmenu_group_id = { 0x64011fa, 0xcfe4, 0x4f47,{ 0xa7, 0x35, 0x3f, 0xae, 0x1d, 0x6f, 0xe5, 0xd3 } };
static const GUID g_mainmenu_entry_id = { 0x140c7ad4, 0x9176, 0x4afe,{ 0xa0, 0x31, 0x2f, 0x6b, 0xa3, 0x82, 0x42, 0x8c } };



static mainmenu_group_popup_factory g_mainmenu_group(g_mainmenu_group_id, mainmenu_groups::view_visualisations, mainmenu_commands::sort_priority_dontcare, "WS2812 Output");

static mainmenu_group_factory g_mainmenu_entry(g_mainmenu_entry_id, mainmenu_groups::view_visualisations, mainmenu_commands::sort_priority_dontcare);

#include "foo_ws2812.h"


class mainmenu_group_commands_ws2812 : public mainmenu_commands {
public:
	enum {
		cmd_playbackstate = 0,
		cmd_show_control,
		cmd_total
	};
	t_uint32 get_command_count() {
		return cmd_total;
	}
	GUID get_command(t_uint32 p_index) {
		//static const GUID guid_toggle_output = { 0x7c4726df, 0x3b2d, 0x4c7c, { 0xad, 0xe8, 0x43, 0xd8, 0x46, 0xbe, 0xce, 0xa8 } };
		// {9D87536E-F82A-4613-8ED2-820C06546B63}
		static const GUID guid_show_control = { 0x9d87536e, 0xf82a, 0x4613,{ 0x8e, 0xd2, 0x82, 0xc, 0x6, 0x54, 0x6b, 0x63 } };
		//static const GUID guid_playbackstate = { 0xbd880c51, 0xf0cc, 0x473f, { 0x9d, 0x14, 0xa6, 0x6e, 0x8c, 0xed, 0x25, 0xae } };
		// {BCF3A4A8-9436-45C6-AC75-420003C0D965}
		static const GUID guid_playbackstate = { 0xbcf3a4a8, 0x9436, 0x45c6,{ 0xac, 0x75, 0x42, 0x0, 0x3, 0xc0, 0xd9, 0x65 } };

		switch(p_index) {
			case cmd_playbackstate: return guid_playbackstate;
			case cmd_show_control: return guid_show_control;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	void get_name(t_uint32 p_index,pfc::string_base & p_out) {
		switch(p_index) {
			case cmd_playbackstate: p_out = "Playback state demo"; break;
			case cmd_show_control: p_out = "Control dialog"; break;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	bool get_description(t_uint32 p_index,pfc::string_base & p_out) {
		switch(p_index) {
			case cmd_playbackstate: p_out = "Opens the playback state demo dialog."; return true;
			case cmd_show_control: p_out = "Opens the control dialog."; return true;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	GUID get_parent() {
	//	return g_mainmenu_group_id;
		return g_mainmenu_entry_id;
	}
	void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback) {
		switch(p_index) {
			case cmd_playbackstate:
				RunPlaybackStateDemo();
				break;
			case cmd_show_control:
			//	popup_message::g_show("This is a sample menu command.", "Blah");
			//	ToggleOutput();
				RunWS2812ControlDialog();
				break;
			default:
				uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
};

class mainmenu_entry_commands_ws2812 : public mainmenu_commands {
public:
	enum {
		cmd_show_control = 0,
		cmd_total
	};
	t_uint32 get_command_count() {
		return cmd_total;
	}
	GUID get_command(t_uint32 p_index) {
		//static const GUID guid_toggle_output = { 0x7c4726df, 0x3b2d, 0x4c7c, { 0xad, 0xe8, 0x43, 0xd8, 0x46, 0xbe, 0xce, 0xa8 } };
		// {9D87536E-F82A-4613-8ED2-820C06546B63}
		static const GUID guid_show_control = { 0x9d87536e, 0xf82a, 0x4613,{ 0x8e, 0xd2, 0x82, 0xc, 0x6, 0x54, 0x6b, 0x63 } };
		//static const GUID guid_playbackstate = { 0xbd880c51, 0xf0cc, 0x473f, { 0x9d, 0x14, 0xa6, 0x6e, 0x8c, 0xed, 0x25, 0xae } };
		// {BCF3A4A8-9436-45C6-AC75-420003C0D965}
		static const GUID guid_playbackstate = { 0xbcf3a4a8, 0x9436, 0x45c6,{ 0xac, 0x75, 0x42, 0x0, 0x3, 0xc0, 0xd9, 0x65 } };

		switch (p_index) {
		case cmd_show_control: return guid_show_control;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	void get_name(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case cmd_show_control: p_out = "WS2812 Output"; break;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	bool get_description(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case cmd_show_control: p_out = "Opens the WS2812 control dialog."; return true;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	GUID get_parent() {
		//	return g_mainmenu_group_id;
		return g_mainmenu_entry_id;
	}
	void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) {
		switch (p_index) {
		case cmd_show_control:
			//	popup_message::g_show("This is a sample menu command.", "Blah");
			//	ToggleOutput();
			RunWS2812ControlDialog();
			break;
		default:
			uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
};

//static mainmenu_commands_factory_t<mainmenu_group_commands_ws2812> g_mainmenu_commands_ws2812_factory;
static mainmenu_commands_factory_t<mainmenu_entry_commands_ws2812> g_mainmenu_commands_ws2812_factory;
