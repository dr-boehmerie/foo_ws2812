/*
 * mainmenu.cpp : Implentation of the visualisation menu entry.
 *
 * Based on mainmenu.cpp from the foo_sample plugin.
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

// {064011FA-CFE4-4F47-A735-3FAE1D6FE5D3}
static const GUID g_mainmenu_group_id = { 0x64011fa, 0xcfe4, 0x4f47,{ 0xa7, 0x35, 0x3f, 0xae, 0x1d, 0x6f, 0xe5, 0xd3 } };
static const GUID g_mainmenu_entry_id = { 0x140c7ad4, 0x9176, 0x4afe,{ 0xa0, 0x31, 0x2f, 0x6b, 0xa3, 0x82, 0x42, 0x8c } };



static mainmenu_group_popup_factory g_mainmenu_group(g_mainmenu_group_id, mainmenu_groups::view_visualisations, mainmenu_commands::sort_priority_dontcare, "WS2812 Output");

static mainmenu_group_factory g_mainmenu_entry(g_mainmenu_entry_id, mainmenu_groups::view_visualisations, mainmenu_commands::sort_priority_dontcare);

#include "foo_ws2812.h"


class mainmenu_group_commands_ws2812 : public mainmenu_commands {
public:
	enum {
		cmd_show_control = 0,
		cmd_total
	};
	t_uint32 get_command_count() {
		return cmd_total;
	}
	GUID get_command(t_uint32 p_index) {
		// {9D87536E-F82A-4613-8ED2-820C06546B63}
		static const GUID guid_show_control = { 0x9d87536e, 0xf82a, 0x4613,{ 0x8e, 0xd2, 0x82, 0xc, 0x6, 0x54, 0x6b, 0x63 } };

		switch(p_index) {
			case cmd_show_control: return guid_show_control;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	void get_name(t_uint32 p_index,pfc::string_base & p_out) {
		switch(p_index) {
			case cmd_show_control: p_out = "Control dialog"; break;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	bool get_description(t_uint32 p_index,pfc::string_base & p_out) {
		switch(p_index) {
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
			case cmd_show_control:
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
		// {9D87536E-F82A-4613-8ED2-820C06546B63}
		static const GUID guid_show_control = { 0x9d87536e, 0xf82a, 0x4613,{ 0x8e, 0xd2, 0x82, 0xc, 0x6, 0x54, 0x6b, 0x63 } };

		switch (p_index) {
		case cmd_show_control: return guid_show_control;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	void get_name(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case cmd_show_control: p_out = WS2812_COMPONENT_NAME; break;
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
			RunWS2812ControlDialog();
			break;
		default:
			uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
};

//static mainmenu_commands_factory_t<mainmenu_group_commands_ws2812> g_mainmenu_commands_ws2812_factory;
static mainmenu_commands_factory_t<mainmenu_entry_commands_ws2812> g_mainmenu_commands_ws2812_factory;
