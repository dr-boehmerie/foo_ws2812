#pragma once


enum line_style
{
	ws2812_spectrum_simple = 0,
	ws2812_spectrum_green_red_bars,
	ws2812_spectrum_fire_lines,

	ws2812_spectrogram,

	ws2812_oscilloscope,

	ws2812_line_style_no
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

private:
	BOOL OpenPort(LPCWSTR gszPort, unsigned int port);
	BOOL ClosePort();
	BOOL WriteABuffer(const unsigned char * lpBuf, DWORD dwToWrite);
	bool CreateBuffer();

	unsigned int LedIndex(unsigned int row, unsigned int col);
	void CalcColorWhiteBar(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcRowColor(audio_sample row, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcColorColoredRows(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcColorColoredBars(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcColor(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b);
	void CalcPersistence(unsigned int &c, unsigned int &p_c);
	void AddPersistence(unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b);
	void ApplyBrightness(unsigned int brightness, unsigned int &r, unsigned int &g, unsigned int &b);
	void LedToBuffer(unsigned char *buffer, unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b);

	void OutputTest(const audio_sample *psample, unsigned int samples, audio_sample peak, unsigned char *buffer, unsigned int bufferSize);
	void OutputSpectrumBars(const audio_sample *psample, unsigned int samples, audio_sample peak, audio_sample delta_f, unsigned char *buffer);

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

private:
	unsigned int		rows;
	unsigned int		columns;

	unsigned int		brightness;
	enum led_mode		ledMode;
	enum line_style		lineStyle;

	bool				logFrequency;
	bool				logAmplitude;
	bool				peakValues;

	unsigned int		fftSize;

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
