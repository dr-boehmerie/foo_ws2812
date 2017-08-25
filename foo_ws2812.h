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

#define CALC_TAB_ELEMENTS(_TAB_)	(sizeof(_TAB_)/sizeof(_TAB_[0]))

#define MAKE_COLOR(_R_,_G_,_B_)		((((_R_) & 0xFF) << 16) | (((_G_) & 0xFF) << 8) | ((_B_) & 0xFF))
#define GET_COLOR_R(_RGB_)			(((_RGB_) >> 16) & 0xFF)
#define GET_COLOR_G(_RGB_)			(((_RGB_) >> 8) & 0xFF)
#define GET_COLOR_B(_RGB_)			((_RGB_) & 0xFF)

enum line_style
{
	ws2812_spectrum_simple = 0,			// simple white bars
	ws2812_spectrum_green_red_bars,		// bars from green to red, each row a seperate color
	ws2812_spectrum_fire_lines,			// bars from green to red, peak color applied to the whole bar

	ws2812_spectrogram_horizontal,		// spectrogram, moving horizontally
	ws2812_spectrogram_vertical,		// spectrogram, moving vertically

	ws2812_oscilloscope_yt,				// oscilloscope, single colored, yt-mode
	ws2812_oscilloscope_xy,				// oscilloscope, single colored, xy-mode

	ws2812_oscillogram_horizontal,		// oscillogram, moving horizontally
	ws2812_oscillogram_vertical,		// oscillogram, moving vertically

	ws2812_line_style_no
};

enum start_led
{
	ws2812_top_left = 0,
	ws2812_top_right,
	ws2812_bottom_left,
	ws2812_bottom_right,

	ws2812_start_led_no
};

enum led_direction
{
	ws2812_led_dir_common = 0,
	ws2812_led_dir_alternating,

	ws2812_led_dir_no
};

enum led_mode
{
	ws2812_led_mode_top_left_common = 0,
	ws2812_led_mode_top_left_alternating,
	ws2812_led_mode_top_right_common,
	ws2812_led_mode_top_right_alternating,
	ws2812_led_mode_bottom_left_common,
	ws2812_led_mode_bottom_left_alternating,
	ws2812_led_mode_bottom_right_common,
	ws2812_led_mode_bottom_right_alternating,

	ws2812_led_mode_no
};

enum led_colors
{
	ws2812_led_colors_grb,		// WS2812 default
	ws2812_led_colors_brg,		// Renkforce (TM1829)
	ws2812_led_colors_rgb,

	ws2812_led_colors_no
};

enum ws2812_baudrate
{
	ws2812_baudrate_9600,
	ws2812_baudrate_14400,
	ws2812_baudrate_19200,
	ws2812_baudrate_38400,
	ws2812_baudrate_56000,
	ws2812_baudrate_57600,
	ws2812_baudrate_115200,
	ws2812_baudrate_128000,
	ws2812_baudrate_250000,
	ws2812_baudrate_256000,
	ws2812_baudrate_500000,

	ws2812_baudrate_no
};

class ws2812
{
public:
	ws2812();
	~ws2812();

	ws2812(unsigned int rows, unsigned int cols, unsigned int port, enum ws2812_baudrate baudrate, unsigned int interval, enum line_style style);

	bool ConfigMatrix(int rows, int cols, enum start_led start_led, enum led_direction led_dir);
	bool SetComPort(unsigned int port);
	bool SetComBaudrate(enum ws2812_baudrate baudrate);

	void SetBrightness(unsigned int brightness);
	void GetBrightness(unsigned int *brightness);

	void SetLineStyle(enum line_style style);
	void GetLineStyle(unsigned int *style);

	void SetLedColors(enum led_colors ledColors);
	void GetLedColors(unsigned int *ledColors);

	void SetScaling(int logFrequency, int logAmplitude, int peakValues);
	void GetScaling(int *logFrequency, int *logAmplitude, int *peakValues);

	void SetInterval(unsigned int interval);
	void GetInterval(unsigned int *interval);

	void SetAmplitudeMinMax(int min, int max);
	void GetAmplitudeMinMax(int *min, int *max);

	void SetFrequencyMinMax(int min, int max);
	void GetFrequencyMinMax(int *min, int *max);

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
	enum led_mode GetLedMode(unsigned int startLed, unsigned int ledDir);
	void CalcColorSimple(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcRowColor(audio_sample row, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcColorColoredRows(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcColorColoredBars(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b);
	void GetColor(unsigned int index, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcPersistenceMax(unsigned int &c, unsigned int &p_c);
	void CalcPersistenceAdd(unsigned int &c, unsigned int &p_c);
	void AddPersistenceSpectrum(unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b);
	void AddPersistenceOscilloscope(unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b);
	void ApplyBrightness(unsigned int brightness, unsigned int &r, unsigned int &g, unsigned int &b);
	void ColorsToImage(unsigned int led_index, unsigned int r, unsigned int g, unsigned int b);
	void ImageToBuffer(unsigned char * buffer, unsigned int bufferSize);
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

public:
	static const unsigned int	rows_min = 1;
	static const unsigned int	rows_max = 240;
	static const unsigned int	rows_def = 8;

	static const unsigned int	columns_min = 1;
	static const unsigned int	columns_max = 240;
	static const unsigned int	columns_def = 8;

	static const unsigned int	port_min = 1;
	static const unsigned int	port_max = 127;
	static const unsigned int	port_def = 3;

	static const unsigned int	brightness_min = 0;			// %
	static const unsigned int	brightness_max = 66;		// %; limited by power supply (max. 60mA per LED)
	static const unsigned int	brightness_def = 25;		// %

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
	const unsigned int		spectrumColorTab[3] = { MAKE_COLOR(0, 255, 0), MAKE_COLOR(200, 200, 0), MAKE_COLOR(255, 0, 0) };
	// black > blue > green > yellow > red
	const unsigned int		spectrogramColorTab[5] = { MAKE_COLOR(0, 0, 0), MAKE_COLOR(0, 0, 127), MAKE_COLOR(0, 200, 0), MAKE_COLOR(200, 200, 0), MAKE_COLOR(255, 0, 0) };
	// black > white
	const unsigned int		oscilloscopeColorTab[2] = { MAKE_COLOR(0, 0, 0), MAKE_COLOR(255, 255, 255)};

private:
	unsigned int			m_rows;
	unsigned int			m_columns;
	unsigned int			m_ledNo;

	unsigned int			m_brightness;
	enum led_mode			m_ledMode;
	enum line_style			m_lineStyle;
	enum led_colors			m_ledColors;

	bool					m_logFrequency;
	bool					m_logAmplitude;
	bool					m_peakValues;

	unsigned int			m_fftSize;
	double					m_audioLength;

	DWORD					m_timerStartDelay;
	DWORD					m_timerInterval;

	volatile bool			m_initDone;
	volatile bool			m_timerStarted;
	volatile bool			m_timerActive;

	HANDLE					m_hComm;
	HANDLE					m_hTimer;
	DWORD					m_commErr;

	unsigned int			m_comPort;
	enum ws2812_baudrate	m_comBaudrate;

	unsigned int			m_bufferSize;
	unsigned char			*m_outputBuffer;			// data to be sent to the arduino
	unsigned int			*m_imageBuffer;				// image (xRGB)
	unsigned int			*m_persistenceBuffer;		// image persistence (xRGB)
	unsigned int			*m_counterBuffer;			// pixel hit count in oscilloscope styles

	unsigned int			*m_indexLut;				// led index table (depends on start led and directions)

	const unsigned int		m_colorNo = 1000;
	unsigned int			*m_colorTab;				// xRGB

	audio_sample			m_colorsPerRow;
	audio_sample			m_colorsPerCol;

	int						m_freqMin[ws2812_line_style_no];
	int						m_freqMax[ws2812_line_style_no];

	int						m_amplMin[ws2812_line_style_no];
	int						m_amplMax[ws2812_line_style_no];

	service_ptr_t<visualisation_stream_v3>	visStream;
};


// foo_ws2812.cpp
void InitOutput();
void DeinitOutput();

bool StopOutput(void);
bool StartOutput(void);
bool ToggleOutput(void);
bool GetOutputState(void);

bool ConfigMatrix(int rows, int cols, int start_led, int led_dir);
bool SetScaling(int logFrequency, int logAmplitude, int peakValues);
bool SetLineStyle(unsigned int lineStyle);
bool SetLedColors(unsigned int ledColors);
bool SetBrightness(unsigned int brightness);

bool SetAmplitudeMinMax(int min, int max);
bool SetFrequencyMinMax(int min, int max);

void SaveAmplitudeMinMax();
void SaveFrequencyMinMax();

bool SetComPort(unsigned int port);
bool SetComBaudrate(unsigned int baudrate);

bool SetInterval(unsigned int interval);

bool GetScaling(int *logFrequency, int *logAmplitude, int *peakValues);
bool GetInterval(unsigned int *interval);
bool GetBrightness(unsigned int *brightness);
bool GetLineStyle(unsigned int *lineStyle);
bool GetLedColors(unsigned int *ledColors);
bool GetMinAmplitudeIsOffset(void);
bool GetMaxAmplitudeIsGain(void);

bool GetAmplitudeMinMax(int *min, int *max);
bool GetFrequencyMinMax(int *min, int *max);

bool InitColorTab(const char *pattern);

unsigned int GetRowLimited(unsigned int rows);
unsigned int GetColumnsLimited(unsigned int columns);
unsigned int GetIntervalLimited(unsigned int interval);
unsigned int GetBrightnessLimited(unsigned int brightness);


// playback_state.cpp
void RunPlaybackStateDemo();

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
void GetCfgSpectrumAmplitudeMinMax(int *min, int *max);
void GetCfgSpectrogramAmplitudeMinMax(int *min, int *max);
void GetCfgOscilloscopeOffsetAmplitude(int *min, int *max);
void GetCfgSpectrumFrequencyMinMax(int *min, int *max);
void GetCfgSpectrogramFrequencyMinMax(int *min, int *max);

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
