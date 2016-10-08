#pragma once

#define CALC_TAB_ELEMENTS(_TAB_)	(sizeof(_TAB_)/sizeof(_TAB_[0]))

#define MAKE_COLOR(_R_,_G_,_B_)		((((_R_) & 0xFF) << 16) | (((_G_) & 0xFF) << 8) | ((_B_) & 0xFF))
#define GET_COLOR_R(_RGB_)			(((_RGB_) >> 16) & 0xFF)
#define GET_COLOR_G(_RGB_)			(((_RGB_) >> 8) & 0xFF)
#define GET_COLOR_B(_RGB_)			((_RGB_) & 0xFF)

enum line_style
{
	ws2812_spectrum_simple = 0,
	ws2812_spectrum_green_red_bars,
	ws2812_spectrum_fire_lines,

	ws2812_spectrogram_horizontal,
	ws2812_spectrogram_vertical,

	ws2812_oscilloscope,

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

class ws2812
{
public:
	ws2812();
	~ws2812();

	ws2812(unsigned int rows, unsigned int cols, unsigned int port, unsigned int interval, enum line_style style);

	void ConfigMatrix(int rows, int cols, enum start_led start_led, enum led_direction led_dir);
	bool SetComPort(unsigned int port);
	void CalcAndOutput(void);
	void SetBrightness(unsigned int brightness);
	void SetLineStyle(enum line_style style);
	void SetScaling(int logFrequency, int logAmplitude, int peakValues);

	void SetInterval(unsigned int interval);

	bool StartTimer();
	bool StopTimer();

	bool StartOutput(void);
	bool StopOutput(void);
	bool ToggleOutput(void);

	void ClearPersistence(void);
	void ClearOutputBuffer(void);

	void InitColorTab(void);
	void InitColorTab(const unsigned int *initTab, unsigned int tabElements);

private:
	BOOL OpenPort(LPCWSTR gszPort, unsigned int port);
	BOOL ClosePort();
	BOOL WriteABuffer(const unsigned char * lpBuf, DWORD dwToWrite);
	bool AllocateBuffers();
	void FreeBuffers();

	unsigned int LedIndex(unsigned int row, unsigned int col);
	enum led_mode GetLedMode(unsigned int startLed, unsigned int ledDir);
	void CalcColorWhiteBar(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcColor(audio_sample sample, audio_sample min, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcRowColor(audio_sample row, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcColorColoredRows(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcColorColoredBars(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcColor(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcPersistenceMax(unsigned int &c, unsigned int &p_c);
	void CalcPersistenceAdd(unsigned int &c, unsigned int &p_c);
	void AddPersistenceSpectrum(unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b);
	void AddPersistenceOscilloscope(unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b);
	void ApplyBrightness(unsigned int brightness, unsigned int &r, unsigned int &g, unsigned int &b);
	void ColorsToBuffer(unsigned char *buffer, unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b);

	void OutputTest(const audio_sample *psample, unsigned int samples, audio_sample peak, unsigned char *buffer, unsigned int bufferSize);
	void OutputSpectrumBars(const audio_sample *psample, unsigned int samples, audio_sample peak, audio_sample delta_f, unsigned char *buffer);
	void OutputSpectrogram(const audio_sample *psample, unsigned int samples, audio_sample peak, audio_sample delta_f, unsigned char *buffer);
	void OutputOscilloscope(const audio_sample *psample, unsigned int samples, audio_sample peak, unsigned char *buffer);

public:
	const unsigned int	rows_min = 1;
	const unsigned int	rows_max = 32;
	const unsigned int	rows_def = 8;

	const unsigned int	columns_min = 1;
	const unsigned int	columns_max = 32;
	const unsigned int	columns_def = 8;

	const unsigned int	port_min = 1;
	const unsigned int	port_max = 127;
	const unsigned int	port_def = 3;

	const unsigned int	brightness_min = 0;
	const unsigned int	brightness_max = 100;
	const unsigned int	brightness_def = 25;

	const unsigned int	timerInterval_min = 50;
	const unsigned int	timerInterval_max = 1000;
	const unsigned int	timerInterval_def = 330;

	const unsigned int	spectrumColorTab[2] = { MAKE_COLOR(0, 255, 0), MAKE_COLOR(0, 255, 0) };
	const unsigned int	spectrogramColorTab[4] = { MAKE_COLOR(0, 0, 0), MAKE_COLOR(0, 0, 127), MAKE_COLOR(0, 200, 0), MAKE_COLOR(255, 0, 0) };
	const unsigned int	oscilloscopeColorTab[2] = { MAKE_COLOR(0, 0, 0), MAKE_COLOR(255, 255, 255)};

private:
	unsigned int		rows;
	unsigned int		columns;
	unsigned int		ledNo;

	unsigned int		brightness;
	enum led_mode		ledMode;
	enum line_style		lineStyle;

	bool				logFrequency;
	bool				logAmplitude;
	bool				peakValues;

	unsigned int		fftSize;
	double				audioLength;

	DWORD				timerStartDelay;
	DWORD				timerInterval;

	bool				initDone;
	bool				timerStarted;

	HANDLE				hComm;
	HANDLE				hTimer;
	DWORD				commErr;

	unsigned int		comPort;

	unsigned int		bufferSize;
	unsigned char		*outputBuffer;
	unsigned char		*persistenceBuffer;
	unsigned int		*counterBuffer;

	unsigned int		*indexLut;

	const unsigned int	colorNo = 1000;
	unsigned int		*colorTab;

	service_ptr_t<visualisation_stream_v3>	visStream;
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
