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
		// 8 N 1
		dcb.ByteSize = 8;
		dcb.Parity = NOPARITY;
		dcb.StopBits = ONESTOPBIT;
		// Update DCB rate.
		switch (comBaudrate)
		{
		case ws2812_baudrate_9600:		dcb.BaudRate = CBR_9600;	break;
		case ws2812_baudrate_14400:		dcb.BaudRate = CBR_14400;	break;
		case ws2812_baudrate_19200:		dcb.BaudRate = CBR_19200;	break;
		case ws2812_baudrate_38400:		dcb.BaudRate = CBR_38400;	break;
		case ws2812_baudrate_56000:		dcb.BaudRate = CBR_56000;	break;
		case ws2812_baudrate_57600:		dcb.BaudRate = CBR_57600;	break;
		case ws2812_baudrate_115200:	dcb.BaudRate = CBR_115200;	break;
		case ws2812_baudrate_128000:	dcb.BaudRate = CBR_128000;	break;
		case ws2812_baudrate_250000:	dcb.BaudRate = 250000;		break;
		case ws2812_baudrate_256000:	dcb.BaudRate = CBR_256000;	break;
		case ws2812_baudrate_500000:	dcb.BaudRate = 500000;		break;
		default:						dcb.BaudRate = CBR_115200;	break;
		}
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

		// wait if the timer is currently active
		while (timerActive) {

		}
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

void ws2812::InitIndexLut(void)
{
	unsigned int	rows = this->rows;
	unsigned int	cols = this->columns;
	unsigned int	row, col;
	unsigned int	idx;

	assert(indexLut != nullptr);
	if (indexLut == nullptr || rows <= 0 || cols <= 0)
		return;

	switch (ledMode)
	{
	default:
	case ws2812_led_mode_top_left_common:			// Top Left, Common direction left to right
		// 1:1
		for (idx = 0; idx < (rows * cols); idx++) {
			indexLut[idx] = idx;
		}
		break;

	case ws2812_led_mode_top_right_common:			// Top Right, Common direction right to left
		idx = 0;
		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				indexLut[idx] = row * cols + (cols - 1 - col);
				idx++;
			}
		}
		break;

	case ws2812_led_mode_bottom_left_common:		// Bottom Left, Common direction left to right
		idx = 0;
		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				indexLut[idx] = (rows - 1 - row) * cols + col;
				idx++;
			}
		}
		break;

	case ws2812_led_mode_bottom_right_common:		// Bottom Right, Common direction right to left
		idx = 0;
		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				indexLut[idx] = (rows - 1 - row) * cols + (cols - 1 - col);
				idx++;
			}
		}
		break;

	case ws2812_led_mode_top_left_alternating:		// Top Left, Alternating direction left to right/right to left
		idx = 0;
		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				if ((row & 1) == 0) {
					// even row: left to right
					indexLut[idx] = row * cols + col;
				}
				else {
					// odd row: right to left
					indexLut[idx] = row * cols + (cols - 1 - col);
				}
				idx++;
			}
		}
		break;

	case ws2812_led_mode_top_right_alternating:		// Top Right, Alternating direction right to left/left to right
		idx = 0;
		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				if ((row & 1) != 0) {
					// odd row: left to right
					indexLut[idx] = row * cols + col;
				}
				else {
					// even row: right to left
					indexLut[idx] = row * cols + (cols - 1 - col);
				}
				idx++;
			}
		}
		break;

	case ws2812_led_mode_bottom_left_alternating:	// Bottom Left, Alternating direction left to right/right to left
		idx = 0;
		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				if ((row & 1) == (rows & 1)) {
					// last row, second next to last and so on: left to right
					indexLut[idx] = (rows - 1 - row) * cols + col;
				}
				else {
					// other row: right to left
					indexLut[idx] = (rows - 1 - row) * cols + (cols - 1 - col);
				}
				idx++;
			}
		}
		break;

	case ws2812_led_mode_bottom_right_alternating:	// Bottom Right, Alternating right to left/direction left to right
		idx = 0;
		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				if ((row & 1) == (rows & 1)) {
					// last row, second next to last and so on: right to left
					indexLut[idx] = (rows - 1 - row) * cols + (cols - 1 - col);
				}
				else {
					// other row: left to right
					indexLut[idx] = (rows - 1 - row) * cols + col;
				}
				idx++;
			}
		}
		break;
	}
}

unsigned int ws2812::LedIndex(unsigned int row, unsigned int col)
{
	// use prebuilt lut
	assert(indexLut != nullptr);

	return indexLut[row * columns + col];
}

void ws2812::CalcColorSimple(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b)
{
	if ((audio_sample)row > sample) {
		// off
		r = g = b = 0;
	}
	else if (row == (unsigned int)floor(sample)) {
		// peak
		// use max color table entry
		GetColor(colorNo - 1, r, g, b);
	}
	else {
		// between
		// use max color table entry
		GetColor(colorNo - 1, r, g, b);
		// reduce brightness
		r /= 4;
		g /= 4;
		b /= 4;
	}
}

void ws2812::CalcRowColor(audio_sample row, unsigned int &r, unsigned int &g, unsigned int &b)
{
	// use color table
	unsigned int	tabIdx;

	if (row <= 0) {
		// off
		r = g = b = 0;
	}
	else if (row >= (audio_sample)rows) {
		// peak
		GetColor(colorNo - 1, r, g, b);
	}
	else {
		// between
		tabIdx = (unsigned int)floor(row * this->colorsPerRow);
		GetColor(tabIdx, r, g, b);
	}
}

void ws2812::CalcColorColoredRows(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b)
{
	if (sample >= 0.0) {
		unsigned int	peak = (unsigned int)round(sample);

		if (row > peak) {
			// off
			r = g = b = 0;
		}
		else if (row == peak) {
			// peak
			CalcRowColor((audio_sample)row, r, g, b);
		}
		else {
			// between
			CalcRowColor((audio_sample)row, r, g, b);

			if (1) {
				// reduce brightness
				r /= 4;
				g /= 4;
				b /= 4;
			}
			else if (0) {
				// reduce brightness
				r /= 2;
				g /= 2;
				b /= 2;
			}
		}
	}
	else {
		// off
		r = g = b = 0;
	}
}

void ws2812::CalcColorColoredBars(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b)
{
	if (sample >= 0.0) {
		unsigned int	peak = (unsigned int)round(sample);

		if (row > peak) {
			// off
			r = g = b = 0;
		}
		else if (row == peak) {
			// peak
			CalcRowColor(sample, r, g, b);
		}
		else {
			// between
			CalcRowColor(sample, r, g, b);

			if (0) {
				// reduce brightness
				r /= 4;
				g /= 4;
				b /= 4;
			}
			else if (1) {
				// reduce brightness
				r /= 2;
				g /= 2;
				b /= 2;
			}
		}
	}
	else {
		// off
		r = g = b = 0;
	}
}

void ws2812::GetColor(unsigned int index, unsigned int &r, unsigned int &g, unsigned int &b)
{
	assert(colorTab != nullptr);

	r = GET_COLOR_R(colorTab[index]);
	g = GET_COLOR_G(colorTab[index]);
	b = GET_COLOR_B(colorTab[index]);
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

	assert(persistenceBuffer != nullptr);

	p_r = GET_COLOR_R(persistenceBuffer[led_index]);
	p_g = GET_COLOR_G(persistenceBuffer[led_index]);
	p_b = GET_COLOR_B(persistenceBuffer[led_index]);

	CalcPersistenceMax(r, p_r);
	CalcPersistenceMax(g, p_g);
	CalcPersistenceMax(b, p_b);

	persistenceBuffer[led_index] = MAKE_COLOR(p_r, p_g, p_b);
}

void ws2812::AddPersistenceOscilloscope(unsigned int led_index, unsigned int &r, unsigned int &g, unsigned int &b)
{
	unsigned int	p_r, p_g, p_b;

	assert(persistenceBuffer != nullptr);

	p_r = GET_COLOR_R(persistenceBuffer[led_index]);
	p_g = GET_COLOR_G(persistenceBuffer[led_index]);
	p_b = GET_COLOR_B(persistenceBuffer[led_index]);

	CalcPersistenceAdd(r, p_r);
	CalcPersistenceAdd(g, p_g);
	CalcPersistenceAdd(b, p_b);

	persistenceBuffer[led_index ] = MAKE_COLOR(p_r, p_g, p_b);
}


void ws2812::ApplyBrightness(unsigned int brightness, unsigned int &r, unsigned int &g, unsigned int &b)
{
	// brightness 0...100
	r = (r * brightness) / 100;
	g = (g * brightness) / 100;
	b = (b * brightness) / 100;
}

void ws2812::ColorsToImage(unsigned int led_index, unsigned int r, unsigned int g, unsigned int b)
{
	unsigned int	color = MAKE_COLOR(r, g, b);

	assert(imageBuffer != nullptr);

	// write colors to buffer
	imageBuffer[led_index] = color;
}

void ws2812::ImageToBuffer(unsigned char *buffer, unsigned int bufferSize)
{
	unsigned int	i;
	unsigned int	r, g, b;
	unsigned int	count;

	count = bufferSize / 3;
	if (count > ledNo)
		count = ledNo;

	if (buffer && imageBuffer) {
		for (i = 0; i < count; i++) {
			// get RGB
			r = GET_COLOR_R(imageBuffer[i]);
			g = GET_COLOR_G(imageBuffer[i]);
			b = GET_COLOR_B(imageBuffer[i]);

			// all values < 2 are replaced by 0 (1 is reserved for start of block)
			if (r < 2)	r = 0;
			if (g < 2)	g = 0;
			if (b < 2)	b = 0;

			// write colors to buffer
			// color ouput order is fixed to GRB
			buffer[3 * i + 0] = (unsigned char)g;
			buffer[3 * i + 1] = (unsigned char)r;
			buffer[3 * i + 2] = (unsigned char)b;
		}
	}
}

void ws2812::ImageScrollLeft(void)
{
	unsigned int	rows = this->rows;
	unsigned int	cols = this->columns;
	unsigned int	row, col;

	if (cols > 1 && rows > 0) {
		// move current image one col to the left
		for (col = 0; col < cols - 1; col++) {
			for (row = 0; row < rows; row++) {
				// new col
				unsigned int new_index = LedIndex(row, col);
				// old col
				unsigned int old_index = LedIndex(row, col + 1);

				// copy colors
				imageBuffer[new_index] = imageBuffer[old_index];
			}
		}
	}
}

void ws2812::ImageScrollRight(void)
{
	unsigned int	rows = this->rows;
	unsigned int	cols = this->columns;
	unsigned int	row, col;

	if (cols > 1 && rows > 0) {
		// move current image one col to the right
		for (col = cols - 1; col > 0; col--) {
			for (row = 0; row < rows; row++) {
				// new col
				unsigned int new_index = LedIndex(row, col);
				// old col
				unsigned int old_index = LedIndex(row, col - 1);

				// copy colors
				imageBuffer[new_index] = imageBuffer[old_index];
			}
		}
	}
}

void ws2812::ImageScrollUp(void)
{
	unsigned int	rows = this->rows;
	unsigned int	cols = this->columns;
	unsigned int	row, col;

	if (cols > 0 && rows > 1) {
		// move current image one row up
		for (row = 0; row < rows - 1; row++) {
			for (col = 0; col < cols; col++) {
				// new col
				unsigned int new_index = LedIndex(row, col);
				// old col
				unsigned int old_index = LedIndex(row + 1, col);

				// copy colors
				imageBuffer[new_index] = imageBuffer[old_index];
			}
		}
	}
}

void ws2812::ImageScrollDown(void)
{
	unsigned int	rows = this->rows;
	unsigned int	cols = this->columns;
	unsigned int	row, col;

	if (cols > 0 && rows > 1) {
		// move current image one row down
		for (row = rows - 1; row > 0; row--) {
			for (col = 0; col < cols; col++) {
				// new col
				unsigned int new_index = LedIndex(row, col);
				// old col
				unsigned int old_index = LedIndex(row - 1, col);

				// copy colors
				imageBuffer[new_index] = imageBuffer[old_index];
			}
		}
	}
}


void ws2812::OutputSpectrumBars(const audio_sample *psample, unsigned int samples, audio_sample peak, audio_sample delta_f)
{
	unsigned int	rows = this->rows;
	unsigned int	cols = this->columns;
	unsigned int	bright = this->brightness;
	enum line_style	line_style = this->lineStyle;
	bool			log_a = this->logAmplitude;
	bool			log_f = this->logFrequency;
	bool			peaks = this->peakValues;

	unsigned int	bar, cnt, row;
	unsigned int	bar_cnt;
	unsigned int	samples_per_bar;
	unsigned int	sample_index;
	unsigned int	sample_min, sample_max;
	unsigned int	led_index;
	unsigned int	r, g, b;
	audio_sample	sum;
	audio_sample	sample;
	audio_sample	m, n;
	audio_sample	db_max;
	audio_sample	db_min;
	double			bar_freq;
	double			f_min, f_max;
	double			f_mult;
	bool			f_limit;

	// vertical bars
	bar_cnt = cols;

	// limits
	db_max = (audio_sample)this->amplMax[this->lineStyle];
	db_min = (audio_sample)this->amplMin[this->lineStyle];

	if (db_max <= db_min) {
		// invalid values
		ClearImageBuffer();
		return;
	}

	// rows per dB
	m = (audio_sample)rows / (db_max - db_min);
	// offset: no lights @ db_min
	n = 0 - m * db_min;

	// multiple samples added together for one bar
	samples_per_bar = samples / bar_cnt;
	if (samples_per_bar < 1) {
		bar_cnt = samples;
		samples_per_bar = 1;
	}

	// first bar includes samples up to 50 Hz
	f_min = 50.0;
	f_max = (double)delta_f * (double)samples;
	f_limit = false;

	sample_min = 0;
	sample_max = samples;

	// user limits
	if (this->freqMin[this->lineStyle] > frequency_min) {
		f_limit = true;

		f_min = (double)this->freqMin[this->lineStyle];
		sample_min = (unsigned int)floor(f_min / (double)delta_f);
	}

	if (this->freqMax[this->lineStyle] < frequency_max) {
		f_limit = true;

		f_max = (double)this->freqMax[this->lineStyle];
		sample_max = (unsigned int)ceil(f_max / (double)delta_f);
	}

	if (f_limit)
		samples = sample_max - sample_min;

	if (f_max <= f_min || samples <= bar_cnt) {
		// invalid values
		ClearImageBuffer();
		return;
	}

	// nth root of max_freq/min_freq, where n is the count of bars
	f_mult = pow(f_max / f_min, 1.0 / (double)bar_cnt);

	if (log_f == false) {
		// multiple samples added together for one bar
		samples_per_bar = samples / bar_cnt;
		if (samples_per_bar < 1)
			samples_per_bar = 1;
	}
	else {
		samples_per_bar = 1;
	}

	// fixed peak value for mean calculation
	peak = (audio_sample)(1.0 * samples_per_bar);

	bar_freq = f_min;
	sample_index = sample_min;

	for (bar = 0; bar < bar_cnt; bar++) {
		if (log_f == false) {
			// linear frequency scaling

			if (bar == (bar_cnt - 1)) {
				// last bar -> remaining samples
				samples_per_bar = sample_max - sample_index;
			}
			else {
				// prevent index overflow
				if ((sample_index + samples_per_bar) >= sample_max)
					samples_per_bar = sample_max - sample_index;
			}

			if (peaks == false) {
				// calc mean value of samples per bar
				sum = 0;
				for (cnt = 0; cnt < samples_per_bar; cnt++) {
					sample = psample[sample_index + cnt];
					sum += fabs(sample);
				}

				if (log_a) {
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

				if (log_a) {
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

			if (bar == (bar_cnt - 1)) {
				// last bar -> remaining samples
				if (sample_max > sample_index)
					samples_per_bar = sample_max - sample_index;
				else
					samples_per_bar = 0;
			}
			else {
				double	tmp;

				// calc start sample index of the following bar
				tmp = ceil(bar_freq / delta_f);

				// count of samples to the next index
				if (tmp <= (double)sample_index)
					samples_per_bar = 0;
				else
					samples_per_bar = (unsigned int)tmp - sample_index;

				// minimum sample count
				if (samples_per_bar < 1)
					samples_per_bar = 1;

				// prevent index overflow
				if (sample_index >= sample_max)
					samples_per_bar = 0;
				else if ((sample_index + samples_per_bar) >= sample_max)
					samples_per_bar = sample_max - sample_index;
			}

			if (samples_per_bar > 0) {
				if (peaks == false) {
					// calc mean value of samples per bar
					sum = 0;
					for (cnt = 0; cnt < samples_per_bar; cnt++) {
						sample = psample[sample_index + cnt];
						sum += fabs(sample);
					}
					// variable peak
					peak = (audio_sample)(1.0 * samples_per_bar);

					if (log_a) {
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

					if (log_a) {
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
				sample = 0;
			}
		}
		// increase max frequency for the next bar
		bar_freq *= f_mult;

		// calc height of bar in rows
		if (sample >= db_max) {
			// all rows
			sample = (audio_sample)rows;
		}
		else if (sample < db_min) {
			// none
			sample = (audio_sample)0.0;
		}
		else {
			// between
			sample = sample * m + n;
		}

		// colorize the bar
		switch (line_style)
		{
		default:
		case ws2812_spectrum_simple:		// simple white bars
			for (row = 1; row <= rows; row++) {
				// shades of white
			//	CalcColorSimple(row, sample, r, g, b);
				CalcColorColoredRows(row, sample, r, g, b);

				// invert Y axis: y = rows - row
				led_index = LedIndex(rows - row, bar);

				AddPersistenceSpectrum(led_index, r, g, b);

				ApplyBrightness(bright, r, g, b);

				ColorsToImage(led_index, r, g, b);
			}
			break;

		case ws2812_spectrum_green_red_bars:		// from green to red, each row a single color
			for (row = 1; row <= rows; row++) {
				// transition from green to red
				CalcColorColoredRows(row, sample, r, g, b);

				// invert Y axis: y = rows - row
				led_index = LedIndex(rows - row, bar);

				AddPersistenceSpectrum(led_index, r, g, b);

				ApplyBrightness(bright, r, g, b);

				ColorsToImage(led_index, r, g, b);
			}
			break;

		case ws2812_spectrum_fire_lines:		// fire style: from green to red the whole bar in one color
			for (row = 1; row <= rows; row++) {
				// transition from green to red
				CalcColorColoredBars(row, sample, r, g, b);

				// invert Y axis: y = rows - row
				led_index = LedIndex(rows - row, bar);

				AddPersistenceSpectrum(led_index, r, g, b);

				ApplyBrightness(bright, r, g, b);

				ColorsToImage(led_index, r, g, b);
			}
			break;
		}
	}
}



void ws2812::OutputSpectrogram(const audio_sample *psample, unsigned int samples, audio_sample peak, audio_sample delta_f)
{
	unsigned int	rows = this->rows;
	unsigned int	cols = this->columns;
	unsigned int	bright = this->brightness;
	bool			log_a = this->logAmplitude;
	bool			log_f = this->logFrequency;
	bool			peaks = this->peakValues;

	unsigned int	bar, cnt, row, col;
	unsigned int	bar_cnt;
	unsigned int	samples_per_bar;
	unsigned int	sample_index;
	unsigned int	sample_min, sample_max;
	unsigned int	led_index;
	unsigned int	r, g, b, i;
	audio_sample	sum;
	audio_sample	sample;
	audio_sample	m, n;
//	audio_sample	min;
	audio_sample	db_max;
	audio_sample	db_min;
	double			bar_freq;
	double			f_min, f_max;
	double			f_mult;
	bool			f_limit;

	if (lineStyle == ws2812_spectrogram_horizontal) {
		// horizontal bars
		bar_cnt = rows;
	}
	else {
		// vertical bars
		bar_cnt = cols;
	}

	// limits
	db_max = (audio_sample)this->amplMax[this->lineStyle];
	db_min = (audio_sample)this->amplMin[this->lineStyle];

	if (db_max <= db_min) {
		// invalid values
		ClearImageBuffer();
		return;
	}

	// first bar includes samples up to 50 Hz
	f_min = 50.0;
	f_max = (double)delta_f * (double)samples;
	f_limit = false;

	sample_min = 0;
	sample_max = samples;

	// user limits
	if (this->freqMin[this->lineStyle] > frequency_min) {
		f_limit = true;

		f_min = (double)this->freqMin[this->lineStyle];
		sample_min = (unsigned int)floor(f_min / (double)delta_f);
	}

	if (this->freqMax[this->lineStyle] < frequency_max) {
		f_limit = true;

		f_max = (double)this->freqMax[this->lineStyle];
		sample_max = (unsigned int)ceil(f_max / (double)delta_f);
	}

	if (f_limit)
		samples = sample_max - sample_min;

	if (f_max <= f_min || samples <= bar_cnt) {
		// invalid values
		ClearImageBuffer();
		return;
	}

	// nth root of max_freq/min_freq, where n is the count of bars
	f_mult = pow(f_max / f_min, 1.0 / (double)bar_cnt);

	if (log_f == false) {
		// multiple samples added together for one bar
		samples_per_bar = samples / bar_cnt;
		if (samples_per_bar < 1)
			samples_per_bar = 1;
	}
	else {
		samples_per_bar = 1;
	}

	// fixed peak value for mean calculation
	peak = (audio_sample)(1.0 * samples_per_bar);

#if 0
	m = (audio_sample)1.0 / (db_max - db_min);
	n = (audio_sample)1.0 - db_max * m;
	min = (audio_sample)0.5;
#else
	// use colorTab
	m = (audio_sample)colorNo / (db_max - db_min);
	n = (audio_sample)colorNo - db_max * m;
#endif

	if (lineStyle == ws2812_spectrogram_horizontal) {
		// move current image one col to the left
		ImageScrollLeft();
		// new values added in the last col (right)
		col = cols - 1;
	}
	else {
		// move current image one row up
		ImageScrollUp();
		// new values added in the last row (bottom)
		row = rows - 1;
	}

	bar_freq = f_min;
	sample_index = sample_min;

	for (bar = 0; bar < bar_cnt; bar++) {
		if (log_f == false) {
			// linear frequency scaling

			if (bar == (bar_cnt - 1)) {
				// last bar -> remaining samples
				samples_per_bar = sample_max - sample_index;
			}
			else {
				// prevent index overflow
				if ((sample_index + samples_per_bar) >= sample_max)
					samples_per_bar = sample_max - sample_index;
			}

			if (peaks == false) {
				// calc mean value of samples per bar
				sum = 0;
				for (cnt = 0; cnt < samples_per_bar; cnt++) {
					sample = psample[sample_index + cnt];
					sum += fabs(sample);
				}

				if (log_a) {
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

				if (log_a) {
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

			if (bar == (bar_cnt - 1)) {
				// last bar -> remaining samples
				if (sample_max > sample_index)
					samples_per_bar = sample_max - sample_index;
				else
					samples_per_bar = 0;
			}
			else {
				double	tmp;

				// calc start sample index of the following bar
				tmp = ceil(bar_freq / (double)delta_f);

				// count of samples to the next index
				if (tmp <= (double)sample_index)
					samples_per_bar = 0;
				else
					samples_per_bar = (unsigned int)tmp - sample_index;

				// minimum sample count
				if (samples_per_bar < 1)
					samples_per_bar = 1;

				// prevent index overflow
				if (sample_index >= sample_max)
					samples_per_bar = 0;
				else if ((sample_index + samples_per_bar) >= sample_max)
					samples_per_bar = sample_max - sample_index;
			}

			if (samples_per_bar > 0) {
				if (peaks == false) {
					// calc mean value of samples per bar
					sum = 0;
					for (cnt = 0; cnt < samples_per_bar; cnt++) {
						sample = psample[sample_index + cnt];
						sum += fabs(sample);
					}
					// variable peak
					peak = (audio_sample)(1.0 * samples_per_bar);

					if (log_a) {
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

					if (log_a) {
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
				sample = 0;
			}
		}
		// increase max frequency for the next bar
		bar_freq *= f_mult;

		// use colorTab
		sample = sample * m + n;
		if (sample < 0) {
			// off
			i = 0;
		}
		else if (sample < (audio_sample)colorNo) {
			// between
			i = (unsigned int)floorf(sample + 0.5f);
		}
		else {
			// max
			i = colorNo - 1;
		}
		GetColor(i, r, g, b);

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

		ColorsToImage(led_index, r, g, b);
	}
}



void ws2812::OutputOscilloscope(const audio_sample * psample, unsigned int samples, unsigned int samplerate, audio_sample peak)
{
	unsigned int	rows = this->rows;
	unsigned int	cols = this->columns;
	unsigned int	bright = this->brightness;

	unsigned int	col, row;
	unsigned int	sample_index;
	unsigned int	sample_min, sample_max;
	unsigned int	led_index;
	unsigned int	r, g, b, i;
	unsigned int	max_cnt;
	unsigned int	min_color;
	audio_sample	sample;
	audio_sample	offset;
	audio_sample	val_max;
	audio_sample	val_min;
	audio_sample	m, n;
	audio_sample	duration;
	unsigned int	samples_per_col;
	unsigned int	max_wfm;
	bool			softTrigger = this->peakValues;

	// min is offset
	offset = (audio_sample)this->amplMin[this->lineStyle] / 100;	// -1.0 ... 1.0
	// max is gain
	val_max = ((audio_sample)this->amplMax[this->lineStyle] / 10);	// 1.0 ... 10.0
	if (val_max > 0)
		val_max = 1 / val_max;	// 0.1 ... 1.0
	else
		val_max = 1;
	val_min = -1 * val_max;		// -0.1 ... -1.0

	// rows per "volt", invert Y axis
	m = -1 * (audio_sample)rows / (val_max - val_min);
	// center in Y
	n = (audio_sample)(rows - 1) / 2;

	// add user offset -1.0 ... 1.0 -> - rows/2 ... + rows/2
	offset = offset * (-1 * (audio_sample)rows / 2);
	n += offset;

	sample_min = 0;
	sample_max = samples;

	if (softTrigger) {
		// soft trigger is on
		audio_sample	triggerLevel = (audio_sample)0.0;
		audio_sample	triggerHyst = (audio_sample)0.02;
		audio_sample	triggerMin, triggerMax;
		unsigned int	triggerStart, triggerEnd;
		unsigned int	triggerPos[5];
		unsigned int	triggerCnt;

		triggerMin = triggerLevel - triggerHyst;
		triggerMax = triggerLevel + triggerHyst;

		triggerStart = 0;
		triggerEnd = 0;
		triggerCnt = 0;
		triggerPos[0] = 0;
		triggerPos[1] = 0;
		triggerPos[2] = 0;
		triggerPos[3] = 0;
		triggerPos[4] = 0;

		for (sample_index = 0; sample_index < samples; sample_index++) {
			sample = psample[sample_index];

			if (sample >= triggerMax) {
				// max level reached
				triggerEnd = sample_index;

				if (triggerStart > 0 && triggerEnd > triggerStart) {
					// find center
					unsigned int tmp = (triggerEnd - triggerStart) / 2 + triggerStart;

					if (triggerStart < (samples / 2) && triggerEnd <= (samples / 2)) {
						// add potential trigger position left of center
						triggerPos[0] = triggerPos[1];
						triggerPos[1] = tmp;
						triggerCnt++;
					}
					else if (triggerStart >= (samples / 2) && triggerEnd > (samples / 2)) {
						// add potential trigger position right of center
						if (triggerPos[3] == 0)
							triggerPos[3] = tmp;
						else if (triggerPos[4] == 0)
							triggerPos[4] = tmp;

						triggerCnt++;
					}
					else {
						// add potential trigger position around center
						triggerPos[5] = tmp;
						triggerCnt++;
					}

					// wait for next slope
					triggerStart = 0;
					triggerEnd = 0;
				}
				else {
					// no start found
					triggerStart = 0;
					triggerEnd = 0;
				}
			}
			else if (sample >= triggerMin) {
				// min level reached
				triggerStart = sample_index;
				triggerEnd = 0;
			}
			else {
				// outside trigger window
				triggerStart = 0;
			}
		}

		if (triggerCnt > 0) {
			// default to center
			triggerStart = (samples / 2);

			if (triggerPos[2] > 0) {
				// use center trigger position
				triggerStart = triggerPos[3];
			}
			else {
				triggerEnd = (samples / 2);

				if (triggerPos[1] > 0 && (samples / 2) >= triggerPos[1]) {
					triggerStart = (samples / 2) - triggerPos[1];
				}
				if (triggerPos[3] > 0 && triggerPos[3] >= (samples / 2)) {
					triggerEnd = triggerPos[3] - (samples / 2);
				}

				// use the position closer to the center
				if (triggerPos[1] > 0 && triggerStart < triggerEnd) {
					// left of center
					triggerStart = triggerPos[1];
				}
				else if (triggerPos[3] > 0 && triggerEnd < triggerStart) {
					// right of center
					triggerStart = triggerPos[3];
				}
			}

			// curve duration is half of the available chuck length
			// so draw one quarter of the total samples to the left, and one quarter to the right, if possible
			if (triggerStart >= (samples / 4) && (triggerStart + (samples / 4) <= samples)) {
				sample_min = triggerStart - (samples / 4);
				sample_max = triggerStart + (samples / 4) + 1;
			}
			else {
				sample_min = (samples / 4);
				sample_max = 3 * (samples / 4) + 1;
			}
		}
		else {
			// no trigger found
			softTrigger = false;
		}
	}

	// curve duration, determines number of samples per column
//	duration = (audio_sample)100e-3;
	duration = (audio_sample)(sample_max - sample_min) / (audio_sample)samplerate;

	samples_per_col = (unsigned int)floor((duration * (audio_sample)samplerate) / (audio_sample)cols);

	// limit samples_per_col if not enough samples available (the length of the requested audio buffer is currently equal to the timer interval!)
	if ((sample_max - sample_min) < (samples_per_col * cols))
		samples_per_col = (sample_max - sample_min) / cols;

	// clear counters
	ClearCounterBuffer();

	max_wfm = 1;
	row = 0;
	col = 0;
	max_cnt = 1;
	i = 0;
	for (sample_index = sample_min; sample_index < sample_max; sample_index++) {
		sample = psample[sample_index];

		// sample to row
		sample = sample * m + n;

		// prevent underflow
		if (sample >= 0) {
			row = (unsigned int)round(sample);

			// prevent overflow
			if (row < rows) {
				led_index = LedIndex(row, col);

				// sum of all hits of this pixel
				counterBuffer[led_index] += 1;

				// update max count for brightness calculation
				if (max_cnt < counterBuffer[led_index])
					max_cnt = counterBuffer[led_index];
			}
		}

#if 0
		// one samples per row, multiple chunks overlapped
#else
		// multiple samples in one column
		i++;
		if (i == samples_per_col) {
			i = 0;
#endif
			col++;
			if (col >= cols) {
				col = 0;

				// limit max overlapped waveforms
				if (max_wfm > 0)
					max_wfm--;
				else
					break;
			}
#if 0
#else
		}
#endif
	}

	min_color = colorNo / 10;

	// convert sample counts to brightness
	for (led_index = 0; led_index < rows * cols; led_index++) {
		i = counterBuffer[led_index];
		if (i > 0) {
			// colorTab entry
			i = (i * (colorNo - 1)) / max_cnt;
			// minimum brightness
			if (i < min_color)
				i = min_color;
			GetColor(i, r, g, b);
		}
		else {
			// off
			r = g = b = 0;
		}

		AddPersistenceOscilloscope(led_index, r, g, b);

		ApplyBrightness(bright, r, g, b);

		ColorsToImage(led_index, r, g, b);
	}
}

void ws2812::OutputOscillogram(const audio_sample * psample, unsigned int samples, unsigned int samplerate, audio_sample peak)
{
	unsigned int	rows = this->rows;
	unsigned int	cols = this->columns;
	unsigned int	bright = this->brightness;

	unsigned int	col, row;
	unsigned int	sample_index;
	unsigned int	sample_min, sample_max;
	unsigned int	led_index;
	unsigned int	r, g, b, i;
	unsigned int	max_cnt;
	unsigned int	min_color;
	audio_sample	sample;
	audio_sample	offset;
	audio_sample	val_max;
	audio_sample	val_min;
	audio_sample	m, n;
	bool			peakValues = this->peakValues && false;	// disabled

	// min is offset
	offset = (audio_sample)this->amplMin[this->lineStyle] / 100;	// -1.0 ... 1.0
	// max is gain
	val_max = ((audio_sample)this->amplMax[this->lineStyle] / 10);	// 1.0 ... 10.0
	if (val_max > 0)
		val_max = 1 / val_max;	// 0.1 ... 1.0
	else
		val_max = 1;
	val_min = -1 * val_max;		// -0.1 ... -1.0

	if (lineStyle == ws2812_oscillogram_horizontal) {
		// rows per "volt", invert Y axis
		m = -1 * (audio_sample)rows / (val_max - val_min);
		// center in Y
		n = (audio_sample)(rows - 1) / 2;

		// add user offset -1.0 ... 1.0 -> - rows/2 ... + rows/2
		offset = offset * (-1 * (audio_sample)rows / 2);
		n += offset;
	}
	else {
		// cols per "volt"
		m = (audio_sample)cols / (val_max - val_min);
		// center in X
		n = (audio_sample)(cols - 1) / 2;

		// add user offset -1.0 ... 1.0 -> - cols/2 ... + cols/2
		offset = offset * ((audio_sample)cols / 2);
		n += offset;
	}

	sample_min = 0;
	sample_max = samples;

	// clear counters
	ClearCounterBuffer();
	max_cnt = 1;

	min_color = colorNo / 10;

	if (lineStyle == ws2812_oscillogram_horizontal) {
		// move current image one col to the left
		ImageScrollLeft();
		// new values added in the last col (right)
		col = cols - 1;

		// map all samples to a single column
		for (sample_index = sample_min; sample_index < sample_max; sample_index++) {
			sample = psample[sample_index];

			// sample to row
			sample = sample * m + n;

			// prevent underflow
			if (sample >= 0) {
				row = (unsigned int)round(sample);

				// prevent overflow
				if (row < rows) {
					led_index = LedIndex(row, col);

					// sum of all hits of this pixel
					counterBuffer[led_index] += 1;

					// update max count for brightness calculation
					if (max_cnt < counterBuffer[led_index])
						max_cnt = counterBuffer[led_index];
				}
			}
		}

		// convert sample counts to brightness
		for (row = 0; row < rows; row++) {
			led_index = LedIndex(row, col);
			i = counterBuffer[led_index];
			if (i > 0) {
				if (peakValues) {
					i = colorNo - 1;	// ((max_cnt + 1 - i) * (colorNo - 1)) / max_cnt;
				}
				else {
					i = (i * (colorNo - 1)) / max_cnt;
				}
				// minimum brightness of hit pixels
				if (i < min_color)
					i = min_color;
				// colorTab entry
				GetColor(i, r, g, b);
			}
			else {
				// off
				r = g = b = 0;
			}

			ApplyBrightness(bright, r, g, b);

			ColorsToImage(led_index, r, g, b);
		}
	}
	else {
		// move current image one row up
		ImageScrollUp();
		// new values added in the last row (bottom)
		row = rows - 1;

		// map all samples to a single row
		for (sample_index = sample_min; sample_index < sample_max; sample_index++) {
			sample = psample[sample_index];

			// sample to col
			sample = sample * m + n;

			// prevent underflow
			if (sample >= 0) {
				col = (unsigned int)round(sample);

				// prevent overflow
				if (col < cols) {
					led_index = LedIndex(row, col);

					// sum of all hits of this pixel
					counterBuffer[led_index] += 1;

					// update max count for brightness calculation
					if (max_cnt < counterBuffer[led_index])
						max_cnt = counterBuffer[led_index];
				}
			}
		}

		// convert sample counts to brightness
		for (col = 0; col < cols; col++) {
			led_index = LedIndex(row, col);
			i = counterBuffer[led_index];
			if (i > 0) {
				if (peakValues) {
					i = colorNo - 1;	// ((max_cnt + 1 - i) * (colorNo - 1)) / max_cnt;
				}
				else {
					i = (i * (colorNo - 1)) / max_cnt;
				}
				// minimum brightness of hit pixels
				if (i < min_color)
					i = min_color;
				// colorTab entry
				GetColor(i, r, g, b);
			}
			else {
				// off
				r = g = b = 0;
			}

			ApplyBrightness(bright, r, g, b);

			ColorsToImage(led_index, r, g, b);
		}
	}
}


void ws2812::CalcAndOutput(void)
{
	double	abs_time;

	timerActive = true;

	// get current track time
	if (visStream->get_absolute_time(abs_time)) {
		audio_chunk_fast_impl	chunk;

		switch (lineStyle)
		{
		case ws2812_oscilloscope:
			{
				// length of audio data
#if 0
				double		length = (double)timerInterval * 1e-3;	//audioLength;
#else
				// read samples before and after the current track time
				double		length = (double)timerInterval * 1e-3;	//audioLength;

				if (abs_time >= length) {
					// enough time has passed -> double the length
					abs_time -= length;
					length *= 2;
				}
#endif
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
						// create image
						OutputOscilloscope(psample, samples, sample_rate, peak);

						// convert image
						ImageToBuffer(&outputBuffer[1], bufferSize - 1);
						// a value of 1 is used as start byte
						// and must not occur in other bytes in the buffer
						outputBuffer[0] = 1;

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
			break;

		case ws2812_oscillogram_horizontal:
		case ws2812_oscillogram_vertical:
		{
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
					// create image
					OutputOscillogram(psample, samples, sample_rate, peak);

					// convert image
					ImageToBuffer(&outputBuffer[1], bufferSize - 1);
					// a value of 1 is used as start byte
					// and must not occur in other bytes in the buffer
					outputBuffer[0] = 1;

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
		break;

		case ws2812_spectrogram_horizontal:
		case ws2812_spectrogram_vertical:
			{
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
						// create image
						OutputSpectrogram(psample, samples, peak, delta_f);

						// convert image
						ImageToBuffer(&outputBuffer[1], bufferSize - 1);
						// a value of 1 is used as start byte
						// and must not occur in other bytes in the buffer
						outputBuffer[0] = 1;

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
			break;

		default:
			{
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
						// create image
						OutputSpectrumBars(psample, samples, peak, delta_f);

						// convert image
						ImageToBuffer(&outputBuffer[1], bufferSize - 1);
						// a value of 1 is used as start byte
						// and must not occur in other bytes in the buffer
						outputBuffer[0] = 1;

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
			break;
		}
	}
	else {
		// playback hasn't started yet...
	}

	timerActive = false;
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

bool ws2812::GetOutputState(void)
{
	return timerStarted;
}

bool GetOutputState(void)
{
	if (ws2812_global) {
		return ws2812_global->GetOutputState();
	}
	return false;
}



bool ConfigMatrix(int rows, int cols, int start_led, int led_dir)
{
	if (ws2812_global) {
		return ws2812_global->ConfigMatrix(rows, cols, (enum start_led)start_led, (enum led_direction)led_dir);
	}
	return false;
}

bool ws2812::ConfigMatrix(int rows, int cols, enum start_led start_led, enum led_direction led_dir)
{
	bool	timerRunning;

	// allow change of one dimension only, the other must be < 0
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

			FreeBuffers();

			// allocate buffers
			initDone = AllocateBuffers();
			if (initDone) {
				if (timerRunning) {
					StartTimer();
				}
			}
		}

		if (initDone) {
			// update led mode
			ledMode = GetLedMode(start_led, led_dir);
			InitIndexLut();
		}
	}
	return initDone;
}

bool ws2812::InitColorTab(void)
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

		return true;
	}
	return false;
}

bool ws2812::InitColorTab(const unsigned int *initTab, unsigned int tabElements)
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
		return true;
	}
	return false;
}

bool ws2812::InitColorTab(const char *pattern)
{
	// web color format expected (#RRGGBB), separated by colons or semicolons
	// max eight colors
	unsigned int	initTab[8];
	unsigned int	no;
	unsigned int	color;
	unsigned int	i;
	unsigned int	digits;
	char			chr;

	// constant scaling factors used in color lookups
	this->colorsPerRow = (audio_sample)colorNo / (audio_sample)rows;
	this->colorsPerCol = (audio_sample)colorNo / (audio_sample)columns;

	if (pattern == NULL || pattern[0] == '\0') {
		// invalid parameter
		return false;
	}

	no = 0;
	digits = 7;
	color = 0;
	i = 0;
	while ((chr = pattern[i]) != '\0') {
		i++;

		if (chr == '#') {
			// start of a color
			color = 0;
			digits = 0;
		}
		else if (chr == ',' || chr == ';') {
			if (digits == 6) {
				// store color
				if (no < CALC_TAB_ELEMENTS(initTab)) {
					initTab[no] = color;
					no++;
				}
			}
			else {
				// invalid chars
			}
			// wait for #
			digits = 7;
		}
		else if (digits < 6) {
			if (chr >= '0' && chr <= '9') {
				color <<= 4;
				color += (unsigned int)(chr - '0');
				digits++;
			}
			else if (chr >= 'A' && chr <= 'F') {
				color <<= 4;
				color += (unsigned int)(chr - 'A' + 10);
				digits++;
			}
			else if (chr >= 'a' && chr <= 'f') {
				color <<= 4;
				color += (unsigned int)(chr - 'a' + 10);
				digits++;
			}
			else {
				// invalid char
				digits = 7;
			}
		}
	}

	if (digits == 6) {
		// last color, no seperator needed
		if (no < CALC_TAB_ELEMENTS(initTab)) {
			initTab[no] = color;
			no++;
		}
	}

	if (no > 0) {
		// apply colors
		return InitColorTab(initTab, no);
	}
	return false;
}


bool SetScaling(int logFrequency, int logAmplitude, int peakValues)
{
	if (ws2812_global) {
		ws2812_global->SetScaling(logFrequency, logAmplitude, peakValues);

		// save values
		ws2812_global->GetScaling(&logFrequency, &logAmplitude, &peakValues);
		SetCfgLogFrequency(logFrequency);
		SetCfgLogAmplitude(logAmplitude);
		SetCfgPeakValues(peakValues);

		return true;
	}
	return false;
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

bool GetScaling(int *logFrequency, int *logAmplitude, int *peakValues)
{
	if (ws2812_global) {
		ws2812_global->GetScaling(logFrequency, logAmplitude, peakValues);
		return true;
	}
	return false;
}

void ws2812::GetScaling(int *logFrequency, int *logAmplitude, int *peakValues)
{
	if (logFrequency != NULL)
		*logFrequency = this->logFrequency;

	if (logAmplitude != NULL)
		*logAmplitude = this->logAmplitude;

	if (peakValues != NULL)
		*peakValues = this->peakValues;
}

bool SetLineStyle(unsigned int lineStyle)
{
	if (ws2812_global) {
		ws2812_global->SetLineStyle((enum line_style)lineStyle);

		// save new value
	//	ws2812_global->GetLineStyle(&lineStyle);
	//	SetCfgLineStyle(lineStyle);

		return true;
	}
	return false;
}

void ws2812::SetLineStyle(enum line_style style)
{
	bool	timerRunning;

	if (style >= 0 && style < ws2812_line_style_no) {
		this->lineStyle = style;

		timerRunning = StopTimer();

		switch (this->lineStyle)
		{
		default:
		case ws2812_spectrum_simple:
			ClearImageBuffer();
			ClearPersistence();

			// init default spectrum colors
			InitColorTab(oscilloscopeColorTab, CALC_TAB_ELEMENTS(oscilloscopeColorTab));
			break;

		case ws2812_spectrum_green_red_bars:
		case ws2812_spectrum_fire_lines:
			ClearImageBuffer();
			ClearPersistence();

			// init default spectrum colors
			InitColorTab(spectrumColorTab, CALC_TAB_ELEMENTS(spectrumColorTab));
			break;

		case ws2812_spectrogram_horizontal:
		case ws2812_spectrogram_vertical:
			ClearImageBuffer();

			// init default spectrogram colors
			InitColorTab(spectrogramColorTab, CALC_TAB_ELEMENTS(spectrogramColorTab));
			break;

		case ws2812_oscilloscope:
		case ws2812_oscillogram_horizontal:
		case ws2812_oscillogram_vertical:
			ClearImageBuffer();
			ClearPersistence();

			// init default oscilloscope colors
			InitColorTab(oscilloscopeColorTab, CALC_TAB_ELEMENTS(oscilloscopeColorTab));
			break;
		}

		if (timerRunning)
			StartTimer();
	}
}

bool GetLineStyle(unsigned int *lineStyle)
{
	if (ws2812_global) {
		ws2812_global->GetLineStyle(lineStyle);
		return true;
	}
	return false;
}

bool GetMinAmplitudeIsOffset(void)
{
	unsigned int	lineStyle = 0;

	if (ws2812_global) {
		ws2812_global->GetLineStyle(&lineStyle);

		if (lineStyle == ws2812_oscilloscope
			|| lineStyle == ws2812_oscillogram_horizontal
			|| lineStyle == ws2812_oscillogram_vertical)
		{
			return true;
		}
	}
	return false;
}

bool GetMaxAmplitudeIsGain(void)
{
	unsigned int	lineStyle = 0;

	if (ws2812_global) {
		ws2812_global->GetLineStyle(&lineStyle);

		if (lineStyle == ws2812_oscilloscope
			|| lineStyle == ws2812_oscillogram_horizontal
			|| lineStyle == ws2812_oscillogram_vertical)
		{
			return true;
		}
	}
	return false;
}

void ws2812::GetLineStyle(unsigned int *lineStyle)
{
	if (lineStyle != NULL)
		*lineStyle = (unsigned int)this->lineStyle;
}

bool SetBrightness(unsigned int brightness)
{
	if (ws2812_global) {
		ws2812_global->SetBrightness(brightness);

		return true;
	}
	return false;
}

void ws2812::SetBrightness(unsigned int brightness)
{
	if (brightness >= brightness_min && brightness <= brightness_max) {
		if (this->brightness != brightness) {
			this->brightness = brightness;
		}
	}
}

void ws2812::GetBrightness(unsigned int *brightness)
{
	if (brightness != NULL)
		*brightness = this->brightness;
}

bool GetBrightness(unsigned int *brightness)
{
	if (ws2812_global) {
		ws2812_global->GetBrightness(brightness);
		return true;
	}
	return false;
}

bool SetComPort(unsigned int port)
{
	if (ws2812_global) {
		return ws2812_global->SetComPort(port);
	}
	return false;
}

bool ws2812::SetComPort(unsigned int port)
{
	bool	result = false;
	bool	timerRunning;

	if (port >= port_min && port <= port_max) {
		if (comPort != port) {
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

bool SetComBaudrate(unsigned int baudrate)
{
	if (ws2812_global) {
		return ws2812_global->SetComPort((enum ws2812_baudrate)baudrate);
	}
	return false;
}

bool ws2812::SetComBaudrate(enum ws2812_baudrate baudrate)
{
	bool	result = false;
	bool	timerRunning;

	if (baudrate < ws2812_baudrate_no) {
		if (comBaudrate != baudrate) {
			comBaudrate = baudrate;

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


bool SetInterval(unsigned int interval)
{
	if (ws2812_global) {
		ws2812_global->SetInterval(interval);

		return true;
	}
	return false;
}

void ws2812::SetInterval(unsigned int interval)
{
	if (interval >= timerInterval_min && interval <= timerInterval_max) {
		if (this->timerInterval != interval) {
			this->timerInterval = interval;
			if (hTimer != INVALID_HANDLE_VALUE) {
				ChangeTimerQueueTimer(NULL, hTimer, this->timerStartDelay, this->timerInterval);
			}
		}
	}
}

bool GetInterval(unsigned int *interval)
{
	if (ws2812_global) {
		ws2812_global->GetInterval(interval);
		return true;
	}
	return false;
}

void ws2812::GetInterval(unsigned int *interval)
{
	if (interval != NULL)
		*interval = this->timerInterval;
}


void ws2812::SetAmplitudeMinMax(int min, int max)
{
	if (min < max) {
		bool changed = false;

		if (this->lineStyle == ws2812_oscilloscope
			|| this->lineStyle == ws2812_oscillogram_horizontal
			|| this->lineStyle == ws2812_oscillogram_vertical)
		{
			// min is offset
			if (min >= offset_oscilloscope_min && min <= offset_oscilloscope_max) {
				changed |= (this->amplMin[this->lineStyle] != min);
			}
			else {
				min = this->amplMin[this->lineStyle];
			}
			// max is gain
			if (max >= gain_oscilloscope_min && max <= gain_oscilloscope_max) {
				changed |= (this->amplMax[this->lineStyle] != max);
			}
			else {
				max = this->amplMax[this->lineStyle];
			}
		}
		else {
			if (min >= amplitude_min && min <= amplitude_max) {
				changed |= (this->amplMin[this->lineStyle] != min);
			}
			else {
				min = this->amplMin[this->lineStyle];
			}

			if (max >= amplitude_min && max <= amplitude_max) {
				changed |= (this->amplMax[this->lineStyle] != max);
			}
			else {
				max = this->amplMax[this->lineStyle];
			}
		}

		if (changed) {
			switch (this->lineStyle)
			{
			case ws2812_spectrum_simple:
			case ws2812_spectrum_green_red_bars:
			case ws2812_spectrum_fire_lines:
				// common values for spectrum
				this->amplMin[ws2812_spectrum_simple] = min;
				this->amplMax[ws2812_spectrum_simple] = max;
				this->amplMin[ws2812_spectrum_green_red_bars] = min;
				this->amplMax[ws2812_spectrum_green_red_bars] = max;
				this->amplMin[ws2812_spectrum_fire_lines] = min;
				this->amplMax[ws2812_spectrum_fire_lines] = max;
				break;

			case ws2812_spectrogram_horizontal:
			case ws2812_spectrogram_vertical:
				// common values for spectrogram
				this->amplMin[ws2812_spectrogram_horizontal] = min;
				this->amplMax[ws2812_spectrogram_horizontal] = max;
				this->amplMin[ws2812_spectrogram_vertical] = min;
				this->amplMax[ws2812_spectrogram_vertical] = max;
				break;

			case ws2812_oscilloscope:
			case ws2812_oscillogram_horizontal:
			case ws2812_oscillogram_vertical:
				// common values for oscilloscope and oscillogram
				this->amplMin[ws2812_oscilloscope] = min;
				this->amplMax[ws2812_oscilloscope] = max;
				this->amplMin[ws2812_oscillogram_horizontal] = min;
				this->amplMax[ws2812_oscillogram_horizontal] = max;
				this->amplMin[ws2812_oscillogram_vertical] = min;
				this->amplMax[ws2812_oscillogram_vertical] = max;
				break;

			default:
				break;
			}
		}
	}
}

bool SetAmplitudeMinMax(int min, int max)
{
	if (ws2812_global) {
		ws2812_global->SetAmplitudeMinMax(min, max);
		return true;
	}
	return false;
}

void SaveAmplitudeMinMax()
{
	if (ws2812_global) {
		unsigned int style;
		int min, max;

		ws2812_global->GetLineStyle(&style);
		ws2812_global->GetAmplitudeMinMax(&min, &max);

		switch (style)
		{
		default:
		case ws2812_spectrum_simple:
		case ws2812_spectrum_green_red_bars:
		case ws2812_spectrum_fire_lines:
			SetCfgSpectrumAmplitudeMinMax(min, max);
			break;

		case ws2812_spectrogram_horizontal:
		case ws2812_spectrogram_vertical:
			SetCfgSpectrogramAmplitudeMinMax(min, max);
			break;

		case ws2812_oscilloscope:
		case ws2812_oscillogram_horizontal:
		case ws2812_oscillogram_vertical:
			SetCfgOscilloscopeOffsetAmplitude(min, max);
			break;
		}
	}
}


void ws2812::GetAmplitudeMinMax(int *min, int *max)
{
	if (min != NULL)
		*min = this->amplMin[this->lineStyle];
	if (max != NULL)
		*max = this->amplMax[this->lineStyle];
}

bool GetAmplitudeMinMax(int *min, int *max)
{
	if (ws2812_global) {
		ws2812_global->GetAmplitudeMinMax(min, max);
		return true;
	}
	return false;
}

void ws2812::SetFrequencyMinMax(int min, int max)
{
	if (min < max) {
		bool changed = false;

		if (min >= frequency_min && min <= frequency_max) {
			changed |= (this->freqMin[this->lineStyle] != min);
		}
		else {
			min = this->freqMin[this->lineStyle];
		}

		if (max >= frequency_min && max <= frequency_max) {
			changed |= (this->freqMax[this->lineStyle] != max);
		}
		else {
			max = this->freqMax[this->lineStyle];
		}

		if (changed) {
			switch (this->lineStyle)
			{
			case ws2812_spectrum_simple:
			case ws2812_spectrum_green_red_bars:
			case ws2812_spectrum_fire_lines:
				// common values for spectrum
				this->freqMin[ws2812_spectrum_simple] = min;
				this->freqMax[ws2812_spectrum_simple] = max;
				this->freqMin[ws2812_spectrum_green_red_bars] = min;
				this->freqMax[ws2812_spectrum_green_red_bars] = max;
				this->freqMin[ws2812_spectrum_fire_lines] = min;
				this->freqMax[ws2812_spectrum_fire_lines] = max;
				break;

			case ws2812_spectrogram_horizontal:
			case ws2812_spectrogram_vertical:
				// common values for spectrogram
				this->freqMin[ws2812_spectrogram_horizontal] = min;
				this->freqMax[ws2812_spectrogram_horizontal] = max;
				this->freqMin[ws2812_spectrogram_vertical] = min;
				this->freqMax[ws2812_spectrogram_vertical] = max;
				break;

			case ws2812_oscilloscope:
			case ws2812_oscillogram_horizontal:
			case ws2812_oscillogram_vertical:
				// common values for oscilloscope and oscillogram
				this->freqMin[ws2812_oscilloscope] = min;
				this->freqMax[ws2812_oscilloscope] = max;
				this->freqMin[ws2812_oscillogram_horizontal] = min;
				this->freqMax[ws2812_oscillogram_horizontal] = max;
				this->freqMin[ws2812_oscillogram_vertical] = min;
				this->freqMax[ws2812_oscillogram_vertical] = max;
				break;

			default:
				break;
			}
		}
	}
}

bool SetFrequencyMinMax(int min, int max)
{
	if (ws2812_global) {
		ws2812_global->SetFrequencyMinMax(min, max);
		return true;
	}
	return false;
}

void ws2812::GetFrequencyMinMax(int *min, int *max)
{
	if (min != NULL)
		*min = this->freqMin[this->lineStyle];
	if (max != NULL)
		*max = this->freqMax[this->lineStyle];
}

bool GetFrequencyMinMax(int *min, int *max)
{
	if (ws2812_global) {
		ws2812_global->GetFrequencyMinMax(min, max);
		return true;
	}
	return false;
}

void SaveFrequencyMinMax()
{
	if (ws2812_global) {
		unsigned int style;
		int min, max;

		ws2812_global->GetLineStyle(&style);
		ws2812_global->GetFrequencyMinMax(&min, &max);

		switch (style)
		{
		default:
		case ws2812_spectrum_simple:
		case ws2812_spectrum_green_red_bars:
		case ws2812_spectrum_fire_lines:
			SetCfgSpectrumFrequencyMinMax(min, max);
			break;

		case ws2812_spectrogram_horizontal:
		case ws2812_spectrogram_vertical:
			SetCfgSpectrogramFrequencyMinMax(min, max);
			break;

		case ws2812_oscilloscope:
			break;
		}
	}
}


bool InitColorTab(const char *pattern)
{
	if (ws2812_global)
		return ws2812_global->InitColorTab(pattern);
	return false;
}



bool ws2812::AllocateBuffers()
{
	// count of leds
	ledNo = rows * columns;

	// start byte + GRB for each LED
	bufferSize = 1 + 3 * ledNo;

	outputBuffer		= new unsigned char[bufferSize];
	imageBuffer			= new unsigned int[ledNo];
	persistenceBuffer	= new unsigned int[ledNo];
	counterBuffer		= new unsigned int[ledNo];
	indexLut			= new unsigned int[ledNo];
	colorTab			= new unsigned int[colorNo];

	if (   outputBuffer
		&& imageBuffer
		&& persistenceBuffer
		&& counterBuffer
		&& indexLut
		&& colorTab)
	{
		ZeroMemory(outputBuffer,		bufferSize * sizeof(unsigned char));
		ZeroMemory(imageBuffer,			ledNo * sizeof(unsigned int));
		ZeroMemory(persistenceBuffer,	ledNo * sizeof(unsigned int));
		ZeroMemory(counterBuffer,		ledNo * sizeof(unsigned int));
		ZeroMemory(indexLut,			ledNo * sizeof(unsigned int));
		ZeroMemory(colorTab,			colorNo * sizeof(unsigned int));

		return true;
	}
	FreeBuffers();
	return false;
}

void ws2812::FreeBuffers()
{
	if (outputBuffer)
		delete outputBuffer;
	outputBuffer = nullptr;

	if (imageBuffer)
		delete imageBuffer;
	imageBuffer = nullptr;

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

void ws2812::ClearOutputBuffer(void)
{
	if (outputBuffer)
		ZeroMemory(outputBuffer, bufferSize * sizeof(unsigned char));
}

void ws2812::ClearImageBuffer(void)
{
	if (imageBuffer)
		ZeroMemory(imageBuffer, ledNo * sizeof(unsigned int));
}

void ws2812::ClearPersistence(void)
{
	if (persistenceBuffer)
		ZeroMemory(persistenceBuffer, ledNo * sizeof(unsigned int));
}

void ws2812::ClearCounterBuffer(void)
{
	if (counterBuffer)
		ZeroMemory(counterBuffer, ledNo * sizeof(unsigned int));
}


void ws2812::InitAmplitudeMinMax()
{
	bool changed = false;
	int min = amplitude_min, max = amplitude_max;

	// common limits for all spectrum modes
	GetCfgSpectrumAmplitudeMinMax(&min, &max);

	changed = false;
	if (min < amplitude_min) {
		min = amplitude_min;
		changed = true;
	}
	if (min > amplitude_max) {
		min = amplitude_max;
		changed = true;
	}
	if (max < amplitude_min) {
		max = amplitude_min;
		changed = true;
	}
	if (max > amplitude_max) {
		max = amplitude_max;
		changed = true;
	}
	if (min > max) {
		min = max;
		changed = true;
	}

	if (changed)
		SetCfgSpectrumAmplitudeMinMax(min, max);

	this->amplMin[ws2812_spectrum_simple] = min;
	this->amplMax[ws2812_spectrum_simple] = max;

	this->amplMin[ws2812_spectrum_green_red_bars] = min;
	this->amplMax[ws2812_spectrum_green_red_bars] = max;

	this->amplMin[ws2812_spectrum_fire_lines] = min;
	this->amplMax[ws2812_spectrum_fire_lines] = max;


	// common limits for all spectrogram modes
	GetCfgSpectrogramAmplitudeMinMax(&min, &max);

	changed = false;
	if (min < amplitude_min) {
		min = amplitude_min;
		changed = true;
	}
	if (min > amplitude_max) {
		min = amplitude_max;
		changed = true;
	}
	if (max < amplitude_min) {
		max = amplitude_min;
		changed = true;
	}
	if (max > amplitude_max) {
		max = amplitude_max;
		changed = true;
	}
	if (min > max) {
		min = max;
		changed = true;
	}

	if (changed)
		SetCfgSpectrogramAmplitudeMinMax(min, max);

	this->amplMin[ws2812_spectrogram_horizontal] = min;
	this->amplMax[ws2812_spectrogram_horizontal] = max;

	this->amplMin[ws2812_spectrogram_vertical] = min;
	this->amplMax[ws2812_spectrogram_vertical] = max;


	min = (offset_oscilloscope_max - offset_oscilloscope_max) / 2;
	max = gain_oscilloscope_min;

	// oscilloscope
	GetCfgOscilloscopeOffsetAmplitude(&min, &max);

	changed = false;
	if (min < offset_oscilloscope_min) {
		min = offset_oscilloscope_min;
		changed = true;
	}
	if (min > offset_oscilloscope_max) {
		min = offset_oscilloscope_max;
		changed = true;
	}
	if (max < gain_oscilloscope_min) {
		max = gain_oscilloscope_min;
		changed = true;
	}
	if (max > gain_oscilloscope_max) {
		max = gain_oscilloscope_max;
		changed = true;
	}
	if (min > max) {
		min = max;
		changed = true;
	}

	if (changed)
		SetCfgOscilloscopeOffsetAmplitude(min, max);

	this->amplMin[ws2812_oscilloscope] = min;
	this->amplMax[ws2812_oscilloscope] = max;

	this->amplMin[ws2812_oscillogram_horizontal] = min;
	this->amplMax[ws2812_oscillogram_horizontal] = max;

	this->amplMin[ws2812_oscillogram_vertical] = min;
	this->amplMax[ws2812_oscillogram_vertical] = max;
}

void ws2812::InitFrequencyMinMax()
{
	bool changed = false;
	int min = frequency_min, max = frequency_max;

	// common limits for all spectrum modes
	GetCfgSpectrumFrequencyMinMax(&min, &max);

	changed = false;
	if (min < frequency_min) {
		min = frequency_min;
		changed = true;
	}
	if (min > frequency_max) {
		min = frequency_max;
		changed = true;
	}
	if (max < frequency_min) {
		max = frequency_min;
		changed = true;
	}
	if (max > frequency_max) {
		max = frequency_max;
		changed = true;
	}
	if (min > max) {
		min = max;
		changed = true;
	}

	if (changed)
		SetCfgSpectrumFrequencyMinMax(min, max);

	this->freqMin[ws2812_spectrum_simple] = min;
	this->freqMax[ws2812_spectrum_simple] = max;

	this->freqMin[ws2812_spectrum_green_red_bars] = min;
	this->freqMax[ws2812_spectrum_green_red_bars] = max;

	this->freqMin[ws2812_spectrum_fire_lines] = min;
	this->freqMax[ws2812_spectrum_fire_lines] = max;


	// common limits for all spectrogram modes
	GetCfgSpectrogramFrequencyMinMax(&min, &max);

	changed = false;
	if (min < frequency_min) {
		min = frequency_min;
		changed = true;
	}
	if (min > frequency_max) {
		min = frequency_max;
		changed = true;
	}
	if (max < frequency_min) {
		max = frequency_min;
		changed = true;
	}
	if (max > frequency_max) {
		max = frequency_max;
		changed = true;
	}
	if (min > max) {
		min = max;
		changed = true;
	}

	if (changed)
		SetCfgSpectrogramFrequencyMinMax(min, max);

	this->freqMin[ws2812_spectrogram_horizontal] = min;
	this->freqMax[ws2812_spectrogram_horizontal] = max;

	this->freqMin[ws2812_spectrogram_vertical] = min;
	this->freqMax[ws2812_spectrogram_vertical] = max;


	// oscilloscope
	this->freqMin[ws2812_oscilloscope] = frequency_min;
	this->freqMax[ws2812_oscilloscope] = frequency_max;

	this->freqMin[ws2812_oscillogram_horizontal] = frequency_min;
	this->freqMax[ws2812_oscillogram_horizontal] = frequency_max;

	this->freqMin[ws2812_oscillogram_vertical] = frequency_min;
	this->freqMax[ws2812_oscillogram_vertical] = frequency_max;
}

unsigned int GetRowLimited(unsigned int rows)
{
	if (rows < ws2812::rows_min)
		rows = ws2812::rows_min;
	else if (rows > ws2812::rows_max)
		rows = ws2812::rows_max;

	return rows;
}

unsigned int GetColumnsLimited(unsigned int columns)
{
	if (columns < ws2812::columns_min)
		columns = ws2812::columns_min;
	else if (columns > ws2812::columns_max)
		columns = ws2812::columns_max;

	return columns;
}

unsigned int GetIntervalLimited(unsigned int interval)
{
	if (interval < ws2812::timerInterval_min)
		interval = ws2812::timerInterval_min;
	else if (interval > ws2812::timerInterval_max)
		interval = ws2812::timerInterval_max;

	return interval;
}

unsigned int GetBrightnessLimited(unsigned int brightness)
{
	if (brightness < ws2812::brightness_min)
		brightness = ws2812::brightness_min;
	else if (brightness > ws2812::brightness_max)
		brightness = ws2812::brightness_max;

	return brightness;
}


ws2812::ws2812()
{
	unsigned int tmp;

	hComm = INVALID_HANDLE_VALUE;
	hTimer = INVALID_HANDLE_VALUE;

	ledNo = 0;
	bufferSize = 0;
	outputBuffer = nullptr;
	imageBuffer = nullptr;
	persistenceBuffer = nullptr;
	counterBuffer = nullptr;
	indexLut = nullptr;
	colorTab = nullptr;

	timerStarted = false;
	timerActive = false;
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

		tmp = GetCfgComBaudrate();
		if (tmp >= ws2812_baudrate_no)
			comBaudrate = ws2812_baudrate_115200;
		else
			comBaudrate = (enum ws2812_baudrate)tmp;

		timerInterval = GetCfgUpdateInterval();
		if (timerInterval < 50 || timerInterval > 1000)
			timerInterval = 250;

		brightness = GetCfgBrightness();
		if (brightness < 1 || brightness > 100)
			brightness = 25;

		if (initDone) {
			const char *colors = NULL;

			tmp = GetCfgLineStyle();
			if (tmp > ws2812_line_style_no)
				tmp = ws2812_spectrum_simple;

			SetLineStyle((enum line_style)tmp);

			logFrequency = GetCfgLogFrequency() != 0;
			logAmplitude = GetCfgLogAmplitude() != 0;
			peakValues = GetCfgPeakValues() != 0;

			ledMode = GetLedMode(GetCfgStartLed(), GetCfgLedDirection());

			InitIndexLut();

			timerStartDelay = 500;

			// fast FFT
			fftSize = 8 * 1024;

			// oscilloscope display time
			audioLength = 10 * 1e-3;

			// init color tab
			switch (tmp)
			{
			case ws2812_spectrum_simple:			colors = GetCfgSpectrumColors();		break;
			case ws2812_spectrum_green_red_bars:	colors = GetCfgSpectrumBarColors();		break;
			case ws2812_spectrum_fire_lines:		colors = GetCfgSpectrumFireColors();	break;
			case ws2812_spectrogram_horizontal:		colors = GetCfgSpectrogramColors();		break;
			case ws2812_spectrogram_vertical:		colors = GetCfgSpectrogramColors();		break;
			case ws2812_oscilloscope:				colors = GetCfgOscilloscopeColors();	break;
			case ws2812_oscillogram_horizontal:		colors = GetCfgOscilloscopeColors();	break;
			case ws2812_oscillogram_vertical:		colors = GetCfgOscilloscopeColors();	break;
			}
			InitColorTab(colors);

			InitAmplitudeMinMax();
			InitFrequencyMinMax();
		}
	}
}

ws2812::ws2812(unsigned int rows, unsigned int cols, unsigned int port, enum ws2812_baudrate baudrate, unsigned int interval, enum line_style style)
{
	hComm = INVALID_HANDLE_VALUE;
	hTimer = INVALID_HANDLE_VALUE;

	ledNo = 0;
	bufferSize = 0;
	outputBuffer = nullptr;
	imageBuffer = nullptr;
	persistenceBuffer = nullptr;
	counterBuffer = nullptr;
	indexLut = nullptr;
	colorTab = nullptr;

	timerStarted = false;
	timerActive = false;
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

		if (port >= port_min && port < port_max)
			this->comPort = port;
		else
			this->comPort = port_def;

		if (baudrate < ws2812_baudrate_no)
			this->comBaudrate = baudrate;
		else
			this->comBaudrate = ws2812_baudrate_115200;

		if (interval >= timerInterval_min && interval <= timerInterval_max)
			this->timerInterval = interval;
		else
			this->timerInterval = timerInterval_def;

		this->brightness = brightness_def;

		if (initDone) {
			const char *colors = NULL;

			if (style >= 0 && style < ws2812_line_style_no)
				style = ws2812_spectrum_simple;

			SetLineStyle(style);

			logFrequency = true;
			logAmplitude = true;
			peakValues = true;

			ledMode = GetLedMode(0, 0);

			InitIndexLut();

			timerStartDelay = 500;

			// fast FFT
			fftSize = 8 * 1024;

			// oscilloscope display time
			audioLength = 10 * 1e-3;

			// init color tab
			switch (style)
			{
			case ws2812_spectrum_simple:			colors = GetCfgSpectrumColors();		break;
			case ws2812_spectrum_green_red_bars:	colors = GetCfgSpectrumBarColors();		break;
			case ws2812_spectrum_fire_lines:		colors = GetCfgSpectrumFireColors();	break;
			case ws2812_spectrogram_horizontal:		colors = GetCfgSpectrogramColors();		break;
			case ws2812_spectrogram_vertical:		colors = GetCfgSpectrogramColors();		break;
			case ws2812_oscilloscope:				colors = GetCfgOscilloscopeColors();	break;
			case ws2812_oscillogram_horizontal:		colors = GetCfgOscilloscopeColors();	break;
			case ws2812_oscillogram_vertical:		colors = GetCfgOscilloscopeColors();	break;
			}
			InitColorTab(colors);

			InitAmplitudeMinMax();
			InitFrequencyMinMax();
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
