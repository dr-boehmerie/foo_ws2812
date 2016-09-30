#include "stdafx.h"

// Sample initquit implementation. See also: initquit class documentation in relevant header.

void InitOutput();
void DeinitOutput();


class myinitquit : public initquit {
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

static initquit_factory_t<myinitquit> g_myinitquit_factory;
