#pragma once

/*
 * foo_ws2812.h : Defines all sorts of values, variables and functions.
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

#include <cstdint>
#include <vector>

#define CALC_TAB_ELEMENTS(_TAB_)	(sizeof(_TAB_)/sizeof(_TAB_[0]))

//#define MAKE_COLOR(_R_,_G_,_B_)		((((_R_) & 0xFF) << 16) | (((_G_) & 0xFF) << 8) | ((_B_) & 0xFF))
#define MAKE_COLOR(_R_,_G_,_B_,_W_)	((((_W_) & 0xFF) << 24) | (((_R_) & 0xFF) << 16) | (((_G_) & 0xFF) << 8) | ((_B_) & 0xFF))
#define GET_COLOR_R(_RGB_)			(((_RGB_) >> 16) & 0xFF)
#define GET_COLOR_G(_RGB_)			(((_RGB_) >> 8) & 0xFF)
#define GET_COLOR_B(_RGB_)			((_RGB_) & 0xFF)
#define GET_COLOR_W(_WRGB_)			(((_WRGB_) >> 24) & 0xFF)

#define SET_COLOR_R(_RGB_,_R_)		((_RGB_) & 0xFF00FFFF) | (((_R_) & 0xFF) << 16)
#define SET_COLOR_G(_RGB_,_G_)		((_RGB_) & 0xFFFF00FF) | (((_G_) & 0xFF) << 8)
#define SET_COLOR_B(_RGB_,_B_)		((_RGB_) & 0xFFFFFF00) | (((_B_) & 0xFF) << 0)
#define SET_COLOR_W(_WRGB_,_W_)		((_WRGB_) & 0x00FFFFFF) | (((_W_) & 0xFF) << 24)

enum class ws2812_style
{
	spectrum_simple = 0,		// simple white bars
	spectrum_green_red_bars,	// bars from green to red, each row a seperate color
	spectrum_fire_lines,		// bars from green to red, peak color applied to the whole bar

	spectrogram_horizontal,		// spectrogram, moving horizontally
	spectrogram_vertical,		// spectrogram, moving vertically

	oscilloscope_yt,			// oscilloscope, single colored, yt-mode
	oscilloscope_xy,			// oscilloscope, single colored, xy-mode

	oscillogram_horizontal,		// oscillogram, moving horizontally
	oscillogram_vertical,		// oscillogram, moving vertically

	led_test,					// all leds the same color, user values rgbw

	eNo
};

enum class ws2812_start_led
{
	top_left = 0,
	top_right,
	bottom_left,
	bottom_right,

	eNo
};

enum class ws2812_led_direction
{
	common = 0,
	alternating,

	eNo
};

enum class ws2812_led_mode
{
	top_left_common = 0,
	top_left_alternating,
	top_right_common,
	top_right_alternating,
	bottom_left_common,
	bottom_left_alternating,
	bottom_right_common,
	bottom_right_alternating,

	eNo
};

enum class ws2812_led_colors
{
	eGRB,		// WS2812 default
	eBRG,		// Renkforce (TM1829)
	eRGB,
	eGRBW,		// SK6812GRBW
	eRGBW,		// SK6812RGBW

	eNo
};

enum class ws2812_baudrate
{
	e9600,
	e14400,
	e19200,
	e38400,
	e56000,
	e57600,
	e115200,
	e128000,
	e250000,
	e256000,
	e500000,

	eNo
};

class ws2812
{
public:
	ws2812();
	~ws2812();

	//ws2812(unsigned int rows, unsigned int cols, unsigned int port, ws2812_baudrate baudrate, unsigned int interval, ws2812_style style);

	bool ConfigMatrix(int rows, int cols, unsigned int start_led, unsigned int led_dir, unsigned int led_colors);
	bool SetComPort(unsigned int port);
	bool SetComBaudrate(ws2812_baudrate baudrate);

	void SetLedTestVal(unsigned int idx, unsigned int val);

	void SetBrightness(unsigned int brightness);
	void GetBrightness(unsigned int &brightness);

	void SetCurrentLimit(unsigned int limit);
	void GetCurrentLimit(unsigned int &limit);

	bool SetLedStripeSof(const char *pattern);

	void SetStyle(ws2812_style style);
	void GetLineStyle(unsigned int &style);

	void SetLedColors(ws2812_led_colors ledColors);
	void GetLedColors(unsigned int & ledColors);

	void SetScaling(int logFrequency, int logAmplitude, int peakValues);
	void GetScaling(int &logFrequency, int &logAmplitude, int &peakValues);

	void SetInterval(unsigned int interval);
	void GetInterval(unsigned int &interval);

	void SetAmplitudeMinMax(ws2812_style style, int min, int max);

	void SetAmplitudeMinMax(int min, int max);
	void GetAmplitudeMinMax(int &min, int &max);

	void SetFrequencyMinMax(ws2812_style style, int min, int max);

	void SetFrequencyMinMax(int min, int max);
	void GetFrequencyMinMax(int &min, int &max);

	bool StartTimer();
	bool StopTimer();

	bool StartOutput(void);
	bool StopOutput(void);
	bool ToggleOutput(void);
	bool GetOutputState(void);
	void CalcAndOutput(void);

	bool InitColorTab(void);
	bool InitColorTab(const unsigned int *initTab, unsigned int tabElements);
	bool InitColorTab(const char *pattern);

private:
	BOOL OpenPort(LPCWSTR gszPort, unsigned int port);
	BOOL ClosePort();
	BOOL WriteABuffer(const unsigned char * lpBuf, DWORD dwToWrite);

	bool AllocateBuffers();
	void FreeBuffers();
	void InitAmplitudeMinMax();
	void InitFrequencyMinMax();

	void ClearPersistence(void);
	void ClearOutputBuffer(void);
	void ClearCounterBuffer(void);
	void ClearImageBuffer(void);

	void InitIndexLut(void);
	unsigned int LedIndex(unsigned int row, unsigned int col);
	ws2812_led_mode GetLedMode(unsigned int startLed, unsigned int ledDir);
	void CalcColorSimple(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcRowColor(audio_sample row, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcRowColor(audio_sample row, unsigned int & r, unsigned int & g, unsigned int & b, unsigned int & w);
	void CalcColorColoredRows(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcColorColoredRows(unsigned int row, audio_sample sample, unsigned int & r, unsigned int & g, unsigned int & b, unsigned int & w);
	void CalcColorColoredBars(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcColorColoredBars(unsigned int row, audio_sample sample, unsigned int & r, unsigned int & g, unsigned int & b, unsigned int & w);
	void GetColor(unsigned int index, unsigned int &r, unsigned int &g, unsigned int &b);
	void GetColor(const unsigned int index, unsigned int & r, unsigned int & g, unsigned int & b, unsigned int & w);
	void CalcPersistenceMax(unsigned int &c, unsigned int &p_c);
	void CalcPersistenceAdd(unsigned int &c, unsigned int &p_c);
	void AddPersistenceSpectrum(unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b);
	void AddPersistenceSpectrum(const unsigned int index, unsigned int & r, unsigned int & g, unsigned int & b, unsigned int & w);
	void AddPersistenceOscilloscope(unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b);
	void AddPersistenceOscilloscope(const unsigned int index, unsigned int & r, unsigned int & g, unsigned int & b, unsigned int & w);
	void ApplyBrightness(unsigned int brightness, unsigned int &r, unsigned int &g, unsigned int &b);
	void ApplyBrightness(unsigned int brightness, unsigned int & r, unsigned int & g, unsigned int & b, unsigned int & w);
	void ColorsToImage(unsigned int led_index, unsigned int r, unsigned int g, unsigned int b);
	void ColorsToImage(unsigned int led_index, unsigned int r, unsigned int g, unsigned int b, unsigned int w);
	void ClipColors(unsigned int &r, unsigned int &g, unsigned int &b);
	void ClipColors(unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &w);
	void GetColorIndexes(unsigned int & r, unsigned int & g, unsigned int & b, unsigned int & w, unsigned int & no);
	void ImageToBuffer(unsigned int offset, unsigned int count, unsigned int bufferSize);
	void LedTestToBuffer(unsigned int offset, unsigned int count, unsigned int bufferSize);
	void ImageScrollLeft(void);
	void ImageScrollRight(void);
	void ImageScrollUp(void);
	void ImageScrollDown(void);

	void OutputTest(const audio_sample *psample, unsigned int samples, audio_sample peak, unsigned char *buffer, unsigned int bufferSize);
	void OutputSpectrumBars(const audio_sample *psample, unsigned int samples, unsigned int channels, audio_sample peak, audio_sample delta_f);
	void OutputSpectrogram(const audio_sample *psample, unsigned int samples, unsigned int channels, audio_sample peak, audio_sample delta_f);
	void OutputOscilloscopeYt(const audio_sample *psample, unsigned int samples, unsigned int samplerate, unsigned int channels, audio_sample peak);
	void OutputOscilloscopeXy(const audio_sample * psample, unsigned int samples, unsigned int samplerate, unsigned int channels, audio_sample peak);
	void OutputOscillogram(const audio_sample *psample, unsigned int samples, unsigned int samplerate, unsigned int channels, audio_sample peak);

	void OutputImage();
	void OutputTest();

public:
	static const unsigned int	rows_min = 1;
	static const unsigned int	rows_max = 300;
	static const unsigned int	rows_def = 8;

	static const unsigned int	columns_min = 1;
	static const unsigned int	columns_max = 300;
	static const unsigned int	columns_def = 8;

	static const unsigned int	port_min = 1;
	static const unsigned int	port_max = 127;
	static const unsigned int	port_def = 3;

	static const unsigned int	brightness_min = 0;			// %
	static const unsigned int	brightness_max = 80;		// %; limited by power supply (max. 60mA per LED)
	static const unsigned int	brightness_def = 25;		// %

	static const unsigned int	sof_def = 1;				// Start of frame

	static const unsigned int	timerInterval_min = 50;		// ms
	static const unsigned int	timerInterval_max = 500;	// ms
	static const unsigned int	timerInterval_def = 330;	// ms

	static const int			frequency_min = 10;			// Hz
	static const int			frequency_max = 22050;		// Hz

	static const int			amplitude_min = -100;		// dB
	static const int			amplitude_max = 10;			// dB

	static const int			gain_oscilloscope_min = 10;		// => * 0.1
	static const int			gain_oscilloscope_max = 300;	// => * 3.0
	static const int			gain_oscilloscope_div = 100;	// => / 100

	static const int			offset_oscilloscope_min = -100;	// => -1.0
	static const int			offset_oscilloscope_max = 100;	// => 1.0
	static const int			offset_oscilloscope_div = 100;	// => / 100

	// green > red
	const unsigned int		spectrumColorTab[3] = { MAKE_COLOR(0, 255, 0, 0), MAKE_COLOR(200, 200, 0, 0), MAKE_COLOR(255, 0, 0, 0) };
	// black > blue > green > yellow > red
	const unsigned int		spectrogramColorTab[5] = { MAKE_COLOR(0, 0, 0, 0), MAKE_COLOR(0, 0, 127, 0), MAKE_COLOR(0, 200, 0, 0), MAKE_COLOR(200, 200, 0, 0), MAKE_COLOR(255, 0, 0, 0) };
	// black > white
	const unsigned int		oscilloscopeColorTab[2] = { MAKE_COLOR(0, 0, 0, 0), MAKE_COLOR(255, 255, 255, 0) };

private:
	unsigned int			m_rows{ 0 };
	unsigned int			m_columns{ 0 };
	unsigned int			m_ledNo{ 0 };

	unsigned int			m_brightness{ 0 };
	unsigned int			m_ledBrightnessMax{ 250 };
	unsigned int			m_currentLimit{ 0 };
	ws2812_led_mode			m_ledMode{ ws2812_led_mode::top_left_common };
	ws2812_style			m_lineStyle{ ws2812_style::spectrum_simple };
	ws2812_led_colors		m_ledColors{ ws2812_led_colors::eGRB };
	bool					m_rgbw{ false };

	bool					m_logFrequency{ false };
	bool					m_logAmplitude{ false };
	bool					m_peakValues{ false };

	unsigned int			m_fftSize{ 0 };
	double					m_audioLength{ 0.0 };

	DWORD					m_timerStartDelay{ 0 };
	DWORD					m_timerInterval{ 0 };

	volatile bool			m_initDone{ false };
	volatile bool			m_timerStarted{ false };
	volatile bool			m_timerActive{ false };

	HANDLE					m_hComm{ INVALID_HANDLE_VALUE };
	HANDLE					m_hTimer{ INVALID_HANDLE_VALUE };
	DWORD					m_commErr{ 0 };

	unsigned int			m_comPort{ 0 };
	enum ws2812_baudrate	m_comBaudrate { ws2812_baudrate::e115200 };

	unsigned int				m_bufferSize{ 0 };		// 1 + m_ledNo * 4
	unsigned int				m_outputSize{ 0 };		// 1 + m_ledNo * (4 if m_ledColors == rgbw, 3 otherwise)
	std::vector<unsigned char>	m_outputBuffer;			// data to be sent to the arduino
	std::vector<unsigned char>	m_ledSof;				// Start of Frame values
	std::vector<unsigned int>	m_imageBuffer;			// image (xRGB)
	std::vector<unsigned int>	m_persistenceBuffer;	// image persistence (xRGB)
	std::vector<unsigned int>	m_counterBuffer;		// pixel hit count in oscilloscope styles

	std::vector<unsigned int>	m_indexLut;				// led index table (depends on start led and directions)

	const unsigned int			m_colorNo{ 1000 };
	std::vector<unsigned int>	m_colorTab;				// xRGB

	unsigned int			m_testColor{ 0 };			// WRGB

	audio_sample			m_colorsPerRow{ 0 };
	audio_sample			m_colorsPerCol{ 0 };

	std::vector<int>		m_freqMin;
	std::vector<int>		m_freqMax;

	std::vector<int>		m_amplMin;
	std::vector<int>		m_amplMax;

	service_ptr_t<visualisation_stream_v3>	visStream;
};


// foo_ws2812.cpp
void InitOutput();
void DeinitOutput();

bool StopOutput(void);
bool StartOutput(void);
bool ToggleOutput(void);
bool GetOutputState(void);

bool ConfigMatrix(int rows, int cols, unsigned int start_led, unsigned int led_dir, unsigned int led_colors);
bool SetScaling(int logFrequency, int logAmplitude, int peakValues);
bool SetStyle(unsigned int lineStyle);
bool SetBrightness(unsigned int brightness);
bool SetCurrentLimit(unsigned int limit);
bool SetLedStripeSof(const char *pattern);

bool SetAmplitudeMinMax(int min, int max);
bool SetFrequencyMinMax(int min, int max);

void SaveAmplitudeMinMax();
void SaveFrequencyMinMax();

bool SetComPort(unsigned int port);
bool SetComBaudrate(unsigned int baudrate);

bool SetInterval(unsigned int interval);

bool GetScaling(int &logFrequency, int &logAmplitude, int &peakValues);
bool GetInterval(unsigned int &interval);
bool GetBrightness(unsigned int &brightness);
bool GetCurrentLimit(unsigned int &brightness);
bool GetLineStyle(unsigned int &lineStyle);
bool GetLedColors(unsigned int &ledColors);
bool GetMinAmplitudeIsOffset(void);
bool GetMaxAmplitudeIsGain(void);
bool GetLedTestMode(void);
bool SetLedTestVal(unsigned int idx, unsigned int val);

bool GetAmplitudeMinMax(int &min, int &max);
bool GetFrequencyMinMax(int &min, int &max);

bool InitColorTab(const char *pattern);

unsigned int GetRowLimited(unsigned int rows);
unsigned int GetColumnsLimited(unsigned int columns);
unsigned int GetIntervalLimited(unsigned int interval);
unsigned int GetBrightnessLimited(unsigned int brightness);


// control_dialog.cpp
void RunWS2812ControlDialog();

// preferences.cpp
unsigned int GetCfgComPort();
unsigned int GetCfgComBaudrate();
unsigned int GetCfgMatrixRows();
unsigned int GetCfgMatrixCols();
unsigned int GetCfgBrightness();
unsigned int GetCfgUpdateInterval();
unsigned int GetCfgStartLed();
unsigned int GetCfgLedDirection();
unsigned int GetCfgLedColors();

unsigned int GetCfgLineStyle();
unsigned int GetCfgLogFrequency();
unsigned int GetCfgLogAmplitude();
unsigned int GetCfgPeakValues();
const char * GetCfgSpectrumColors();
const char * GetCfgSpectrumBarColors();
const char * GetCfgSpectrumFireColors();
const char * GetCfgSpectrogramColors();
const char * GetCfgOscilloscopeColors();
const char * GetCfgLedStripeSof();
void GetCfgSpectrumAmplitudeMinMax(int &min, int &max);
void GetCfgSpectrogramAmplitudeMinMax(int &min, int &max);
void GetCfgOscilloscopeOffsetAmplitude(int &min, int &max);
void GetCfgSpectrumFrequencyMinMax(int &min, int &max);
void GetCfgSpectrogramFrequencyMinMax(int &min, int &max);

bool SetCfgComPort(unsigned int value);
bool SetCfgComBaudrate(unsigned int value);
bool SetCfgMatrixRows(unsigned int value);
bool SetCfgMatrixCols(unsigned int value);
bool SetCfgBrightness(unsigned int value);
bool SetCfgUpdateInterval(unsigned int value);
bool SetCfgStartLed(unsigned int value);
bool SetCfgLedDirection(unsigned int value);
bool SetCfgLedColors(unsigned int value);

bool SetCfgLineStyle(unsigned int value);
bool SetCfgLogFrequency(unsigned int value);
bool SetCfgLogAmplitude(unsigned int value);
bool SetCfgPeakValues(unsigned int value);

bool SetCfgSpectrumAmplitudeMinMax(int min, int max);
bool SetCfgSpectrogramAmplitudeMinMax(int min, int max);
bool SetCfgOscilloscopeOffsetAmplitude(int offset, int amplitude);

bool SetCfgSpectrumFrequencyMinMax(int min, int max);
bool SetCfgSpectrogramFrequencyMinMax(int min, int max);
