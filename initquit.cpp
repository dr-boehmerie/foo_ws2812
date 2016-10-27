#include "stdafx.h"

// Sample initquit implementation. See also: initquit class documentation in relevant header.

#include "foo_ws2812.h"


class ws2812_initquit : public initquit {
public:
	void on_init() {
		console::print("WS2812 Output: on_init()");

		InitOutput();
	}
	void on_quit() {
		console::print("WS2812 Output: on_quit()");

		DeinitOutput();
	}
};

static initquit_factory_t<ws2812_initquit> g_ws2812_initquit_factory;
