// foo_ws2812.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "foo_ws2812.h"


// global class instance
ws2812			*ws2812_global = nullptr;



// COM port handling taken from the MSDN
BOOL ws2812::OpenPort(LPCWSTR gszPort, unsigned int port)
{
	if (hComm == INVALID_HANDLE_VALUE) {
		WCHAR	portStr[32];
		LPCWSTR	format = L"COM%u";

		wsprintf(portStr, format, port);

		hComm = CreateFile(portStr,
			GENERIC_READ | GENERIC_WRITE,
			0,
			0,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			0);
	}
	if (hComm == INVALID_HANDLE_VALUE) {
		// error opening port; abort
		commErr = GetLastError();
		return false;
	}

	DCB dcb;

	ZeroMemory(&dcb, sizeof(dcb));

	// get current DCB
	if (GetCommState(hComm, &dcb)) {
		// Update DCB rate.
	//	dcb.BaudRate = CBR_9600;
		// Disable flow controls
		dcb.fOutxCtsFlow = false;
		dcb.fOutxDsrFlow = false;
		dcb.fRtsControl = RTS_CONTROL_DISABLE;

		// Set new state.
		if (!SetCommState(hComm, &dcb)) {
			// Error in SetCommState. Possibly a problem with the communications 
			// port handle or a problem with the DCB structure itself.
		}
	}
	return true;
}

BOOL ws2812::ClosePort()
{
	if (hComm != INVALID_HANDLE_VALUE) {
		CloseHandle(hComm);
		hComm = INVALID_HANDLE_VALUE;
		return true;
	}
	return false;
}

BOOL ws2812::WriteABuffer(const unsigned char * lpBuf, DWORD dwToWrite)
{
	OVERLAPPED osWrite = { 0 };
	DWORD dwWritten;
	DWORD dwRes;
	BOOL fRes;

	// Port not opened?
	if (hComm == INVALID_HANDLE_VALUE)
		return false;

	// Create this write operation's OVERLAPPED structure's hEvent.
	osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (osWrite.hEvent == NULL) {
		// error creating overlapped event handle
		return FALSE;
	}

	// Issue write.
	if (!WriteFile(hComm, lpBuf, dwToWrite, &dwWritten, &osWrite)) {
		commErr = GetLastError();
		if (commErr != ERROR_IO_PENDING) {
			// WriteFile failed, but isn't delayed. Report error and abort.
			fRes = FALSE;
		}
		else {
			// Write is pending.
			dwRes = WaitForSingleObject(osWrite.hEvent, INFINITE);
			switch (dwRes)
			{
				// OVERLAPPED structure's event has been signaled. 
			case WAIT_OBJECT_0:
				if (!GetOverlappedResult(hComm, &osWrite, &dwWritten, FALSE))
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
	if (ws2812_global)
		ws2812_global->CalcAndOutput();
}

bool ws2812::StartTimer()
{
	if (timerInterval < timerInterval_min)
		timerInterval = timerInterval_min;
	else if (timerInterval > timerInterval_max)
		timerInterval = timerInterval_max;

	if (hTimer == INVALID_HANDLE_VALUE) {
		// timer not created yet
		if (CreateTimerQueueTimer(&hTimer, NULL,
			WaitOrTimerCallback, NULL, timerStartDelay,
			timerInterval, WT_EXECUTELONGFUNCTION))
		{
			// success
			timerStarted = true;
		}
		else {
			// failed
			timerStarted = false;
		}
	}
	else {
		// already created
	}
	return timerStarted;
}

bool ws2812::StopTimer()
{
	timerStarted = false;

	if (hTimer != INVALID_HANDLE_VALUE) {
		DeleteTimerQueueTimer(NULL, hTimer, NULL);
		hTimer = INVALID_HANDLE_VALUE;
		return true;
	}
	return false;
}

void ws2812::OutputTest(const audio_sample *psample, unsigned int samples, audio_sample peak, unsigned char *buffer, unsigned int bufferSize)
{
	unsigned int	i;
	audio_sample	sample;
	unsigned int	bright = (brightness * 255) / 100;

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

unsigned int ws2812::LedIndex(unsigned int row, unsigned int col)
{
	unsigned int	result = 0;

	switch (ledMode)
	{
	case ws2812_led_mode_top_left_common:			// Top Left, Common direction left to right
		result = row * columns + col;
		break;

	case ws2812_led_mode_top_right_common:			// Top Right, Common direction right to left
		result = row * columns + (columns - 1 - col);
		break;

	case ws2812_led_mode_bottom_left_common:		// Bottom Left, Common direction left to right
		result = (rows - 1 - row) * columns + col;
		break;

	case ws2812_led_mode_bottom_right_common:		// Bottom Right, Common direction right to left
		result = (rows - 1 - row) * columns + (columns - 1 - col);
		break;

	case ws2812_led_mode_top_left_alternating:		// Top Left, Alternating direction left to right/right to left
	case ws2812_led_mode_top_right_alternating:		// Top Right, Alternating direction right to left/left to right
	case ws2812_led_mode_bottom_left_alternating:	// Bottom Left, Alternating direction left to right/right to left
	case ws2812_led_mode_bottom_right_alternating:	// Bottom Right, Alternating right to left/direction left to right
		break;
	}

	if (result < rows * columns)
		return result;

	return 0;
}

void ws2812::CalcColorWhiteBar(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b)
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

void ws2812::CalcRowColor(audio_sample row, unsigned int &r, unsigned int &g, unsigned int &b)
{
	audio_sample	row_max = (audio_sample)rows - 1;
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

void ws2812::CalcColorColoredRows(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b)
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

void ws2812::CalcColorColoredBars(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b)
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

void ws2812::CalcColor(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b)
{
	switch (lineStyle)
	{
	default:
	case ws2812_spectrum_simple:		// simple white bars
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

	case ws2812_spectrum_green_red_bars:		// from green to red, each row a single color
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

	case ws2812_spectrum_fire_lines:		// from green to red, each bar the same color (fire style)
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

void ws2812::CalcPersistence(unsigned int &c, unsigned int &p_c)
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

void ws2812::AddPersistence(unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b)
{
	unsigned int	p_r, p_g, p_b;

	p_r = persistenceBuffer[3 * led_index + 0];
	p_g = persistenceBuffer[3 * led_index + 1];
	p_b = persistenceBuffer[3 * led_index + 2];

	CalcPersistence(r, p_r);
	CalcPersistence(g, p_g);
	CalcPersistence(b, p_b);

	persistenceBuffer[3 * led_index + 0] = p_r;
	persistenceBuffer[3 * led_index + 1] = p_g;
	persistenceBuffer[3 * led_index + 2] = p_b;
}

void ws2812::ApplyBrightness(unsigned int brightness, unsigned int &r, unsigned int &g, unsigned int &b)
{
	// apply brightness
	r = (r * brightness) / 255;
	g = (g * brightness) / 255;
	b = (b * brightness) / 255;
}

void ws2812::LedToBuffer(unsigned char *buffer, unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b)
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

void ws2812::OutputSpectrumBars(const audio_sample *psample, unsigned int samples, audio_sample peak, audio_sample delta_f, unsigned char *buffer)
{
	unsigned int	rows = this->rows;
	unsigned int	cols = this->columns;
	unsigned int	bar, cnt, row;
	unsigned int	bar_cnt;
	unsigned int	samples_per_bar;
	unsigned int	sample_index;
	unsigned int	led_index;
	unsigned int	r, g, b;
	unsigned int	bright = (255 * this->brightness) / 100;
	enum line_style	line_style = this->lineStyle;
	audio_sample	sum;
	audio_sample	sample;
	audio_sample	db_per_row;
	audio_sample	db_max;
	audio_sample	db_min;
	double			log_mult;
	double			bar_freq;

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


	// first bar includes samples up to 50 Hz
	bar_freq = 50;

//	log_mult = log10(samples);
//	log_mult /= (double)bar_cnt;
	log_mult = delta_f * (audio_sample)samples;
	log_mult /= bar_freq;
	// nth root of max_freq/min_freq, where n is the count of bars
	log_mult = pow(log_mult, 1.0 / bar_cnt);

	sample_index = 0;

	for (bar = 0; bar < bar_cnt; bar++) {
		if (logFrequency == false) {
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

			if (peakValues == false) {
				// calc mean value of samples per bar
				sum = 0;
				for (cnt = 0; cnt < samples_per_bar; cnt++) {
					sample = psample[sample_index + cnt];
					sum += fabs(sample);
				}

				if (logAmplitude) {
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

				if (logAmplitude) {
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
		//	if (bar == 0) {
		//		// first bar -> everthing up to 50 Hz
		//		samples_per_bar = (unsigned int)(50 / delta_f);
		//		if (samples_per_bar < 1)
		//			samples_per_bar = 1;
		//	}
		//	else
			if (bar == (bar_cnt - 1)) {
				// last bar -> remaining samples
				if (samples > sample_index)
					samples_per_bar = samples - sample_index;
				else
					samples_per_bar = 0;
			}
			else {
				double	tmp;

				// calc start sample index of the following bar
			//	tmp = log_mult * (double)(bar + 1);
			//	tmp = pow(10, tmp);
			//	tmp = 50 * pow(log_mult, bar) / delta_f;
				tmp = ceil(bar_freq / delta_f);

				// count of samples to the next index
				if (tmp <= (double)sample_index)
					samples_per_bar = 0;
				else
					samples_per_bar = (unsigned int)tmp - sample_index;

				// minimum sample count (trial and error, should depend somehow on the max fft frequency...)
				if (samples_per_bar < 1)
					samples_per_bar = 1;

				// prevent index overflow
				if (sample_index >= samples)
					samples_per_bar = 0;
				else if ((sample_index + samples_per_bar) >= samples)
					samples_per_bar = samples - sample_index;
			}

			if (peakValues == false) {
				// calc mean value of samples per bar
				sum = 0;
				for (cnt = 0; cnt < samples_per_bar; cnt++) {
					sample = psample[sample_index + cnt];
					sum += fabs(sample);
				}

				if (logAmplitude) {
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

				if (logAmplitude) {
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
		// increase max frequency for the next bar
		bar_freq *= log_mult;

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
		case ws2812_spectrum_simple:		// simple white bars
			for (row = 1; row <= rows; row++) {
				// led order: start is top left
				led_index = LedIndex(rows - row, bar);

				CalcColorWhiteBar(row, sample, r, g, b);

				AddPersistence(led_index, r, g, b);

				ApplyBrightness(bright, r, g, b);

				LedToBuffer(buffer, led_index, r, g, b);
			}
			break;

		case ws2812_spectrum_green_red_bars:		// from green to red, each row a single color
			for (row = 1; row <= rows; row++) {
				// led order: start is top left
				led_index = LedIndex(rows - row, bar);

				CalcColorColoredRows(row, sample, r, g, b);

				AddPersistence(led_index, r, g, b);

				ApplyBrightness(bright, r, g, b);

				LedToBuffer(buffer, led_index, r, g, b);
			}
			break;

		case ws2812_spectrum_fire_lines:		// fire style: from green to red the whole bar a single color
			for (row = 1; row <= rows; row++) {
				// led order: start is top left
				led_index = LedIndex(rows - row, bar);

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

void ws2812::CalcAndOutput(void)
{
	double	abs_time;

	// get current track time
	if (visStream->get_absolute_time(abs_time)) {
		audio_chunk_fast_impl	chunk;
		unsigned int			fft_size = fftSize;

		// FFT data
		if (visStream->get_spectrum_absolute(chunk, abs_time, fft_size)) {
			// number of channels, should be 1
			unsigned int	channels = chunk.get_channel_count();
			// number of samples in the chunk, fft_size / 2
			int				samples = chunk.get_sample_count();
			unsigned int	sample_rate = chunk.get_sample_rate();
			// peak sample value
			audio_sample	peak = 1.0;	// chunk.get_peak();
			audio_sample	delta_f = (audio_sample)sample_rate / (audio_sample)fft_size;

			// convert samples
			const audio_sample *psample = chunk.get_data();
			if (outputBuffer != nullptr && psample != nullptr && samples > 0) {
				// a value of 1 is used as start byte
				// and must not occur in other bytes in the buffer
				outputBuffer[0] = 1;

				OutputSpectrumBars(psample, samples, peak, delta_f, &outputBuffer[1]);

				// send buffer
				WriteABuffer(outputBuffer, bufferSize);

			}
			else {
				// no samples ?
			}
		}
		else {
			// no FFT data ?
		}
	}
	else {
		// playback hasn't started yet...
	}
}

bool ws2812::StopOutput(void)
{
	bool	isRunning = false;

	if (initDone && timerStarted) {
		isRunning = true;

		StopTimer();
		ClosePort();
	}
	return isRunning;
}

bool StopOutput(void)
{
	if (ws2812_global)
		return ws2812_global->StopOutput();

	return false;
}

bool ws2812::StartOutput(void)
{
	if (initDone && timerStarted == false) {
		if (OpenPort(NULL, comPort)) {
			StartTimer();
		}
	}
	return timerStarted;
}

bool StartOutput(void)
{
	if (ws2812_global)
		return ws2812_global->StartOutput();

	return false;
}

bool ToggleOutput(void)
{
	if (ws2812_global)
		return ws2812_global->ToggleOutput();

	return false;
}

bool ws2812::ToggleOutput(void)
{
	if (initDone) {
		// toggle timer state
		if (timerStarted == false) {
			if (OpenPort(NULL, comPort)) {
				if (StartTimer()) {

				}
				else {
					popup_message::g_show("Start timer failed!", "WS2812 Output", popup_message::icon_error);
				}
			}
			else {
				popup_message::g_show("Open COM port failed!", "WS2812 Output", popup_message::icon_error);
			}
		}
		else {
			StopTimer();
			ClosePort();
		}
	}
	else {
		popup_message::g_show("Not initialised!", "WS2812 Output", popup_message::icon_error);
	}

	return timerStarted;
}

void ConfigMatrix(int rows, int cols, int start_led, int led_dir)
{
	if (ws2812_global)
		ws2812_global->ConfigMatrix(rows, cols, (enum start_led)start_led, (enum led_direction)led_dir);
}

void ws2812::ConfigMatrix(int rows, int cols, enum start_led start_led, enum led_direction led_dir)
{
	bool	timerRunning;

	if (rows < 0)
		rows = this->rows;
	if (cols < 0)
		cols = this->columns;

	if (   (unsigned int)rows >= rows_min && (unsigned int)rows <= rows_max
		&& (unsigned int)cols >= columns_min && (unsigned int)cols <= columns_max)
	{
		if (   this->rows != (unsigned int)rows
			|| this->columns != (unsigned int)cols)
		{
			initDone = false;

			this->rows = (unsigned int)rows;
			this->columns = (unsigned int)cols;

			// kill the timer
			timerRunning = StopTimer();

			if (outputBuffer)
				delete outputBuffer;
			outputBuffer = nullptr;

			if (persistenceBuffer)
				delete persistenceBuffer;
			persistenceBuffer = nullptr;

			// 240 LEDs, RGB for each, plus one start byte
			initDone = CreateBuffer();
			if (initDone) {
				if (timerRunning) {
					StartTimer();
				}
			}
		}
	}
}

void SetScaling(int logFrequency, int logAmplitude, int peakValues)
{
	if (ws2812_global)
		ws2812_global->SetScaling(logFrequency, logAmplitude, peakValues);
}

void ws2812::SetScaling(int logFrequency, int logAmplitude, int peakValues)
{
	if (logFrequency == 0)
		this->logFrequency = false;
	else if (logFrequency > 0)
		this->logFrequency = true;

	if (logAmplitude == 0)
		this->logAmplitude = false;
	else if (logAmplitude > 0)
		this->logAmplitude = true;

	if (peakValues == 0)
		this->peakValues = false;
	else if (peakValues > 0)
		this->peakValues = true;
}

void SetLineStyle(unsigned int lineStyle)
{
	if (ws2812_global)
		ws2812_global->SetLineStyle((enum line_style)lineStyle);
}

void ws2812::SetLineStyle(enum line_style style)
{
	if (style >= 0 && style < ws2812_line_style_no)
		this->lineStyle = style;
}

void SetBrightness(unsigned int brightness)
{
	if (ws2812_global)
		ws2812_global->SetBrightness(brightness);
}

void ws2812::SetBrightness(unsigned int brightness)
{
	bool	timerRunning;

	if (brightness >= brightness_min && brightness <= brightness_max) {
		if (this->brightness != brightness) {
			timerRunning = StopTimer();

			this->brightness = brightness;

			if (timerRunning) {
				StartTimer();
			}
		}
	}
}

void SetComPort(unsigned int port)
{
	if (ws2812_global)
		ws2812_global->SetComPort(port);
}

bool ws2812::SetComPort(unsigned int port)
{
	bool	result = false;
	bool	timerRunning;

	if (port >= port_min && port <= port_max) {
		if (port != comPort) {
			comPort = port;

			timerRunning = StopTimer();

			ClosePort();

			if (timerRunning) {
				if (OpenPort(NULL, comPort)) {
					StartTimer();
					result = true;
				}
			}
		}
	}
	return result;
}

void SetInterval(unsigned int interval)
{
	if (ws2812_global)
		ws2812_global->SetInterval(interval);
}

void ws2812::SetInterval(unsigned int interval)
{
	if (interval >= timerInterval_min && interval <= timerInterval_max) {
		if (interval != timerInterval) {
			timerInterval = interval;
			if (hTimer != INVALID_HANDLE_VALUE) {
				ChangeTimerQueueTimer(NULL, hTimer, timerStartDelay, timerInterval);
			}
		}
	}
}

bool ws2812::CreateBuffer()
{
	bufferSize = 1 + 3 * rows * columns;
	outputBuffer = new unsigned char[bufferSize];
	persistenceBuffer = new unsigned char[bufferSize];

	if (outputBuffer && persistenceBuffer) {
		ZeroMemory(outputBuffer, bufferSize);
		ZeroMemory(persistenceBuffer, bufferSize);
		return true;
	}
	return false;
}



ws2812::ws2812()
{
	hComm = INVALID_HANDLE_VALUE;
	hTimer = INVALID_HANDLE_VALUE;
	bufferSize = 0;
	outputBuffer = nullptr;
	persistenceBuffer = nullptr;

	// ask the global visualisation manager to create a stream for us
	static_api_ptr_t<visualisation_manager>()->create_stream(visStream, visualisation_manager::KStreamFlagNewFFT);

	// I probably should test this
	if (visStream != nullptr) {
		// mono is preferred, unless you want to use two displays ;-)
		visStream->set_channel_mode(visualisation_stream_v2::channel_mode_mono);

		// read configuration values
		comPort = GetCfgComPort();
		if (comPort < port_min || comPort > port_max)
			comPort = port_def;

		rows = GetCfgMatrixRows();
		if (rows < rows_min || rows > rows_max)
			rows = rows_def;

		columns = GetCfgMatrixCols();
		if (columns < columns_min || columns > columns_max)
			columns = columns_def;

		timerInterval = GetCfgUpdateInterval();
		if (timerInterval < 50 || timerInterval > 1000)
			timerInterval = 250;

		brightness = GetCfgBrightness();
		if (brightness < 1 || brightness > 100)
			brightness = 25;

		unsigned int tmp = GetCfgLineStyle();
		if (tmp > ws2812_line_style_no)
			lineStyle = ws2812_spectrum_simple;
		else
			lineStyle = (enum line_style)tmp;

		logFrequency = GetCfgLogFrequency() != 0;
		logAmplitude = GetCfgLogAmplitude() != 0;
		peakValues = GetCfgPeakValues() != 0;

		// 240 LEDs, RGB for each, plus one start byte
		initDone = CreateBuffer();
	}
}

ws2812::ws2812(unsigned int rows, unsigned int cols, unsigned int port, unsigned int interval, enum line_style style)
{
	hComm = INVALID_HANDLE_VALUE;
	hTimer = INVALID_HANDLE_VALUE;
	bufferSize = 0;
	outputBuffer = nullptr;
	persistenceBuffer = nullptr;

	// ask the global visualisation manager to create a stream for us
	static_api_ptr_t<visualisation_manager>()->create_stream(visStream, visualisation_manager::KStreamFlagNewFFT);

	// I probably should test this
	if (visStream != nullptr) {
		// mono is preferred, unless you want to use two displays ;-)
		visStream->set_channel_mode(visualisation_stream_v2::channel_mode_mono);

		// read configuration values
		if (port >= 1 && port < 127)
			this->comPort = port;
		else
			this->comPort = 1;

		if (rows >= rows_min && rows <= rows_max)
			this->rows = rows;
		else
			this->rows = rows_def;

		if (cols >= columns_min && cols <= columns_max)
			this->columns = cols;
		else
			this->columns = columns_def;

		if (interval >= timerInterval_min && interval <= timerInterval_max)
			this->timerInterval = interval;
		else
			this->timerInterval = timerInterval_def;

		this->brightness = brightness_def;

		if (style >= 0 && style < ws2812_line_style_no)
			this->lineStyle = style;
		else
			this->lineStyle = ws2812_spectrum_simple;

		logFrequency = true;
		logAmplitude = true;
		peakValues = true;

		// 240 LEDs, RGB for each, plus one start byte
		initDone = CreateBuffer();
	}
}

ws2812::~ws2812()
{
	initDone = false;

	// how to delete the stream?
//	delete visStream;

	// kill the timer
	StopTimer();

	// close the COM port
	ClosePort();

	if (outputBuffer)
		delete outputBuffer;
	outputBuffer = nullptr;

	if (persistenceBuffer)
		delete persistenceBuffer;
	persistenceBuffer = nullptr;
}

void InitOutput()
{
	if (ws2812_global == nullptr)
		ws2812_global = new ws2812();

	if (ws2812_global) {

	}
}

void DeinitOutput()
{
	if (ws2812_global) {
		delete ws2812_global;
		ws2812_global = nullptr;
	}
}
