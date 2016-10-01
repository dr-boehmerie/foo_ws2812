#pragma once

// foo_ws2812.cpp
void InitOutput();
void DeinitOutput();

void ToggleOutput(void);

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
unsigned int GetCfgLineStyle();
unsigned int GetCfgLogFrequency();
unsigned int GetCfgLogAmplitude();
unsigned int GetCfgPeakValues();
