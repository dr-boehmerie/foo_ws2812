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
		dcb.fOutxCtsFlow = FALSE;
		dcb.fOutxDsrFlow = FALSE;
		// must set reception control lines to high state (arduino won't receive data otherwise)
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
		dcb.fDtrControl = DTR_CONTROL_ENABLE;

		// Set new state.
		if (!SetCommState(hComm, &dcb)) {
			// Error in SetCommState. Possibly a problem with the communications 
			// port handle or a problem with the DCB structure itself.
			commErr = GetLastError();

			CloseHandle(hComm);
			hComm = INVALID_HANDLE_VALUE;
			return false;
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
		return FALSE;

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
		//	dwRes = WaitForSingleObject(osWrite.hEvent, INFINITE);
			dwRes = WaitForSingleObject(osWrite.hEvent, 1000);
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
			case WAIT_TIMEOUT:
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

enum led_mode ws2812::GetLedMode(unsigned int startLed, unsigned int ledDir)
{
	enum led_mode	result = ws2812_led_mode_top_left_common;

	if (ledDir == 0) {
		// common direction
		switch (startLed)
		{
		default:
		case 0:
			result = ws2812_led_mode_top_left_common;
			break;
		case 1:
			result = ws2812_led_mode_top_right_common;
			break;
		case 2:
			result = ws2812_led_mode_bottom_left_common;
			break;
		case 3:
			result = ws2812_led_mode_bottom_right_common;
			break;
		}
	}
	else {
		// alternating direction
		switch (startLed)
		{
		default:
		case 0:
			result = ws2812_led_mode_top_left_alternating;
			break;
		case 1:
			result = ws2812_led_mode_top_right_alternating;
			break;
		case 2:
			result = ws2812_led_mode_bottom_left_alternating;
			break;
		case 3:
			result = ws2812_led_mode_bottom_right_alternating;
			break;
		}
	}
	return result;
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
		if ((row & 1) == 0) {
			// even row: left to right
		}
		else {
			// odd row: right to left
		}
		break;

	case ws2812_led_mode_top_right_alternating:		// Top Right, Alternating direction right to left/left to right
		break;

	case ws2812_led_mode_bottom_left_alternating:	// Bottom Left, Alternating direction left to right/right to left
		break;

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

void ws2812::CalcColor(audio_sample sample, audio_sample min, unsigned int &r, unsigned int &g, unsigned int &b)
{
	audio_sample	r_start, g_start, b_start;
	audio_sample	r_end, g_end, b_end;

	// green
	r_start = 0; g_start = 200; b_start = 0;
	// to red
	r_end = 255; g_end = 0; b_end = 0;

	// row = 0 to (rows - 1) => black to green to red via yellow
	if (sample <= 0.0) {
		r_end = 0;
		g_end = 0;
		b_end = 0;
	}
	else if (sample <= min) {
		r_end = r_start * (sample / min);
		g_end = g_start * (sample / min);
		b_end = b_start * (sample / min);
	}
	else if (sample < 1.0) {
		// scale min .. sample to 0.0 .. 1.0
		sample = (sample - min) / ((audio_sample)1.0 - min);
		r_end = sample * (r_end - r_start) + r_start;
		g_end = sample * (g_end - g_start) + g_start;
		b_end = sample * (b_end - b_start) + b_start;
	}
	r = (unsigned int)r_end;
	g = (unsigned int)g_end;
	b = (unsigned int)b_end;
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

void ws2812::CalcPersistenceMax(unsigned int &c, unsigned int &p_c)
{
	if (c >= p_c) {
		p_c = c;
	}
	else {
		if (c < p_c)
			c = p_c;
		p_c /= 2;
	}
}

void ws2812::CalcPersistenceAdd(unsigned int &c, unsigned int &p_c)
{
	if (c >= p_c) {
		p_c = c;
	}
	else {
		c += p_c;
		if (c > 255)
			c = 255;
		p_c /= 2;
	}
}

void ws2812::AddPersistenceSpectrum(unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b)
{
	unsigned int	p_r, p_g, p_b;

	p_r = persistenceBuffer[3 * led_index + 0];
	p_g = persistenceBuffer[3 * led_index + 1];
	p_b = persistenceBuffer[3 * led_index + 2];

	CalcPersistenceMax(r, p_r);
	CalcPersistenceMax(g, p_g);
	CalcPersistenceMax(b, p_b);

	persistenceBuffer[3 * led_index + 0] = p_r;
	persistenceBuffer[3 * led_index + 1] = p_g;
	persistenceBuffer[3 * led_index + 2] = p_b;
}

void ws2812::AddPersistenceOscilloscope(unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b)
{
	unsigned int	p_r, p_g, p_b;

	p_r = persistenceBuffer[3 * led_index + 0];
	p_g = persistenceBuffer[3 * led_index + 1];
	p_b = persistenceBuffer[3 * led_index + 2];

	CalcPersistenceAdd(r, p_r);
	CalcPersistenceAdd(g, p_g);
	CalcPersistenceAdd(b, p_b);

	persistenceBuffer[3 * led_index + 0] = p_r;
	persistenceBuffer[3 * led_index + 1] = p_g;
	persistenceBuffer[3 * led_index + 2] = p_b;
}


void ws2812::ApplyBrightness(unsigned int brightness, unsigned int &r, unsigned int &g, unsigned int &b)
{
	// brightness 0...100
	r = (r * brightness) / 100;
	g = (g * brightness) / 100;
	b = (b * brightness) / 100;
}

void ws2812::ColorsToBuffer(unsigned char *buffer, unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b)
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
	unsigned int	bright =this->brightness;
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
				// invert Y axis: y = rows - row
				led_index = LedIndex(rows - row, bar);

				CalcColorWhiteBar(row, sample, r, g, b);

				AddPersistenceSpectrum(led_index, r, g, b);

				ApplyBrightness(bright, r, g, b);

				ColorsToBuffer(buffer, led_index, r, g, b);
			}
			break;

		case ws2812_spectrum_green_red_bars:		// from green to red, each row a single color
			for (row = 1; row <= rows; row++) {
				// invert Y axis: y = rows - row
				led_index = LedIndex(rows - row, bar);

				CalcColorColoredRows(row, sample, r, g, b);

				AddPersistenceSpectrum(led_index, r, g, b);

				ApplyBrightness(bright, r, g, b);

				ColorsToBuffer(buffer, led_index, r, g, b);
			}
			break;

		case ws2812_spectrum_fire_lines:		// fire style: from green to red the whole bar a single color
			for (row = 1; row <= rows; row++) {
				// invert Y axis: y = rows - row
				led_index = LedIndex(rows - row, bar);

				CalcColorColoredBars(row, sample, r, g, b);

				AddPersistenceSpectrum(led_index, r, g, b);

				ApplyBrightness(bright, r, g, b);

				ColorsToBuffer(buffer, led_index, r, g, b);
			}
			break;
		}
	}
}

void ws2812::OutputSpectrogram(const audio_sample *psample, unsigned int samples, audio_sample peak, audio_sample delta_f, unsigned char *buffer)
{
	unsigned int	rows = this->rows;
	unsigned int	cols = this->columns;
	unsigned int	bar, cnt, row, col;
	unsigned int	bar_cnt;
	unsigned int	samples_per_bar;
	unsigned int	sample_index;
	unsigned int	led_index;
	unsigned int	r, g, b, i;
	unsigned int	bright = this->brightness;
	audio_sample	sum;
	audio_sample	sample;
	audio_sample	m, n;
//	audio_sample	min;
	audio_sample	db_max;
	audio_sample	db_min;
	double			log_mult;
	double			bar_freq;

	if (lineStyle == ws2812_spectrogram_horizontal) {
		// move current image one col to the left
		for (col = 0; col < cols - 1; col++) {
			for (row = 0; row < rows; row++) {
				// new col
				led_index = LedIndex(row, col);
				// old col
				sample_index = LedIndex(row, col + 1);

				// copy colors
				buffer[3 * led_index + 0] = buffer[3 * sample_index + 0];
				buffer[3 * led_index + 1] = buffer[3 * sample_index + 1];
				buffer[3 * led_index + 2] = buffer[3 * sample_index + 2];
			}
		}
		// new values added in the last col (right)
		col = cols - 1;

		// horizontal bars
		bar_cnt = rows;
	}
	else {
		// move current image one row up
		for (row = 0; row < rows - 1; row++) {
			for (col = 0; col < cols; col++) {
				// new col
				led_index = LedIndex(row, col);
				// old col
				sample_index = LedIndex(row + 1, col);

				// copy colors
				buffer[3 * led_index + 0] = buffer[3 * sample_index + 0];
				buffer[3 * led_index + 1] = buffer[3 * sample_index + 1];
				buffer[3 * led_index + 2] = buffer[3 * sample_index + 2];
			}
		}
		// new values added in the last row (bottom)
		row = rows - 1;

		// vertical bars
		bar_cnt = cols;
	}

	// limits
	db_max = -5;
	db_min = -40;

#if 0
	m = (audio_sample)1.0 / (db_max - db_min);
	n = (audio_sample)1.0 - db_max * m;
	min = (audio_sample)0.5;
#else
	// use colorTab
	m = (audio_sample)colorNo / (db_max - db_min);
	n = (audio_sample)colorNo - db_max * m;
#endif

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

#if 0
		// calc height of bar in rows
		// -10 db is max, -80 is min
		// -10 db is 1.0, -80 is 0
		if (sample >= db_max) {
			// limit
			sample = 1.0;
		}
		else {
			sample = sample * m + n;
		}
		CalcColor(sample, min, r, g, b);
#else
		// use colorTab
		sample = sample * m + n;
		if (sample < 0) {
			i = 0;
		}
		else if (sample < (audio_sample)colorNo) {
			i = (unsigned int)floorf(sample + 0.5f);
		}
		else {
			i = colorNo - 1;
		}
		r = GET_COLOR_R(colorTab[i]);
		g = GET_COLOR_G(colorTab[i]);
		b = GET_COLOR_B(colorTab[i]);
#endif
		if (lineStyle == ws2812_spectrogram_horizontal) {
			// spectrum in last col, invert Y axis
			row = bar_cnt - 1 - bar;
			led_index = LedIndex(row, col);
		}
		else {
			// spectrum in last row
			col = bar;
			led_index = LedIndex(row, col);
		}

		ApplyBrightness(bright, r, g, b);

		ColorsToBuffer(buffer, led_index, r, g, b);
	}
}

void ws2812::OutputOscilloscope(const audio_sample *psample, unsigned int samples, audio_sample peak, unsigned char *buffer)
{
	unsigned int	rows = this->rows;
	unsigned int	cols = this->columns;
	unsigned int	col, row;
	unsigned int	sample_index;
	unsigned int	led_index;
	unsigned int	r, g, b, i;
	unsigned int	bright = this->brightness;
	unsigned int	max_cnt;
	audio_sample	sample;
	audio_sample	val_per_row;
	audio_sample	val_max;
	audio_sample	val_min;
	unsigned int	max_wfm;

	// scale waveform
	val_max = 1.0;
	val_min = -1.0;
	val_per_row = (val_max - val_min) / (audio_sample)rows;

	// clear counters
	ZeroMemory(counterBuffer, bufferSize * sizeof(unsigned int));

	max_wfm = 4;
	row = 0;
	col = 0;
	max_cnt = 1;
	for (sample_index = 0; sample_index < samples; sample_index++) {
		sample = psample[sample_index];

		// sample to row
		sample = sample / val_per_row;
		// center in Y
		sample += rows / 2;

		// invert Y axis
		sample += 1;
		if (sample > 0)
			row = rows - (unsigned int)floor(sample);
		else
			row = rows / 2;

		led_index = LedIndex(row, col);

		// sum of all hits of this pixel
		counterBuffer[led_index] += 1;

		// update max count for brightness calculation
		if (max_cnt < counterBuffer[led_index])
			max_cnt = counterBuffer[led_index];

		// one samples per row, multiple chunks overlapped
		col++;
		if (col >= cols) {
			col = 0;

			// limit max overlapped waveforms
			if (max_wfm > 0)
				max_wfm--;
			else
				break;
		}
	}

	// convert sample counts to brightness
	for (led_index = 0; led_index < rows * cols; led_index++) {
		// shades of white, for a start
	//	r = (counterBuffer[led_index] * 255) / max_cnt;
	//	g = b = r;
		// colorTab entry
		i = (counterBuffer[led_index] * (colorNo - 1)) / max_cnt;
		r = GET_COLOR_R(colorTab[i]);
		g = GET_COLOR_G(colorTab[i]);
		b = GET_COLOR_B(colorTab[i]);

		AddPersistenceOscilloscope(led_index, r, g, b);

		ApplyBrightness(bright, r, g, b);

		ColorsToBuffer(buffer, led_index, r, g, b);
	}
}

void ws2812::CalcAndOutput(void)
{
	double	abs_time;

	// get current track time
	if (visStream->get_absolute_time(abs_time)) {
		audio_chunk_fast_impl	chunk;

		if (lineStyle == ws2812_oscilloscope) {
			// length of audio data
			double		length = (double)timerInterval * 1e-3;	//audioLength;

			// audio data
			if (visStream->get_chunk_absolute(chunk, abs_time, length)) {
				// number of channels, should be 1
				unsigned int	channels = chunk.get_channel_count();
				// number of samples in the chunk, fft_size / 2
				int				samples = chunk.get_sample_count();
				unsigned int	sample_rate = chunk.get_sample_rate();
				// peak sample value
				audio_sample	peak = 1.0;	//chunk.get_peak();

				// convert samples
				const audio_sample *psample = chunk.get_data();
				if (outputBuffer != nullptr && psample != nullptr && samples > 0) {
					// a value of 1 is used as start byte
					// and must not occur in other bytes in the buffer
					outputBuffer[0] = 1;

					OutputOscilloscope(psample, samples, peak, &outputBuffer[1]);

					// send buffer
					WriteABuffer(outputBuffer, bufferSize);

				}
				else {
					// no samples ?
				}
			}
			else {
				// no audio data ?
			}
		}
		else if (lineStyle == ws2812_spectrogram_horizontal
			|| lineStyle == ws2812_spectrogram_vertical) {
			unsigned int			fft_size = fftSize;

			// spectrum data
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

					OutputSpectrogram(psample, samples, peak, delta_f, &outputBuffer[1]);

					// send buffer
					WriteABuffer(outputBuffer, bufferSize);
				}
				else {
					// no samples ?
				}
			}
			else {
				// no spectrum data ?
			}
		}
		else {
			unsigned int			fft_size = fftSize;

			// spectrum data
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
				// no spectrum data ?
			}
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
					// okay
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
			initDone = AllocateBuffers();
			if (initDone) {
				if (timerRunning) {
					StartTimer();
				}
			}
		}
	}
}

void ws2812::InitColorTab(void)
{
	unsigned int	i;
	unsigned int	r, g, b;
	float			r_start, g_start, b_start;
	float			r_end, g_end, b_end;
	float			r_step, g_step, b_step;

	if (colorTab) {
		// create default color tab: green to red
		r_start = 0;
		g_start = 255;
		b_start = 0;

		r_end = 255;
		g_end = 0;
		b_end = 0;

		// dividing by colorNo - 1 ensures that the last entry is filled with the end color
		r_step = (r_end - r_start) / (colorNo - 1);
		g_step = (g_end - g_start) / (colorNo - 1);
		b_step = (b_end - b_start) / (colorNo - 1);

		for (i = 0; i < colorNo; i++) {
			r = (unsigned int)floorf(r_start);
			g = (unsigned int)floorf(g_start);
			b = (unsigned int)floorf(b_start);
			// ARGB
			colorTab[i] = MAKE_COLOR(r, g, b);

			r_start += r_step;
			g_start += g_step;
			b_start += b_step;
		}
	}
}

void ws2812::InitColorTab(const unsigned int *initTab, unsigned int tabElements)
{
	unsigned int	i, j, i_end;
	unsigned int	r, g, b;
	float			colors_per_segment;
	float			r_start, g_start, b_start;
	float			r_end, g_end, b_end;
	float			r_step, g_step, b_step;

	if (colorTab && initTab && tabElements > 0) {
		if (tabElements > 1) {
			// divide colorTab into segments
			colors_per_segment = (float)colorNo / (float)(tabElements - 1);

			i = 0;
			j = 0;
			i_end = 0;

			for (i = 0; i < colorNo; i++) {
				if (i == i_end && j < (tabElements - 1)) {
					// next initTab entry
					r_start = (float)GET_COLOR_R(initTab[j]);
					g_start = (float)GET_COLOR_G(initTab[j]);
					b_start = (float)GET_COLOR_B(initTab[j]);

					r_end = (float)GET_COLOR_R(initTab[j + 1]);
					g_end = (float)GET_COLOR_G(initTab[j + 1]);
					b_end = (float)GET_COLOR_B(initTab[j + 1]);

					// last colorTab index of this segment
					j++;
					i_end = (unsigned int)floor((float)j * colors_per_segment);

					r_step = (r_end - r_start);
					g_step = (g_end - g_start);
					b_step = (b_end - b_start);

					if (j < (tabElements - 1)) {
						// dividing by count because the first entry of the next segment will hold the end color
						r_step /= (float)(i_end - i);
						g_step /= (float)(i_end - i);
						b_step /= (float)(i_end - i);
					}
					else {
						// dividing by count - 1 ensures that the last entry is filled with the end color
						r_step /= (float)((i_end - i) - 1);
						g_step /= (float)((i_end - i) - 1);
						b_step /= (float)((i_end - i) - 1);
					}
				}

				r = (unsigned int)floorf(r_start + 0.5f);
				g = (unsigned int)floorf(g_start + 0.5f);
				b = (unsigned int)floorf(b_start + 0.5f);
				// ARGB
				colorTab[i] = MAKE_COLOR(r, g, b);

				if (i < i_end) {
					r_start += r_step;
					g_start += g_step;
					b_start += b_step;
				}
			}
		}
		else {
			// fill colorTab
			for (i = 0; i < colorNo; i++)
				colorTab[i] = initTab[0];
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
	if (style >= 0 && style < ws2812_line_style_no) {
		this->lineStyle = style;

		switch (this->lineStyle)
		{
		default:
			ClearOutputBuffer();
			ClearPersistence();

			// init default spectrum colors
			InitColorTab(spectrumColorTab, CALC_TAB_ELEMENTS(spectrumColorTab));
			break;

		case ws2812_spectrogram_horizontal:
		case ws2812_spectrogram_vertical:
			ClearOutputBuffer();

			// init default spectrogram colors
			InitColorTab(spectrogramColorTab, CALC_TAB_ELEMENTS(spectrogramColorTab));
			break;

		case ws2812_oscilloscope:
			ClearPersistence();

			// init default oscilloscope colors
			InitColorTab(oscilloscopeColorTab, CALC_TAB_ELEMENTS(oscilloscopeColorTab));
			break;
		}
	}
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

bool ws2812::AllocateBuffers()
{
	ledNo = rows * columns;

	// start byte + GRB for each LED
	bufferSize = 1 + 3 * ledNo;

	outputBuffer		= new unsigned char[bufferSize];
	persistenceBuffer	= new unsigned char[bufferSize];
	counterBuffer		= new unsigned int[bufferSize];
	indexLut			= new unsigned int[ledNo];
	colorTab			= new unsigned int[colorNo];

	if (   outputBuffer
		&& persistenceBuffer
		&& counterBuffer
		&& indexLut
		&& colorTab)
	{
		ZeroMemory(outputBuffer,		bufferSize * sizeof(unsigned char));
		ZeroMemory(persistenceBuffer,	bufferSize * sizeof(unsigned char));
		ZeroMemory(counterBuffer,		bufferSize * sizeof(unsigned int));
		ZeroMemory(indexLut,			ledNo * sizeof(unsigned int));
		ZeroMemory(colorTab,			colorNo * sizeof(unsigned int));

		return true;
	}
	return false;
}

void ws2812::FreeBuffers()
{
	if (outputBuffer)
		delete outputBuffer;
	outputBuffer = nullptr;

	if (persistenceBuffer)
		delete persistenceBuffer;
	persistenceBuffer = nullptr;

	if (counterBuffer)
		delete counterBuffer;
	counterBuffer = nullptr;

	if (indexLut)
		delete indexLut;
	indexLut = nullptr;

	if (colorTab)
		delete colorTab;
	colorTab = nullptr;
}


void ws2812::ClearPersistence(void)
{
	if (persistenceBuffer)
		ZeroMemory(persistenceBuffer, bufferSize * sizeof(unsigned char));
}

void ws2812::ClearOutputBuffer(void)
{
	if (outputBuffer)
		ZeroMemory(outputBuffer, bufferSize * sizeof(unsigned char));
}



ws2812::ws2812()
{
	hComm = INVALID_HANDLE_VALUE;
	hTimer = INVALID_HANDLE_VALUE;

	ledNo = 0;
	bufferSize = 0;
	outputBuffer = nullptr;
	persistenceBuffer = nullptr;
	counterBuffer = nullptr;
	indexLut = nullptr;
	colorTab = nullptr;

	timerStarted = false;
	initDone = false;

	// ask the global visualisation manager to create a stream for us
	static_api_ptr_t<visualisation_manager>()->create_stream(visStream, visualisation_manager::KStreamFlagNewFFT);

	// I probably should test this
	if (visStream != nullptr) {
		// mono is preferred, unless you want to use two displays ;-)
		visStream->set_channel_mode(visualisation_stream_v2::channel_mode_mono);

		// read configuration values
		rows = GetCfgMatrixRows();
		if (rows < rows_min || rows > rows_max)
			rows = rows_def;

		columns = GetCfgMatrixCols();
		if (columns < columns_min || columns > columns_max)
			columns = columns_def;

		// allocate output buffer
		initDone = AllocateBuffers();

		comPort = GetCfgComPort();
		if (comPort < port_min || comPort > port_max)
			comPort = port_def;

		timerInterval = GetCfgUpdateInterval();
		if (timerInterval < 50 || timerInterval > 1000)
			timerInterval = 250;

		brightness = GetCfgBrightness();
		if (brightness < 1 || brightness > 100)
			brightness = 25;

		if (initDone) {
			unsigned int tmp = GetCfgLineStyle();
			if (tmp > ws2812_line_style_no)
				SetLineStyle(ws2812_spectrum_simple);
			else
				SetLineStyle((enum line_style)tmp);

			logFrequency = GetCfgLogFrequency() != 0;
			logAmplitude = GetCfgLogAmplitude() != 0;
			peakValues = GetCfgPeakValues() != 0;

			ledMode = GetLedMode(GetCfgStartLed(), GetCfgLedDirection());

			timerStartDelay = 500;

			// fast FFT
			fftSize = 8 * 1024;

			// oscilloscope display time
			audioLength = 10 * 1e-3;

			// init default color tab
		//	InitColorTab();
		}
	}
}

ws2812::ws2812(unsigned int rows, unsigned int cols, unsigned int port, unsigned int interval, enum line_style style)
{
	hComm = INVALID_HANDLE_VALUE;
	hTimer = INVALID_HANDLE_VALUE;

	ledNo = 0;
	bufferSize = 0;
	outputBuffer = nullptr;
	persistenceBuffer = nullptr;
	counterBuffer = nullptr;
	indexLut = nullptr;
	colorTab = nullptr;

	timerStarted = false;
	initDone = false;

	// ask the global visualisation manager to create a stream for us
	static_api_ptr_t<visualisation_manager>()->create_stream(visStream, visualisation_manager::KStreamFlagNewFFT);

	// I probably should test this
	if (visStream != nullptr) {
		// mono is preferred, unless you want to use two displays ;-)
		visStream->set_channel_mode(visualisation_stream_v2::channel_mode_mono);

		// read configuration values
		if (rows >= rows_min && rows <= rows_max)
			this->rows = rows;
		else
			this->rows = rows_def;

		if (cols >= columns_min && cols <= columns_max)
			this->columns = cols;
		else
			this->columns = columns_def;

		// allocate output buffer
		initDone = AllocateBuffers();

		if (port >= 1 && port < 127)
			this->comPort = port;
		else
			this->comPort = 1;

		if (interval >= timerInterval_min && interval <= timerInterval_max)
			this->timerInterval = interval;
		else
			this->timerInterval = timerInterval_def;

		this->brightness = brightness_def;

		if (initDone) {
			if (style >= 0 && style < ws2812_line_style_no)
				SetLineStyle(style);
			else
				SetLineStyle(ws2812_spectrum_simple);

			logFrequency = true;
			logAmplitude = true;
			peakValues = true;

			ledMode = GetLedMode(0, 0);

			timerStartDelay = 500;

			// fast FFT
			fftSize = 8 * 1024;

			// oscilloscope display time
			audioLength = 10 * 1e-3;

			// init default color tab
		//	InitColorTab();
		}
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

	FreeBuffers();
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
