// foo_ws2812.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "foo_ws2812.h"

service_ptr_t<visualisation_stream_v3> ws2812_stream;

HANDLE ws2812_hComm = INVALID_HANDLE_VALUE;
HANDLE ws2812_hTimer = INVALID_HANDLE_VALUE;

LPCWSTR const	ws2812_port_str = L"COM7";
unsigned int	ws2812_comPort = 7;

bool		ws2812_init_done = false;

unsigned int	ws2812_rows = 8;
unsigned int	ws2812_columns = 30;
unsigned int	ws2812_led_mode = 0;
unsigned int	ws2812_lineStyle = 0;

unsigned int	ws2812_brightness = 25;

DWORD		ws2812_timerStartDelay = 500;
DWORD		ws2812_timerInverval = 100;

bool		ws2812_timerStarted = false;

bool		ws2812_logFrequency = true;
bool		ws2812_peakValues = true;
bool		ws2812_logAmplitude = true;

unsigned int	ws2812_bufferSize = 0;
unsigned char	*ws2812_outputBuffer = nullptr;
unsigned char	*ws2812_persistenceBuffer = nullptr;


void OutputTest(const audio_sample *psample, int samples, audio_sample peak, unsigned char *buffer, int bufferSize);
void OutputSpectrumBars(const audio_sample *psample, unsigned int samples, audio_sample peak, unsigned char *buffer, unsigned int rows, unsigned int cols);


// COM port handling taken from the MSDN
BOOL OpenPort(LPCWSTR gszPort, unsigned int port)
{
	if (ws2812_hComm == INVALID_HANDLE_VALUE) {
		WCHAR	portStr[32];
		LPCWSTR	format = L"COM%u";

		wsprintf(portStr, format, port);

		ws2812_hComm = CreateFile(portStr,
			GENERIC_READ | GENERIC_WRITE,
			0,
			0,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			0);
	}
	if (ws2812_hComm == INVALID_HANDLE_VALUE) {
		// error opening port; abort
		return false;
	}

	DCB dcb;

	FillMemory(&dcb, sizeof(dcb), 0);

	// get current DCB
	if (GetCommState(ws2812_hComm, &dcb)) {
		// Update DCB rate.
	//	dcb.BaudRate = CBR_9600;
		// Disable flow controls
		dcb.fOutxCtsFlow = false;
		dcb.fOutxDsrFlow = false;
		dcb.fRtsControl = RTS_CONTROL_DISABLE;

		// Set new state.
		if (!SetCommState(ws2812_hComm, &dcb)) {
			// Error in SetCommState. Possibly a problem with the communications 
			// port handle or a problem with the DCB structure itself.
		}
	}
	return true;
}

BOOL ClosePort()
{
	if (ws2812_hComm != INVALID_HANDLE_VALUE) {
		CloseHandle(ws2812_hComm);
		ws2812_hComm = INVALID_HANDLE_VALUE;
		return true;
	}
	return false;
}

BOOL WriteABuffer(const unsigned char * lpBuf, DWORD dwToWrite)
{
	OVERLAPPED osWrite = { 0 };
	DWORD dwWritten;
	DWORD dwRes;
	BOOL fRes;

	// Port not opened?
	if (ws2812_hComm == INVALID_HANDLE_VALUE)
		return false;

	// Create this write operation's OVERLAPPED structure's hEvent.
	osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (osWrite.hEvent == NULL) {
		// error creating overlapped event handle
		return FALSE;
	}

	// Issue write.
	if (!WriteFile(ws2812_hComm, lpBuf, dwToWrite, &dwWritten, &osWrite)) {
		if (GetLastError() != ERROR_IO_PENDING) {
			// WriteFile failed, but isn't delayed. Report error and abort.
			fRes = FALSE;
		}
		else {
			// Write is pending.
			dwRes = WaitForSingleObject(osWrite.hEvent, INFINITE);
		}
		switch (dwRes)
		{
			// OVERLAPPED structure's event has been signaled. 
		case WAIT_OBJECT_0:
			if (!GetOverlappedResult(ws2812_hComm, &osWrite, &dwWritten, FALSE))
				fRes = FALSE;
			else
				// Write operation completed successfully.
				fRes = TRUE;
			break;

		default:
			// An error has occurred in WaitForSingleObject.
			// This usually indicates a problem with the
			// OVERLAPPED structure's event handle.
			fRes = FALSE;
			break;
		}
	} else {
		// WriteFile completed immediately.
		fRes = TRUE;
	}

	CloseHandle(osWrite.hEvent);
	return fRes;
}

VOID CALLBACK WaitOrTimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	double	abs_time;

	// get current track time
	if (ws2812_stream->get_absolute_time(abs_time)) {
		audio_chunk_fast_impl	chunk;
		unsigned int			fft_size = 16 * 1024;

		// FFT data
		if (ws2812_stream->get_spectrum_absolute(chunk, abs_time, fft_size)) {
			// number of channels, should be 1
			unsigned int channels = chunk.get_channel_count();
			// number of samples in the chunk, fft_size / 2
			int samples = chunk.get_sample_count();
			unsigned int sample_rate = chunk.get_sample_rate();
			// peak sample value
			audio_sample peak = chunk.get_peak();

			// convert samples
			const audio_sample *psample = chunk.get_data();
			if (ws2812_outputBuffer != nullptr && psample != nullptr && samples > 0) {

				// a value of 1 is used as start byte
				// and must not occur in other bytes in the buffer
				ws2812_outputBuffer[0] = 1;

				OutputSpectrumBars(psample, samples, peak, &ws2812_outputBuffer[1], ws2812_rows, ws2812_columns);

				// send buffer
				WriteABuffer(ws2812_outputBuffer, ws2812_bufferSize);

			} else {
				// no samples ?
			}
		} else {
			// no FFT data ?
		}
	} else {
		// playback hasn't started yet...
	}
}

bool StartTimer()
{
	if (CreateTimerQueueTimer(&ws2812_hTimer, NULL,
		WaitOrTimerCallback, NULL, ws2812_timerStartDelay,
		ws2812_timerInverval, WT_EXECUTELONGFUNCTION))
	{
		// success
		ws2812_timerStarted = true;
	} else {
		// failed
		ws2812_timerStarted = false;
	}
	return ws2812_timerStarted;
}

bool StopTimer()
{
	ws2812_timerStarted = false;

	if (ws2812_hTimer != INVALID_HANDLE_VALUE) {
		DeleteTimerQueueTimer(NULL, ws2812_hTimer, NULL);
		ws2812_hTimer = INVALID_HANDLE_VALUE;
		return true;
	}
	return false;
}

void InitOutput()
{
	// ask the global visualisation manager to create a stream for us
	static_api_ptr_t<visualisation_manager>()->create_stream(ws2812_stream, visualisation_manager::KStreamFlagNewFFT);

	// I probably should test this
	if (ws2812_stream != nullptr) {
		// mono is preferred, unless you want to use two displays ;-)
		ws2812_stream->set_channel_mode(visualisation_stream_v2::channel_mode_mono);
#if 0
		// open the COM port
		if (OpenPort(ws2812_port_str, 7)) {
			// init done :-)
			ws2812_init_done = true;
		}
#endif
		// read configuration values
		ws2812_comPort = GetCfgComPort();
		if (ws2812_comPort < 1 || ws2812_comPort > 127)
			ws2812_comPort = 1;

		ws2812_rows = GetCfgMatrixRows();
		if (ws2812_rows < 1 || ws2812_rows > 30)
			ws2812_rows = 8;

		ws2812_columns = GetCfgMatrixCols();
		if (ws2812_columns < 1 || ws2812_columns > 30)
			ws2812_columns = 8;

		ws2812_timerInverval = GetCfgUpdateInterval();
		if (ws2812_timerInverval < 50 || ws2812_timerInverval > 1000)
			ws2812_timerInverval = 250;

		ws2812_brightness = GetCfgBrightness();
		if (ws2812_brightness < 1 || ws2812_brightness > 100)
			ws2812_brightness = 25;

		ws2812_lineStyle = GetCfgLineStyle();
		if (ws2812_lineStyle > 2)
			ws2812_lineStyle = 0;

		ws2812_logFrequency = GetCfgLogFrequency() != 0;
		ws2812_logAmplitude = GetCfgLogAmplitude() != 0;
		ws2812_peakValues = GetCfgPeakValues() != 0;

		// 240 LEDs, RGB for each, plus one start byte
		ws2812_bufferSize = 1 + 3 * ws2812_rows * ws2812_columns;
		ws2812_outputBuffer = new unsigned char[ws2812_bufferSize];
		ws2812_persistenceBuffer = new unsigned char[ws2812_bufferSize];

		if (ws2812_outputBuffer && ws2812_persistenceBuffer) {
			memset(ws2812_outputBuffer, 0, ws2812_bufferSize);
			memset(ws2812_persistenceBuffer, 0, ws2812_bufferSize);

			ws2812_init_done = true;
		}
	}
}

void DeinitOutput()
{
	// how to delete the stream?
//	delete ws2812_stream;

	// kill the timer
	StopTimer();

	// close the COM port
	ClosePort();

	if (ws2812_outputBuffer)
		delete ws2812_outputBuffer;
	ws2812_outputBuffer = nullptr;

	if (ws2812_persistenceBuffer)
		delete ws2812_persistenceBuffer;
	ws2812_persistenceBuffer = nullptr;

	ws2812_init_done = false;
}

void OutputTest(const audio_sample *psample, int samples, audio_sample peak, unsigned char *buffer, int bufferSize)
{
	int		i;
	audio_sample	sample;
	int		bright = ws2812_brightness;

	// limit analyzed samples to size of buffer
	if (samples > bufferSize)
		samples = bufferSize;

	for (i = 0; i < samples; i++) {
		sample = psample[i];

		sample = fabs(bright * (sample / peak));

		// all values < 2 are replaced by 0
		if (sample < 2)
			buffer[i] = 0;
		else
			buffer[i] = (unsigned char)(sample + 0.5);
	}

	// clear the rest of the buffer
	for (; i < bufferSize; i++)
		buffer[i] = 0;
}

unsigned int LedIndex(unsigned int row, unsigned int col, unsigned int rows, unsigned int cols, int mode)
{
	unsigned int	result = 0;

	switch (mode)
	{
	case 0:		// Top Left, Common direction left to right
		result = row * cols + col;
		break;

	case 1:		// Top Right, Common direction right to left
		result = row * cols + (cols - 1 - col);
		break;

	case 2:		// Bottom Left, Common direction left to right
		result = (rows - 1 - row) * cols + col;
		break;

	case 3:		// Bottom Right, Common direction right to left
		result = (rows - 1 - row) * cols + (cols - 1 - col);
		break;

	case 4:		// Top Left, Alternating direction left to right/right to left
		break;
	}

	if (result < rows * cols)
		return result;

	return 0;
}

void CalcColorWhiteBar(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b)
{
	if ((audio_sample)row > sample) {
		// off
		r = g = b = 0;
	}
	else if ((audio_sample)row == floor(sample)) {
		// peak
		r = g = b = 255;
	}
	else {
		// between
		r = g = b = 255 / 4;
	}
}

void CalcRowColor(audio_sample row, unsigned int &r, unsigned int &g, unsigned int &b)
{
	audio_sample	row_max = (audio_sample)ws2812_rows - 1;
	audio_sample	r_start, g_start, b_start;
	audio_sample	r_end, g_end, b_end;

	// green
	r_start = 0; g_start = 255; b_start = 0;
	// to red
	r_end = 255; g_end = 0; b_end = 0;

	// row = 0 to (rows - 1) => green to red via yellow
	row = row / row_max;
	if (row < 0.0) {
		r_end = r_start;
		g_end = g_start;
		b_end = b_start;
	}
	else if (row < 1.0) {
		r_end = row * (r_end - r_start) + r_start;
		g_end = row * (g_end - g_start) + g_start;
		b_end = row * (b_end - b_start) + b_start;
	}
	r = (unsigned int)r_end;
	g = (unsigned int)g_end;
	b = (unsigned int)b_end;
}

void CalcColorColoredRows(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b)
{
	if ((audio_sample)row > sample) {
		// off
		r = g = b = 0;
	}
	else if ((audio_sample)row == floor(sample)) {
		// peak
		CalcRowColor((audio_sample)row, r, g, b);
	}
	else {
		// between
		CalcRowColor((audio_sample)row, r, g, b);
		// reduce brightness
		r /= 4;
		g /= 4;
		b /= 4;
	}
}

void CalcColorColoredBars(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b)
{
	if ((audio_sample)row > sample) {
		// off
		r = g = b = 0;
	}
	else if ((audio_sample)row == floor(sample)) {
		// peak
		CalcRowColor(sample, r, g, b);
	}
	else {
		// between
		CalcRowColor(sample, r, g, b);
		// reduce brightness
		r /= 4;
		g /= 4;
		b /= 4;
	}
}

void CalcColor(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b)
{
	switch (ws2812_lineStyle)
	{
	case 0:		// simple white bars
		if ((audio_sample)row > sample) {
			// off
			r = g = b = 0;
		}
		else if ((audio_sample)row == floor(sample)) {
			// peak
			r = g = b = 255;
		}
		else {
			// between
			r = g = b = 255 / 4;
		}
		break;

	case 1:		// from green to red, each row a single color
		if ((audio_sample)row > sample) {
			// off
			r = g = b = 0;
		}
		else if ((audio_sample)row == floor(sample)) {
			// peak
			r = g = b = 255;
		}
		else {
			// between
			r = g = b = 255 / 4;
		}
		break;

	case 2:		// from green to red, each bar the same color (fire style)
		if ((audio_sample)row > sample) {
			// off
			r = g = b = 0;
		}
		else if ((audio_sample)row == floor(sample)) {
			// peak
			r = g = b = 255;
		}
		else {
			// between
			r = g = b = 255 / 4;
		}
		break;
	}
}

void CalcPersistence(unsigned int &c, unsigned int &p_c)
{
	if (c >= p_c) {
		p_c = c;
	}
	else {
		if (0) {
			c += p_c;	// lots of flicker...
			if (c > 255)
				c = 255;
		}
		else {
			if (c < p_c)
				c = p_c;
		}
		p_c /= 2;
	}
}

void AddPersistence(unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b)
{
	unsigned int	p_r, p_g, p_b;

	p_r = ws2812_persistenceBuffer[3 * led_index + 0];
	p_g = ws2812_persistenceBuffer[3 * led_index + 1];
	p_b = ws2812_persistenceBuffer[3 * led_index + 2];

	CalcPersistence(r, p_r);
	CalcPersistence(g, p_g);
	CalcPersistence(b, p_b);

	ws2812_persistenceBuffer[3 * led_index + 0] = p_r;
	ws2812_persistenceBuffer[3 * led_index + 1] = p_g;
	ws2812_persistenceBuffer[3 * led_index + 2] = p_b;
}

void ApplyBrightness(unsigned int brightness, unsigned int &r, unsigned int &g, unsigned int &b)
{
	// apply brightness
	r = (r * brightness) / 255;
	g = (g * brightness) / 255;
	b = (b * brightness) / 255;
}

void LedToBuffer(unsigned char *buffer, unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b)
{
	// all values < 2 are replaced by 0 (1 is reserved for start of block)
	if (r < 2)	r = 0;
	if (g < 2)	g = 0;
	if (b < 2)	b = 0;

	// write colors to buffer
	// color ouput order: GRB
	buffer[3 * led_index + 0] = (unsigned char)g;
	buffer[3 * led_index + 1] = (unsigned char)r;
	buffer[3 * led_index + 2] = (unsigned char)b;
}

void OutputSpectrumBars(const audio_sample *psample, unsigned int samples, audio_sample peak, unsigned char *buffer, unsigned int rows, unsigned int cols)
{
	unsigned int	bar, cnt, row;
	unsigned int	bar_cnt;
	unsigned int	samples_per_bar;
	unsigned int	sample_index;
	unsigned int	led_index;
	unsigned int	r, g, b;
	unsigned int	bright = (255 * ws2812_brightness) / 100;
	int				line_style = ws2812_lineStyle;
	int				led_mode = ws2812_led_mode;
	audio_sample	sum;
	audio_sample	sample;
	audio_sample	db_per_row;
	audio_sample	db_max;
	audio_sample	db_min;
	double			log_mult;

	// vertical bars
	bar_cnt = cols;

	// limits
	db_max = -10;
	db_min = (audio_sample)-10 * (audio_sample)rows;
	// 10 db per row
	db_per_row = (db_max - db_min) / (audio_sample)rows;

	// multiple samples added together for one bar
	samples_per_bar = samples / bar_cnt;
	if (samples_per_bar < 1) {
		bar_cnt = samples;
		samples_per_bar = 1;
	}

	// fixed peak value
	peak = (audio_sample)(1.0 * samples_per_bar);

	sample_index = 0;

	log_mult = log10(samples);
	log_mult /= (double)bar_cnt;

	for (bar = 0; bar < bar_cnt; bar++) {
		if (ws2812_logFrequency == false) {
			// linear frequency scaling

			if (bar == (bar_cnt - 1)) {
				// last bar -> remaining samples
				samples_per_bar = samples - sample_index;
			}
			else {
				// prevent index overflow
				if ((sample_index + samples_per_bar) >= samples)
					samples_per_bar = samples - sample_index;
			}

			if (ws2812_peakValues == false) {
				// calc mean value of samples per bar
				sum = 0;
				for (cnt = 0; cnt < samples_per_bar; cnt++) {
					sample = psample[sample_index + cnt];
					sum += fabs(sample);
				}

				if (ws2812_logAmplitude) {
					// calc ratio to peak, logarithmic scale: volts to db
					sample = 20 * log10(sum / peak);
				}
				else {
					// calc ratio to peak, linear scale
					sample = (sum / peak) * (db_max - db_min) + db_min;
				}
			}
			else {
				// find peak value of samples per bar
				sum = 0;
				for (cnt = 0; cnt < samples_per_bar; cnt++) {
					sample = psample[sample_index + cnt];
					sample = fabs(sample);
					if (sum < sample)
						sum = sample;
				}

				if (ws2812_logAmplitude) {
					// logarithmic scale: volts to db
					sample = 20 * log10(sum);
				}
				else {
					// linear scale
					sample = sum * (db_max - db_min) + db_min;
				}
			}
			sample_index += samples_per_bar;
		}
		else {
			// logarithmic frequency scaling
			// number of samples per bar depends on bar index

			if (bar == (bar_cnt - 1)) {
				// last bar -> remaining samples
				samples_per_bar = samples - sample_index;
			}
			else {
				double	tmp;

				tmp = log_mult * (double)(bar + 1);
				tmp = pow(10, tmp);

				// count of samples to the next index
				if (tmp <= (double)sample_index)
					samples_per_bar = 0;
				else
					samples_per_bar = (unsigned int)tmp - sample_index;

				// minimum sample count (trial and error, should depend somehow on the max fft frequency...)
				if (samples_per_bar < 5)
					samples_per_bar = 5;

				// prevent index overflow
				if (sample_index >= samples)
					samples_per_bar = 0;
				else if ((sample_index + samples_per_bar) >= samples)
					samples_per_bar = samples - sample_index;
			}

			if (ws2812_peakValues == false) {
				// calc mean value of samples per bar
				sum = 0;
				for (cnt = 0; cnt < samples_per_bar; cnt++) {
					sample = psample[sample_index + cnt];
					sum += fabs(sample);
				}

				if (ws2812_logAmplitude) {
					// calc ratio to peak, logarithmic scale: volts to db
					sample = 20 * log10(sum / peak);
				}
				else {
					// calc ratio to peak, linear scale
					sample = (sum / peak) * (db_max - db_min) + db_min;
				}
			}
			else {
				// find peak value of samples per bar
				sum = 0;
				for (cnt = 0; cnt < samples_per_bar; cnt++) {
					sample = psample[sample_index + cnt];
					sample = fabs(sample);
					if (sum < sample)
						sum = sample;
				}

				if (ws2812_logAmplitude) {
					// logarithmic scale: volts to db
					sample = 20 * log10(sum);
				}
				else {
					// linear scale
					sample = sum * (db_max - db_min) + db_min;
				}
			}
			sample_index += samples_per_bar;
		}

		// calc height of bar in rows
		// 0 db is max, -10 * (rows - 1) is min
		if (sample >= db_max) {
			sample = (audio_sample)rows;

		} else if (sample < db_min) {
			sample = (audio_sample)0.0;

		} else {
			sample = (sample / db_per_row) + (audio_sample)rows;
		}

		// colorize the bar
		switch (line_style)
		{
		default:
		case 0:		// simple white bars
			for (row = 1; row <= rows; row++) {
				// led order: start is top left
				led_index = LedIndex(rows - row, bar, rows, cols, led_mode);

				CalcColorWhiteBar(row, sample, r, g, b);

				AddPersistence(led_index, r, g, b);

				ApplyBrightness(bright, r, g, b);

				LedToBuffer(buffer, led_index, r, g, b);
			}
			break;

		case 1:		// from green to red, each row a single color
			for (row = 1; row <= rows; row++) {
				// led order: start is top left
				led_index = LedIndex(rows - row, bar, rows, cols, led_mode);

				CalcColorColoredRows(row, sample, r, g, b);

				AddPersistence(led_index, r, g, b);

				ApplyBrightness(bright, r, g, b);

				LedToBuffer(buffer, led_index, r, g, b);
			}
			break;

		case 2:		// fire style: from green to red the whole bar a single color

			for (row = 1; row <= rows; row++) {
				// led order: start is top left
				led_index = LedIndex(rows - row, bar, rows, cols, led_mode);

				CalcColorColoredBars(row, sample, r, g, b);

				AddPersistence(led_index, r, g, b);

				ApplyBrightness(bright, r, g, b);

				LedToBuffer(buffer, led_index, r, g, b);
			}
			break;
		}
	}

	// clear the rest of the buffer
//	for (; i < 3 * rows * cols; i++)
//		buffer[i] = 0;
}

void ToggleOutput(void)
{
//	try {
		if (ws2812_init_done) {
			// toggle timer state
			if (ws2812_timerStarted == false) {
				if (OpenPort(NULL, ws2812_comPort)) {
					StartTimer();
				}
			} else {
				StopTimer();
				ClosePort();
			}
		} else {
			popup_message::g_show("Not initialised!", "WS2812 Output", popup_message::icon_error);
		}
//	}
//	catch (std::exception const & e) {
//		popup_message::g_complain("WS2812 Output exception", e);
//	}
}


void ConfigMatrix(int rows, int cols, int start_led, int led_dir)
{
	bool	timerRunning;

	if (rows > 0 && rows < 30 && cols > 0 && cols < 30) {
		ws2812_rows = rows;
		ws2812_columns = cols;

		// kill the timer
		timerRunning = StopTimer();

		if (ws2812_outputBuffer)
			delete ws2812_outputBuffer;
		ws2812_outputBuffer = nullptr;

		if (ws2812_persistenceBuffer)
			delete ws2812_persistenceBuffer;
		ws2812_persistenceBuffer = nullptr;

		ws2812_init_done = false;

		// 240 LEDs, RGB for each, plus one start byte
		ws2812_bufferSize = 1 + 3 * ws2812_rows * ws2812_columns;
		ws2812_outputBuffer = new unsigned char[ws2812_bufferSize];
		ws2812_persistenceBuffer = new unsigned char[ws2812_bufferSize];

		if (ws2812_outputBuffer && ws2812_persistenceBuffer) {
			memset(ws2812_outputBuffer, 0, ws2812_bufferSize);
			memset(ws2812_persistenceBuffer, 0, ws2812_bufferSize);

			ws2812_init_done = true;

			if (timerRunning) {
				StartTimer();
			}
		}
	}
}

void SetScaling(int logFrequency, int logAmplitude, int peakValues)
{
	if (logFrequency == 0)
		ws2812_logFrequency = false;
	else if (logFrequency > 0)
		ws2812_logFrequency = true;

	if (logAmplitude == 0)
		ws2812_logAmplitude = false;
	else if (logAmplitude > 0)
		ws2812_logAmplitude = true;

	if (peakValues == 0)
		ws2812_peakValues = false;
	else if (peakValues > 0)
		ws2812_peakValues = true;
}

void SetLineStyle(unsigned int lineStyle)
{
	if (lineStyle < 3)
		ws2812_lineStyle = lineStyle;
}

void SetBrightness(unsigned int brightness)
{
	bool	timerRunning;

	if (brightness > 0 && brightness <= 100) {
		timerRunning = StopTimer();

		ws2812_brightness = brightness;

		if (timerRunning) {
			StartTimer();
		}
	}
}

void SetComPort(unsigned int port)
{
	bool	timerRunning;

	if (port > 0 && port < 127 && port != ws2812_comPort) {
		ws2812_comPort = port;

		timerRunning = StopTimer();

		ClosePort();

		if (timerRunning) {
			if (OpenPort(NULL, ws2812_comPort)) {
				StartTimer();
			}
		}
	}
}

void SetInterval(unsigned int interval)
{
	if (interval >= 50 && interval < 1000 && interval != ws2812_timerInverval) {
		ws2812_timerInverval = interval;
		if (ws2812_hTimer != INVALID_HANDLE_VALUE) {
			ChangeTimerQueueTimer(NULL, ws2812_hTimer, ws2812_timerStartDelay, ws2812_timerInverval);
		}
	}
}