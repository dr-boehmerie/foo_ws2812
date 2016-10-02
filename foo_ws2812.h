#pragma once


enum line_style
{
	ws2812_spectrum_simple = 0,
	ws2812_spectrum_green_red_bars,
	ws2812_spectrum_fire_lines,

	ws2812_spectrogram,

	ws2812_oscilloscope,
};

enum start_led
{
	ws2812_top_left = 0,
	ws2812_top_right,
	ws2812_bottom_left,
	ws2812_bottom_right,
};

enum led_direction
{
	ws2812_led_dir_common = 0,
	ws2812_led_dir_alternating,
};


// foo_ws2812.cpp
void InitOutput();
void DeinitOutput();

bool StopOutput(void);
bool StartOutput(void);
bool ToggleOutput(void);

void ConfigMatrix(int rows, int cols, int start_led, int led_dir);
void SetScaling(int logFrequency, int logAmplitude, int peakValues);
void SetLineStyle(unsigned int lineStyle);
void SetBrightness(unsigned int brightness);

void SetComPort(unsigned int port);

void SetInterval(unsigned int interval);

// playback_state.cpp
void RunPlaybackStateDemo();

// preferences.cpp
unsigned int GetCfgComPort();
unsigned int GetCfgMatrixRows();
unsigned int GetCfgMatrixCols();
unsigned int GetCfgBrightness();
unsigned int GetCfgUpdateInterval();
unsigned int GetCfgStartLed();
unsigned int GetCfgLedDirection();

unsigned int GetCfgLineStyle();
unsigned int GetCfgLogFrequency();
unsigned int GetCfgLogAmplitude();
unsigned int GetCfgPeakValues();
