/*
 * foo_ws2812.cpp : Implementents the visualisation of waveform and spectrum data
 *                  and the transmission via COM port.
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

#include "stdafx.h"
#include "foo_ws2812.h"


// global class instance
static std::unique_ptr<ws2812>	ws2812_global;



// COM port handling taken from the MSDN
BOOL ws2812::OpenPort(const LPCWSTR gszPort, const unsigned int port)
{
	if (m_hComm == INVALID_HANDLE_VALUE) {
		WCHAR	portStr[32];
		LPCWSTR	format = L"\\\\.\\COM%u";

		wsprintf(portStr, format, port);

		m_hComm = CreateFile(portStr,
			GENERIC_READ | GENERIC_WRITE,
			0,
			0,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			0);
	}
	if (m_hComm == INVALID_HANDLE_VALUE) {
		// error opening port; abort
		m_commErr = GetLastError();
		return false;
	}

	DCB dcb;

	ZeroMemory(&dcb, sizeof(dcb));

	// get current DCB
	if (GetCommState(m_hComm, &dcb)) {
		// 8 N 1
		dcb.ByteSize = 8;
		dcb.Parity = NOPARITY;
		dcb.StopBits = ONESTOPBIT;
		// Update DCB rate.
		switch (m_comBaudrate)
		{
		case ws2812_baudrate::e9600:	dcb.BaudRate = CBR_9600;	break;
		case ws2812_baudrate::e14400:	dcb.BaudRate = CBR_14400;	break;
		case ws2812_baudrate::e19200:	dcb.BaudRate = CBR_19200;	break;
		case ws2812_baudrate::e38400:	dcb.BaudRate = CBR_38400;	break;
		case ws2812_baudrate::e56000:	dcb.BaudRate = CBR_56000;	break;
		case ws2812_baudrate::e57600:	dcb.BaudRate = CBR_57600;	break;
		case ws2812_baudrate::e115200:	dcb.BaudRate = CBR_115200;	break;
		case ws2812_baudrate::e128000:	dcb.BaudRate = CBR_128000;	break;
		case ws2812_baudrate::e250000:	dcb.BaudRate = 250000;		break;
		case ws2812_baudrate::e256000:	dcb.BaudRate = CBR_256000;	break;
		case ws2812_baudrate::e500000:	dcb.BaudRate = 500000;		break;
		default:						dcb.BaudRate = CBR_115200;	break;
		}
		// Disable flow controls
		dcb.fOutxCtsFlow = FALSE;
		dcb.fOutxDsrFlow = FALSE;
		// must set reception control lines to high state (arduino won't receive data otherwise)
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
		dcb.fDtrControl = DTR_CONTROL_ENABLE;

		// Set new state.
		if (!SetCommState(m_hComm, &dcb)) {
			// Error in SetCommState. Possibly a problem with the communications 
			// port handle or a problem with the DCB structure itself.
			m_commErr = GetLastError();

			CloseHandle(m_hComm);
			m_hComm = INVALID_HANDLE_VALUE;
			return false;
		}
	}
	return true;
}

BOOL ws2812::ClosePort()
{
	if (m_hComm != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hComm);
		m_hComm = INVALID_HANDLE_VALUE;
		return true;
	}
	return false;
}

BOOL ws2812::WriteABuffer(const unsigned char * lpBuf, const DWORD dwToWrite)
{
	OVERLAPPED osWrite = { 0 };
	DWORD dwWritten;
	DWORD dwRes;
	BOOL fRes;

	// Port not opened?
	if (m_hComm == INVALID_HANDLE_VALUE)
		return FALSE;

	// Create this write operation's OVERLAPPED structure's hEvent.
	osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (osWrite.hEvent == NULL) {
		// error creating overlapped event handle
		return FALSE;
	}

	// Issue write.
	if (!WriteFile(m_hComm, lpBuf, dwToWrite, &dwWritten, &osWrite)) {
		m_commErr = GetLastError();
		if (m_commErr != ERROR_IO_PENDING) {
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
				if (!GetOverlappedResult(m_hComm, &osWrite, &dwWritten, FALSE))
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
	}
	else {
		// WriteFile completed immediately.
		fRes = TRUE;
	}

	CloseHandle(osWrite.hEvent);
	return fRes;
}

VOID CALLBACK WaitOrTimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	if (ws2812_global) {
		ws2812_global->CalcAndOutput();
	}
}

bool ws2812::StartTimer()
{
	if (m_timerInterval < timerInterval_min)
		m_timerInterval = timerInterval_min;
	else if (m_timerInterval > timerInterval_max)
		m_timerInterval = timerInterval_max;

	if (m_hTimer == INVALID_HANDLE_VALUE) {
		// timer not created yet
		bool r = CreateTimerQueueTimer(&m_hTimer, NULL,
			WaitOrTimerCallback, NULL, m_timerStartDelay,
			m_timerInterval, WT_EXECUTELONGFUNCTION);
		if (r) {
			// success
			m_timerStarted = true;
		}
		else {
			// failed
			m_timerStarted = false;
		}
	}
	else {
		// already created
	}
	return m_timerStarted;
}

bool ws2812::StopTimer()
{
	m_timerStarted = false;

	if (m_hTimer != INVALID_HANDLE_VALUE) {
		DeleteTimerQueueTimer(NULL, m_hTimer, NULL);
		m_hTimer = INVALID_HANDLE_VALUE;

		// wait if the timer is currently active
		while (m_timerActive) {
			Sleep(100);
		}
		return true;
	}
	return false;
}

void ws2812::OutputTest(const audio_sample *psample, unsigned int samples, audio_sample peak, unsigned char *buffer, unsigned int bufferSize)
{
	unsigned int	i;
	audio_sample	sample;
	unsigned int	bright = (m_brightness * 255) / 100;

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

ws2812_led_mode ws2812::GetLedMode(unsigned int startLed, unsigned int ledDir)
{
	ws2812_led_mode	result = ws2812_led_mode::top_left_common;

	if (ledDir == 0) {
		// common direction
		switch (startLed)
		{
		default:
		case 0:
			result = ws2812_led_mode::top_left_common;
			break;
		case 1:
			result = ws2812_led_mode::top_right_common;
			break;
		case 2:
			result = ws2812_led_mode::bottom_left_common;
			break;
		case 3:
			result = ws2812_led_mode::bottom_right_common;
			break;
		}
	}
	else {
		// alternating direction
		switch (startLed)
		{
		default:
		case 0:
			result = ws2812_led_mode::top_left_alternating;
			break;
		case 1:
			result = ws2812_led_mode::top_right_alternating;
			break;
		case 2:
			result = ws2812_led_mode::bottom_left_alternating;
			break;
		case 3:
			result = ws2812_led_mode::bottom_right_alternating;
			break;
		}
	}
	return result;
}

void ws2812::InitIndexLut(void)
{
	const unsigned int	rows = this->m_rows;
	const unsigned int	cols = this->m_columns;
	unsigned int	row, col;
	unsigned int	idx;

	if (rows <= 0 || cols <= 0)
		return;

	if (m_indexLut.size() < (rows * cols))
		return;

	switch (m_ledMode)
	{
	default:
	case ws2812_led_mode::top_left_common:			// Top Left, Common direction left to right
		// 1:1
		for (idx = 0; idx < (rows * cols); idx++) {
			m_indexLut[idx] = idx;
		}
		break;

	case ws2812_led_mode::top_right_common:			// Top Right, Common direction right to left
		idx = 0;
		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				m_indexLut[idx] = row * cols + (cols - 1 - col);
				idx++;
			}
		}
		break;

	case ws2812_led_mode::bottom_left_common:		// Bottom Left, Common direction left to right
		idx = 0;
		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				m_indexLut[idx] = (rows - 1 - row) * cols + col;
				idx++;
			}
		}
		break;

	case ws2812_led_mode::bottom_right_common:		// Bottom Right, Common direction right to left
		idx = 0;
		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				m_indexLut[idx] = (rows - 1 - row) * cols + (cols - 1 - col);
				idx++;
			}
		}
		break;

	case ws2812_led_mode::top_left_alternating:		// Top Left, Alternating direction left to right/right to left
		idx = 0;
		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				if ((row & 1) == 0) {
					// even row: left to right
					m_indexLut[idx] = row * cols + col;
				}
				else {
					// odd row: right to left
					m_indexLut[idx] = row * cols + (cols - 1 - col);
				}
				idx++;
			}
		}
		break;

	case ws2812_led_mode::top_right_alternating:		// Top Right, Alternating direction right to left/left to right
		idx = 0;
		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				if ((row & 1) != 0) {
					// odd row: left to right
					m_indexLut[idx] = row * cols + col;
				}
				else {
					// even row: right to left
					m_indexLut[idx] = row * cols + (cols - 1 - col);
				}
				idx++;
			}
		}
		break;

	case ws2812_led_mode::bottom_left_alternating:	// Bottom Left, Alternating direction left to right/right to left
		idx = 0;
		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				if ((row & 1) == (rows & 1)) {
					// last row, second next to last and so on: left to right
					m_indexLut[idx] = (rows - 1 - row) * cols + col;
				}
				else {
					// other row: right to left
					m_indexLut[idx] = (rows - 1 - row) * cols + (cols - 1 - col);
				}
				idx++;
			}
		}
		break;

	case ws2812_led_mode::bottom_right_alternating:	// Bottom Right, Alternating right to left/direction left to right
		idx = 0;
		for (row = 0; row < rows; row++) {
			for (col = 0; col < cols; col++) {
				if ((row & 1) == (rows & 1)) {
					// last row, second next to last and so on: right to left
					m_indexLut[idx] = (rows - 1 - row) * cols + (cols - 1 - col);
				}
				else {
					// other row: left to right
					m_indexLut[idx] = (rows - 1 - row) * cols + col;
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
	return m_indexLut.at(row * m_columns + col);
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
		GetColor(m_colorNo - 1, r, g, b);
	}
	else {
		// between
		// use max color table entry
		GetColor(m_colorNo - 1, r, g, b);
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
	else if (row >= (audio_sample)m_rows) {
		// peak
		GetColor(m_colorNo - 1, r, g, b);
	}
	else {
		// between
		tabIdx = (unsigned int)floor(row * this->m_colorsPerRow);
		GetColor(tabIdx, r, g, b);
	}
}

void ws2812::CalcRowColor(audio_sample row, unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &w)
{
	// use color table
	unsigned int	tabIdx;

	if (row <= 0) {
		// off
		r = g = b = w = 0;
	}
	else if (row >= (audio_sample)m_rows) {
		// peak
		GetColor(m_colorNo - 1, r, g, b, w);
	}
	else {
		// between
		tabIdx = (unsigned int)floor(row * this->m_colorsPerRow);
		GetColor(tabIdx, r, g, b, w);
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

void ws2812::CalcColorColoredRows(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &w)
{
	if (sample >= 0.0) {
		unsigned int	peak = (unsigned int)round(sample);

		if (row > peak) {
			// off
			r = g = b = w = 0;
		}
		else if (row == peak) {
			// peak
			CalcRowColor((audio_sample)row, r, g, b, w);
		}
		else {
			// between
			CalcRowColor((audio_sample)row, r, g, b, w);

			if (1) {
				// reduce brightness
				r /= 4;
				g /= 4;
				b /= 4;
				w /= 4;
			}
			else if (0) {
				// reduce brightness
				r /= 2;
				g /= 2;
				b /= 2;
				w /= 2;
			}
		}
	}
	else {
		// off
		r = g = b = w = 0;
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

void ws2812::CalcColorColoredBars(unsigned int row, audio_sample sample, unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &w)
{
	if (sample >= 0.0) {
		unsigned int	peak = (unsigned int)round(sample);

		if (row > peak) {
			// off
			r = g = b = w = 0;
		}
		else if (row == peak) {
			// peak
			CalcRowColor(sample, r, g, b, w);
		}
		else {
			// between
			CalcRowColor(sample, r, g, b, w);

			if (0) {
				// reduce brightness
				r /= 4;
				g /= 4;
				b /= 4;
				w /= 4;
			}
			else if (1) {
				// reduce brightness
				r /= 2;
				g /= 2;
				b /= 2;
				w /= 2;
			}
		}
	}
	else {
		// off
		r = g = b = w = 0;
	}
}

void ws2812::GetColor(const unsigned int index, unsigned int &r, unsigned int &g, unsigned int &b)
{
	unsigned int color = m_colorTab.at(index);

	r = GET_COLOR_R(color);
	g = GET_COLOR_G(color);
	b = GET_COLOR_B(color);
}

void ws2812::GetColor(const unsigned int index, unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &w)
{
	unsigned int color = m_colorTab.at(index);

	r = GET_COLOR_R(color);
	g = GET_COLOR_G(color);
	b = GET_COLOR_B(color);
	w = GET_COLOR_W(color);
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

void ws2812::AddPersistenceSpectrum(const unsigned int index, unsigned int &r, unsigned int &g, unsigned int &b)
{
	unsigned int	p_r, p_g, p_b;
	unsigned int	p = m_persistenceBuffer.at(index);

	p_r = GET_COLOR_R(p);
	p_g = GET_COLOR_G(p);
	p_b = GET_COLOR_B(p);

	CalcPersistenceMax(r, p_r);
	CalcPersistenceMax(g, p_g);
	CalcPersistenceMax(b, p_b);

	m_persistenceBuffer.at(index) = MAKE_COLOR(p_r, p_g, p_b, 0);
}

void ws2812::AddPersistenceSpectrum(const unsigned int index, unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &w)
{
	unsigned int	p_r, p_g, p_b, p_w;
	unsigned int	p = m_persistenceBuffer.at(index);

	p_r = GET_COLOR_R(p);
	p_g = GET_COLOR_G(p);
	p_b = GET_COLOR_B(p);
	p_w = GET_COLOR_W(p);

	CalcPersistenceMax(r, p_r);
	CalcPersistenceMax(g, p_g);
	CalcPersistenceMax(b, p_b);
	CalcPersistenceMax(w, p_w);

	m_persistenceBuffer.at(index) = MAKE_COLOR(p_r, p_g, p_b, p_w);
}

void ws2812::AddPersistenceOscilloscope(const unsigned int index, unsigned int &r, unsigned int &g, unsigned int &b)
{
	unsigned int	p_r, p_g, p_b;
	unsigned int	p = m_persistenceBuffer.at(index);

	p_r = GET_COLOR_R(p);
	p_g = GET_COLOR_G(p);
	p_b = GET_COLOR_B(p);

	CalcPersistenceAdd(r, p_r);
	CalcPersistenceAdd(g, p_g);
	CalcPersistenceAdd(b, p_b);

	m_persistenceBuffer.at(index) = MAKE_COLOR(p_r, p_g, p_b, 0);
}

void ws2812::AddPersistenceOscilloscope(const unsigned int index, unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &w)
{
	unsigned int	p_r, p_g, p_b, p_w;
	unsigned int	p = m_persistenceBuffer.at(index);

	p_r = GET_COLOR_R(p);
	p_g = GET_COLOR_G(p);
	p_b = GET_COLOR_B(p);
	p_w = GET_COLOR_W(p);

	CalcPersistenceAdd(r, p_r);
	CalcPersistenceAdd(g, p_g);
	CalcPersistenceAdd(b, p_b);
	CalcPersistenceAdd(w, p_w);

	m_persistenceBuffer.at(index) = MAKE_COLOR(p_r, p_g, p_b, p_w);
}


void ws2812::ApplyBrightness(unsigned int brightness, unsigned int &r, unsigned int &g, unsigned int &b)
{
	// brightness 0...100
	r = (r * brightness) / 100;
	g = (g * brightness) / 100;
	b = (b * brightness) / 100;
}

void ws2812::ApplyBrightness(unsigned int brightness, unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &w)
{
	// brightness 0...100
	r = (r * brightness) / 100;
	g = (g * brightness) / 100;
	b = (b * brightness) / 100;
	w = (w * brightness) / 100;
}

void ws2812::ColorsToImage(unsigned int led_index, unsigned int r, unsigned int g, unsigned int b)
{
	unsigned int	color = MAKE_COLOR(r, g, b, 0);

	// write colors to buffer
	m_imageBuffer.at(led_index) = color;
}

void ws2812::ColorsToImage(unsigned int led_index, unsigned int r, unsigned int g, unsigned int b, unsigned int w)
{
	unsigned int	color = MAKE_COLOR(r, g, b, w);

	// write colors to buffer
	m_imageBuffer.at(led_index) = color;
}

void ws2812::ClipColors(unsigned int &r, unsigned int &g, unsigned int &b)
{
	const auto valMax = m_ledPwmLimit;

	// clip to max
	if (r > valMax) r = valMax;
	if (g > valMax) g = valMax;
	if (b > valMax) b = valMax;

	// replace SOFs
	for (auto valSof : m_ledSof) {
		if (valSof < valMax) {
			if (r == valSof) r = 0;
			if (g == valSof) g = 0;
			if (b == valSof) b = 0;
		}
	}
}

void ws2812::ClipColors(unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &w)
{
	const auto valMax = m_ledPwmLimit;

	// clip to max
	if (r > valMax) r = valMax;
	if (g > valMax) g = valMax;
	if (b > valMax) b = valMax;
	if (w > valMax) w = valMax;

	// replace SOFs
	for (auto valSof : m_ledSof) {
		if (valSof < valMax) {
			if (r == valSof) r = 0;
			if (g == valSof) g = 0;
			if (b == valSof) b = 0;
			if (w == valSof) w = 0;
		}
	}
}

void ws2812::GetColorIndexes(unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &w, unsigned int &no)
{
	switch (m_ledColors)
	{
	default:
	case ws2812_led_colors::eGRB:
		r = 1;
		g = 0;
		b = 2;
		no = 3;
		break;

	case ws2812_led_colors::eBRG:
		r = 1;
		g = 2;
		b = 0;
		no = 3;
		break;

	case ws2812_led_colors::eRGB:
		r = 0;
		g = 1;
		b = 2;
		no = 3;
		break;

	case ws2812_led_colors::eGRBW:
		r = 1;
		g = 0;
		b = 2;
		w = 3;
		no = 4;
		break;

	case ws2812_led_colors::eRGBW:
		r = 0;
		g = 1;
		b = 2;
		w = 3;
		no = 4;
		break;
	}
}

void ws2812::ImageToBuffer(unsigned int offset, unsigned int count, unsigned int bufferSize)
{
	unsigned int	i;
	unsigned int	led_0;
	unsigned int	led_1;
	unsigned int	incr = 1;
	unsigned int	r, g, b, w = 0;
	unsigned int	r_i, g_i, b_i, w_i;
	const unsigned int	bright = m_brightness;

	GetColorIndexes(r_i, g_i, b_i, w_i, incr);

	if (bufferSize == 0 || incr == 0 || count == 0)
		return;

	if (count > (bufferSize - 1) / incr)
		count = (bufferSize - 1) / incr;
	if (count > m_ledNo)
		count = m_ledNo;

	led_0 = offset;
	if (led_0 > m_ledNo)
		led_0 = m_ledNo;

	led_1 = led_0 + count;
	if (led_1 > m_ledNo)
		led_1 = m_ledNo;

	if (count > led_1 - led_0)
		count = led_1 - led_0;

	if (m_outputBuffer.size() >= (count * incr) && m_imageBuffer.size() >= count) {
		// always skip the first byte (start of frame)
		r_i += 1;
		g_i += 1;
		b_i += 1;
		w_i += 1;

		if (incr == 4) {
			for (i = 0; i < count; i++) {
				// get WRGB
				r = GET_COLOR_R(m_imageBuffer[led_0]);
				g = GET_COLOR_G(m_imageBuffer[led_0]);
				b = GET_COLOR_B(m_imageBuffer[led_0]);
				w = GET_COLOR_W(m_imageBuffer[led_0]);
				led_0++;

				ApplyBrightness(bright, r, g, b, w);
				ClipColors(r, g, b, w);

				// write colors to buffer
				m_outputBuffer[incr * i + r_i] = (unsigned char)r;
				m_outputBuffer[incr * i + g_i] = (unsigned char)g;
				m_outputBuffer[incr * i + b_i] = (unsigned char)b;
				m_outputBuffer[incr * i + w_i] = (unsigned char)w;
			}
		}
		else {
			for (i = 0; i < count; i++) {
				// get RGB
				r = GET_COLOR_R(m_imageBuffer[led_0]);
				g = GET_COLOR_G(m_imageBuffer[led_0]);
				b = GET_COLOR_B(m_imageBuffer[led_0]);
				led_0++;

				ApplyBrightness(bright, r, g, b);
				ClipColors(r, g, b);

				// write colors to buffer
				m_outputBuffer[incr * i + r_i] = (unsigned char)r;
				m_outputBuffer[incr * i + g_i] = (unsigned char)g;
				m_outputBuffer[incr * i + b_i] = (unsigned char)b;
			}
		}
	}
}

void ws2812::LedTestToBuffer(unsigned int offset, unsigned int count, unsigned int bufferSize)
{
	unsigned int	i;
	unsigned int	incr = 1;
	unsigned int	r, g, b, w = 0;
	unsigned int	r_i, g_i, b_i, w_i = 0;
	const unsigned int	bright = m_brightness;

	GetColorIndexes(r_i, g_i, b_i, w_i, incr);

	if (bufferSize == 0 || incr == 0)
		return;

	if (count > (bufferSize - 1) / incr)
		count = (bufferSize - 1) / incr;
	if (count > m_ledNo)
		count = m_ledNo;

	if (m_outputBuffer.size() >= (count * incr)) {
		// always skip the first byte (start of frame)
		r_i += 1;
		g_i += 1;
		b_i += 1;
		w_i += 1;

		if (incr == 4) {
			// get WRGB
			r = GET_COLOR_R(m_testColor);
			g = GET_COLOR_G(m_testColor);
			b = GET_COLOR_B(m_testColor);
			w = GET_COLOR_W(m_testColor);

			ApplyBrightness(bright, r, g, b, w);
			ClipColors(r, g, b, w);

			for (i = 0; i < count; i++) {
				// write colors to buffer
				m_outputBuffer[incr * i + r_i] = (unsigned char)r;
				m_outputBuffer[incr * i + g_i] = (unsigned char)g;
				m_outputBuffer[incr * i + b_i] = (unsigned char)b;
				m_outputBuffer[incr * i + w_i] = (unsigned char)w;
			}
		}
		else {
			// get RGB
			r = GET_COLOR_R(m_testColor);
			g = GET_COLOR_G(m_testColor);
			b = GET_COLOR_B(m_testColor);

			ApplyBrightness(bright, r, g, b);
			ClipColors(r, g, b);

			for (i = 0; i < count; i++) {
				// write colors to buffer
				m_outputBuffer[incr * i + r_i] = (unsigned char)r;
				m_outputBuffer[incr * i + g_i] = (unsigned char)g;
				m_outputBuffer[incr * i + b_i] = (unsigned char)b;
			}
		}
	}
}

void ws2812::ImageScrollLeft(void)
{
	unsigned int	rows = m_rows;
	unsigned int	cols = m_columns;
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
				m_imageBuffer[new_index] = m_imageBuffer[old_index];
			}
		}
	}
}

void ws2812::ImageScrollRight(void)
{
	unsigned int	rows = m_rows;
	unsigned int	cols = m_columns;
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
				m_imageBuffer[new_index] = m_imageBuffer[old_index];
			}
		}
	}
}

void ws2812::ImageScrollUp(void)
{
	unsigned int	rows = m_rows;
	unsigned int	cols = m_columns;
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
				m_imageBuffer[new_index] = m_imageBuffer[old_index];
			}
		}
	}
}

void ws2812::ImageScrollDown(void)
{
	unsigned int	rows = m_rows;
	unsigned int	cols = m_columns;
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
				m_imageBuffer[new_index] = m_imageBuffer[old_index];
			}
		}
	}
}


void ws2812::OutputSpectrumBars(const audio_sample *psample, unsigned int samples, unsigned int channels, audio_sample peak, audio_sample delta_f)
{
	const unsigned int	rows = m_rows;
	const unsigned int	cols = m_columns;
	const ws2812_style	style = m_lineStyle;
	const unsigned int	style_idx = static_cast<unsigned int>(m_lineStyle);
	const bool			log_a = m_logAmplitude;
	const bool			log_f = m_logFrequency;
	const bool			peaks = m_peakValues;
	const bool			rgbw = m_rgbw;

	unsigned int	bar, cnt, row;
	unsigned int	bar_cnt;
	unsigned int	samples_per_bar;
	unsigned int	sample_index;
	unsigned int	sample_min, sample_max;
	unsigned int	led_index;
	unsigned int	r, g, b, w;
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
	db_max = (audio_sample)m_amplMax[style_idx];
	db_min = (audio_sample)m_amplMin[style_idx];

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
	if (m_freqMin[style_idx] > frequency_min) {
		f_limit = true;

		f_min = (double)this->m_freqMin[style_idx];
		sample_min = (unsigned int)floor(f_min / (double)delta_f);
	}

	if (m_freqMax[style_idx] < frequency_max) {
		f_limit = true;

		f_max = (double)m_freqMax[style_idx];
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

		if (rgbw) {
			// colorize the bar
			switch (style)
			{
			default:
			case ws2812_style::spectrum_simple:		// simple white bars
				for (row = 1; row <= rows; row++) {
					// shades of white
				//	CalcColorSimple(row, sample, r, g, b);
					CalcColorColoredRows(row, sample, r, g, b, w);

					// invert Y axis: y = rows - row
					led_index = LedIndex(rows - row, bar);

					AddPersistenceSpectrum(led_index, r, g, b, w);

					ColorsToImage(led_index, r, g, b, w);
				}
				break;

			case ws2812_style::spectrum_green_red_bars:		// from green to red, each row a single color
				for (row = 1; row <= rows; row++) {
					// transition from green to red
					CalcColorColoredRows(row, sample, r, g, b, w);

					// invert Y axis: y = rows - row
					led_index = LedIndex(rows - row, bar);

					AddPersistenceSpectrum(led_index, r, g, b, w);

					ColorsToImage(led_index, r, g, b, w);
				}
				break;

			case ws2812_style::spectrum_fire_lines:		// fire style: from green to red the whole bar in one color
				for (row = 1; row <= rows; row++) {
					// transition from green to red
					CalcColorColoredBars(row, sample, r, g, b, w);

					// invert Y axis: y = rows - row
					led_index = LedIndex(rows - row, bar);

					AddPersistenceSpectrum(led_index, r, g, b, w);

					ColorsToImage(led_index, r, g, b, w);
				}
				break;
			}
		}
		else {
			// colorize the bar
			switch (style)
			{
			default:
			case ws2812_style::spectrum_simple:		// simple white bars
				for (row = 1; row <= rows; row++) {
					// shades of white
				//	CalcColorSimple(row, sample, r, g, b);
					CalcColorColoredRows(row, sample, r, g, b);

					// invert Y axis: y = rows - row
					led_index = LedIndex(rows - row, bar);

					AddPersistenceSpectrum(led_index, r, g, b);

					ColorsToImage(led_index, r, g, b);
				}
				break;

			case ws2812_style::spectrum_green_red_bars:		// from green to red, each row a single color
				for (row = 1; row <= rows; row++) {
					// transition from green to red
					CalcColorColoredRows(row, sample, r, g, b);

					// invert Y axis: y = rows - row
					led_index = LedIndex(rows - row, bar);

					AddPersistenceSpectrum(led_index, r, g, b);

					ColorsToImage(led_index, r, g, b, w);
				}
				break;

			case ws2812_style::spectrum_fire_lines:		// fire style: from green to red the whole bar in one color
				for (row = 1; row <= rows; row++) {
					// transition from green to red
					CalcColorColoredBars(row, sample, r, g, b);

					// invert Y axis: y = rows - row
					led_index = LedIndex(rows - row, bar);

					AddPersistenceSpectrum(led_index, r, g, b);

					ColorsToImage(led_index, r, g, b);
				}
				break;
			}

		}
	}
}



void ws2812::OutputSpectrogram(const audio_sample *psample, unsigned int samples, unsigned int channels, audio_sample peak, audio_sample delta_f)
{
	const unsigned int	rows = m_rows;
	const unsigned int	cols = m_columns;
	const ws2812_style	style = m_lineStyle;
	const unsigned int	style_idx = static_cast<unsigned int>(m_lineStyle);
	const bool			log_a = m_logAmplitude;
	const bool			log_f = m_logFrequency;
	const bool			peaks = m_peakValues;

	unsigned int	bar, cnt, row, col;
	unsigned int	bar_cnt;
	unsigned int	samples_per_bar;
	unsigned int	sample_index;
	unsigned int	sample_min, sample_max;
	unsigned int	led_index;
	unsigned int	r, g, b, w, i;
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

	if (style == ws2812_style::spectrogram_horizontal) {
		// horizontal bars
		bar_cnt = rows;
	}
	else {
		// vertical bars
		bar_cnt = cols;
	}

	// limits
	db_max = (audio_sample)m_amplMax[style_idx];
	db_min = (audio_sample)m_amplMin[style_idx];

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
	if (m_freqMin[style_idx] > frequency_min) {
		f_limit = true;

		f_min = (double)m_freqMin[style_idx];
		sample_min = (unsigned int)floor(f_min / (double)delta_f);
	}

	if (m_freqMax[style_idx] < frequency_max) {
		f_limit = true;

		f_max = (double)m_freqMax[style_idx];
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
	m = (audio_sample)m_colorNo / (db_max - db_min);
	n = (audio_sample)m_colorNo - db_max * m;
#endif

	if (style == ws2812_style::spectrogram_horizontal) {
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
		else if (sample < (audio_sample)(m_colorNo - 1)) {
			// between
			i = (unsigned int)floorf(sample + 0.5f);
		}
		else {
			// max
			i = m_colorNo - 1;
		}
		GetColor(i, r, g, b, w);

		if (style == ws2812_style::spectrogram_horizontal) {
			// spectrum in last col, invert Y axis
			row = bar_cnt - 1 - bar;
			led_index = LedIndex(row, col);
		}
		else {
			// spectrum in last row
			col = bar;
			led_index = LedIndex(row, col);
		}

		ColorsToImage(led_index, r, g, b, w);
	}
}



void ws2812::OutputOscilloscopeYt(const audio_sample * psample, unsigned int samples, unsigned int samplerate, unsigned int channels, audio_sample peak)
{
	const unsigned int	rows = m_rows;
	const unsigned int	cols = m_columns;
	const ws2812_style	style = m_lineStyle;
	const unsigned int	style_idx = static_cast<unsigned int>(m_lineStyle);

	bool			softTrigger = m_peakValues;

	unsigned int	col, row;
	unsigned int	sample_index;
	unsigned int	sample_min, sample_max;
	unsigned int	led_index;
	unsigned int	r, g, b, w, i;
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

	// min is offset
	offset = (audio_sample)m_amplMin[style_idx] / (audio_sample)offset_oscilloscope_div;
	// max is gain
	val_max = ((audio_sample)m_amplMax[style_idx] / (audio_sample)gain_oscilloscope_div);
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
				m_counterBuffer[led_index] += 1;

				// update max count for brightness calculation
				if (max_cnt < m_counterBuffer[led_index])
					max_cnt = m_counterBuffer[led_index];
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

	min_color = m_colorNo / 10;

	if (m_rgbw) {
		// convert sample counts to brightness
		for (led_index = 0; led_index < rows * cols; led_index++) {
			i = m_counterBuffer[led_index];
			if (i > 0) {
				// colorTab entry
				i = (i * (m_colorNo - 1)) / max_cnt;
				// minimum brightness
				if (i < min_color)
					i = min_color;
				GetColor(i, r, g, b, w);
			}
			else {
				// off
				r = g = b = w = 0;
			}

			AddPersistenceOscilloscope(led_index, r, g, b, w);

			ColorsToImage(led_index, r, g, b, w);
		}
	}
	else {
		// convert sample counts to brightness
		for (led_index = 0; led_index < rows * cols; led_index++) {
			i = m_counterBuffer[led_index];
			if (i > 0) {
				// colorTab entry
				i = (i * (m_colorNo - 1)) / max_cnt;
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

			ColorsToImage(led_index, r, g, b);
		}
	}
}

void ws2812::OutputOscilloscopeXy(const audio_sample * psample, unsigned int samples, unsigned int samplerate, unsigned int channels, audio_sample peak)
{
	const unsigned int	rows = m_rows;
	const unsigned int	cols = m_columns;
	const ws2812_style	style = m_lineStyle;
	const unsigned int	style_idx = static_cast<unsigned int>(m_lineStyle);

	unsigned int	col, row;
	unsigned int	sample_index;
	unsigned int	sample_min, sample_max;
	unsigned int	led_index;
	unsigned int	r, g, b, w, i;
	unsigned int	max_cnt;
	unsigned int	min_color;
	audio_sample	sample;
	audio_sample	offset;
	audio_sample	val_max;
	audio_sample	val_min;
	audio_sample	m_x, n_x;
	audio_sample	m_y, n_y;

	// min is offset
	offset = (audio_sample)m_amplMin[style_idx] / (audio_sample)offset_oscilloscope_div;
	// max is gain
	val_max = ((audio_sample)m_amplMax[style_idx] / (audio_sample)gain_oscilloscope_div);
	if (val_max > 0)
		val_max = 1 / val_max;	// 0.1 ... 1.0
	else
		val_max = 1;
	val_min = -1 * val_max;		// -0.1 ... -1.0

	// cols per "volt"
	m_x = (audio_sample)cols / (val_max - val_min);
	// center in X
	n_x = (audio_sample)(cols - 1) / 2;

	// add user offset -1.0 ... 1.0 -> - cols/2 ... + cols/2
	offset = offset * (-1 * (audio_sample)cols / 2);
	n_x += offset;

	// rows per "volt", invert Y axis
	m_y = -1 * (audio_sample)rows / (val_max - val_min);
	// center in Y
	n_y = (audio_sample)(rows - 1) / 2;

	// add user offset -1.0 ... 1.0 -> - rows/2 ... + rows/2
	offset = offset * (-1 * (audio_sample)rows / 2);
	n_y += offset;

	sample_min = 0;
	sample_max = samples;

	// clear counters
	ClearCounterBuffer();

	row = 0;
	col = 0;
	max_cnt = 1;

	if (channels == 1) {
		for (sample_index = sample_min; sample_index < sample_max; sample_index++) {
			// sample to column
			sample = psample[sample_index];
			sample = sample * m_x + n_x;

			// prevent underflow
			if (sample >= 0) {
				col = (unsigned int)round(sample);

				// sample to row
				sample = psample[sample_index];
				sample = sample * m_y + n_y;

				// prevent underflow
				if (sample >= 0) {
					row = (unsigned int)round(sample);

					// prevent overflow
					if (row < rows && col < cols) {
						led_index = LedIndex(row, col);

						// sum of all hits of this pixel
						m_counterBuffer[led_index] += 1;

						// update max count for brightness calculation
						if (max_cnt < m_counterBuffer[led_index])
							max_cnt = m_counterBuffer[led_index];
					}
				}
			}
		}
	}
	else if (channels == 2) {
		for (sample_index = 2 * sample_min; sample_index < 2 * sample_max; sample_index += 2) {
			// sample to column
			sample = psample[sample_index];
			sample = sample * m_x + n_x;

			// prevent underflow
			if (sample >= 0) {
				col = (unsigned int)round(sample);

				// sample to row
				sample = psample[sample_index + 1];
				sample = sample * m_y + n_y;

				// prevent underflow
				if (sample >= 0) {
					row = (unsigned int)round(sample);

					// prevent overflow
					if (row < rows && col < cols) {
						led_index = LedIndex(row, col);

						// sum of all hits of this pixel
						m_counterBuffer[led_index] += 1;

						// update max count for brightness calculation
						if (max_cnt < m_counterBuffer[led_index])
							max_cnt = m_counterBuffer[led_index];
					}
				}
			}
		}
	}

	min_color = m_colorNo / 10;

	if (m_rgbw) {
		// convert sample counts to brightness
		for (led_index = 0; led_index < rows * cols; led_index++) {
			i = m_counterBuffer[led_index];
			if (i > 0) {
				// colorTab entry
				i = (i * (m_colorNo - 1)) / max_cnt;
				// minimum brightness
				if (i < min_color)
					i = min_color;
				GetColor(i, r, g, b, w);
			}
			else {
				// off
				r = g = b = w = 0;
			}

			AddPersistenceOscilloscope(led_index, r, g, b, w);

			ColorsToImage(led_index, r, g, b, w);
		}
	}
	else {
		// convert sample counts to brightness
		for (led_index = 0; led_index < rows * cols; led_index++) {
			i = m_counterBuffer[led_index];
			if (i > 0) {
				// colorTab entry
				i = (i * (m_colorNo - 1)) / max_cnt;
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

			ColorsToImage(led_index, r, g, b);
		}
	}
}

void ws2812::OutputOscillogram(const audio_sample * psample, unsigned int samples, unsigned int samplerate, unsigned int channels, audio_sample peak)
{
	const unsigned int	rows = m_rows;
	const unsigned int	cols = m_columns;
	const ws2812_style	style = m_lineStyle;
	const unsigned int	style_idx = static_cast<unsigned int>(m_lineStyle);

	unsigned int	col, row;
	unsigned int	sample_index;
	unsigned int	sample_min, sample_max;
	unsigned int	led_index;
	unsigned int	r, g, b, w, i;
	unsigned int	max_cnt;
	unsigned int	min_color;
	audio_sample	sample;
	audio_sample	offset;
	audio_sample	val_max;
	audio_sample	val_min;
	audio_sample	m, n;
	bool			peakValues = m_peakValues && false;	// disabled

	// min is offset
	offset = (audio_sample)m_amplMin[style_idx] / (audio_sample)offset_oscilloscope_div;
	// max is gain
	val_max = ((audio_sample)m_amplMax[style_idx] / (audio_sample)gain_oscilloscope_div);
	if (val_max > 0)
		val_max = 1 / val_max;	// 0.1 ... 1.0
	else
		val_max = 1;
	val_min = -1 * val_max;		// -0.1 ... -1.0

	if (style == ws2812_style::oscillogram_horizontal) {
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

	min_color = m_colorNo / 10;

	if (style == ws2812_style::oscillogram_horizontal) {
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
					m_counterBuffer[led_index] += 1;

					// update max count for brightness calculation
					if (max_cnt < m_counterBuffer[led_index])
						max_cnt = m_counterBuffer[led_index];
				}
			}
		}

		// convert sample counts to brightness
		for (row = 0; row < rows; row++) {
			led_index = LedIndex(row, col);
			i = m_counterBuffer[led_index];
			if (i > 0) {
				if (peakValues) {
					i = m_colorNo - 1;	// ((max_cnt + 1 - i) * (colorNo - 1)) / max_cnt;
				}
				else {
					i = (i * (m_colorNo - 1)) / max_cnt;
				}
				// minimum brightness of hit pixels
				if (i < min_color)
					i = min_color;
				// colorTab entry
				GetColor(i, r, g, b, w);
			}
			else {
				// off
				r = g = b = w = 0;
			}
			ColorsToImage(led_index, r, g, b, w);
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
					m_counterBuffer[led_index] += 1;

					// update max count for brightness calculation
					if (max_cnt < m_counterBuffer[led_index])
						max_cnt = m_counterBuffer[led_index];
				}
			}
		}

		// convert sample counts to brightness
		for (col = 0; col < cols; col++) {
			led_index = LedIndex(row, col);
			i = m_counterBuffer[led_index];
			if (i > 0) {
				if (peakValues) {
					i = m_colorNo - 1;	// ((max_cnt + 1 - i) * (colorNo - 1)) / max_cnt;
				}
				else {
					i = (i * (m_colorNo - 1)) / max_cnt;
				}
				// minimum brightness of hit pixels
				if (i < min_color)
					i = min_color;
				// colorTab entry
				GetColor(i, r, g, b, w);
			}
			else {
				// off
				r = g = b = w = 0;
			}
			ColorsToImage(led_index, r, g, b, w);
		}
	}
}

void ws2812::OutputImage()
{
	const unsigned int	bufferSize = m_outputBuffer.size();
	const unsigned int	stripeNo = m_ledSof.size();

	if (bufferSize > 0) {
		if (stripeNo == 1) {
			// a value of 1 is used as start byte
			// and must not occur in other bytes in the buffer
			m_outputBuffer[0] = m_ledSof.at(0);

			// convert image
			ImageToBuffer(0, m_ledNo, bufferSize);

			// send buffer
			WriteABuffer(m_outputBuffer.data(), bufferSize);
		}
		else {
			// split image into horizontal slices
			const unsigned int count = m_ledNo / stripeNo;
			const unsigned int bufferCount = ((bufferSize - 1) / stripeNo) + 1;
			unsigned int offset = 0;

			for (unsigned int i = 0; i < stripeNo; i++) {
				// a value of 1 is used as start byte
				// and must not occur in other bytes in the buffer
				m_outputBuffer[0] = m_ledSof.at(i);

				// convert image
				ImageToBuffer(offset, count, bufferCount);

				// send buffer
				WriteABuffer(m_outputBuffer.data(), bufferCount);

				offset += count;
			}
		}
	}
}

void ws2812::OutputTest()
{
	const unsigned int	bufferSize = m_outputBuffer.size();
	const unsigned int	stripeNo = m_ledSof.size();

	if (bufferSize > 0) {
		if (stripeNo == 1) {
			// a value of 1 is used as start byte
			// and must not occur in other bytes in the buffer
			m_outputBuffer[0] = m_ledSof.at(0);

			// fill output
			LedTestToBuffer(0, m_ledNo, bufferSize);

			// send buffer
			WriteABuffer(m_outputBuffer.data(), bufferSize);
		}
		else {
			// split image into horizontal slices
			const unsigned int count = m_ledNo / stripeNo;
			const unsigned int bufferCount = ((bufferSize - 1) / stripeNo) + 1;
			unsigned int offset = 0;

			for (unsigned int i = 0; i < stripeNo; i++) {
				// a value of 1 is used as start byte
				// and must not occur in other bytes in the buffer
				m_outputBuffer[0] = m_ledSof.at(i);

				// convert image
				LedTestToBuffer(offset, count, bufferCount);

				// send buffer
				WriteABuffer(m_outputBuffer.data(), bufferCount);

				offset += count;
			}
		}
	}
}

void ws2812::CalcAndOutput(void)
{
	const ws2812_style	style = m_lineStyle;

	double				abs_time;

	m_timerActive = true;

	// get current track time
	if (visStream != nullptr && visStream->get_absolute_time(abs_time)) {
		audio_chunk_fast_impl	chunk;

		switch (style)
		{
		case ws2812_style::oscilloscope_yt:
		case ws2812_style::oscilloscope_xy:
		{
			double		length;

			if (style == ws2812_style::oscilloscope_xy) {
				// stereo is required for XY mode
				visStream->set_channel_mode(visualisation_stream_v2::channel_mode_default);

				// length of audio data
#if 0
				length = (double)timerInterval * 1e-3;	//audioLength;
#else
					// read samples before and after the current track time
				length = 2 * (double)m_timerInterval * 1e-3;

				if (abs_time >= length) {
					// enough time has passed -> double the length
					abs_time -= length;
					length *= 2;
				}
#endif
			}
			else {
				// mono is preferred, unless you want to use two displays ;-)
				visStream->set_channel_mode(visualisation_stream_v2::channel_mode_mono);

				// length of audio data
#if 0
				length = (double)timerInterval * 1e-3;	//audioLength;
#else
					// read samples before and after the current track time
				length = (double)m_timerInterval * 1e-3;

				if (abs_time >= length) {
					// enough time has passed -> double the length
					abs_time -= length;
					length *= 2;
				}
#endif
			}

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
				if (psample != nullptr && samples > 0) {
					// create image
					if (style == ws2812_style::oscilloscope_xy)
						OutputOscilloscopeXy(psample, samples, sample_rate, channels, peak);
					else
						OutputOscilloscopeYt(psample, samples, sample_rate, channels, peak);

					// convert and send the image
					OutputImage();
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

		case ws2812_style::oscillogram_horizontal:
		case ws2812_style::oscillogram_vertical:
		{
			// length of audio data
			double		length = (double)m_timerInterval * 1e-3;	//audioLength;

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
				if (psample != nullptr && samples > 0) {
					// create image
					OutputOscillogram(psample, samples, sample_rate, channels, peak);

					// convert and send the image
					OutputImage();
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

		case ws2812_style::spectrogram_horizontal:
		case ws2812_style::spectrogram_vertical:
		{
			unsigned int			fft_size = m_fftSize;

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
				if (psample != nullptr && samples > 0) {
					// create image
					OutputSpectrogram(psample, samples, channels, peak, delta_f);

					// convert and send the image
					OutputImage();
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

		case ws2812_style::led_test:
			OutputTest();
			break;


		default:
		case ws2812_style::spectrum_simple:
		case ws2812_style::spectrum_green_red_bars:
		case ws2812_style::spectrum_fire_lines:
		{
			unsigned int			fft_size = m_fftSize;

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
				if (psample != nullptr && samples > 0) {
					// create image
					OutputSpectrumBars(psample, samples, channels, peak, delta_f);

					// convert and send the image
					OutputImage();
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

	m_timerActive = false;
}

bool ws2812::StopOutput(void)
{
	bool	isRunning = false;

	if (m_initDone && m_timerStarted) {
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
	if (m_initDone && m_timerStarted == false) {
		if (OpenPort(NULL, m_comPort)) {
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
	return m_timerStarted;
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
	if (m_initDone) {
		// toggle timer state
		if (m_timerStarted == false) {
			if (OpenPort(NULL, m_comPort)) {
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

	return m_timerStarted;
}

bool ws2812::GetOutputState(void)
{
	return m_timerStarted;
}

bool GetOutputState(void)
{
	if (ws2812_global) {
		return ws2812_global->GetOutputState();
	}
	return false;
}



bool ConfigMatrix(int rows, int cols, unsigned int start_led, unsigned int led_dir, unsigned int led_colors)
{
	if (ws2812_global) {
		return ws2812_global->ConfigMatrix(rows, cols, start_led, led_dir, led_colors);
	}
	return false;
}

bool ws2812::ConfigMatrix(int rows, int cols, unsigned int start_led, unsigned int led_dir, unsigned int led_colors)
{
	bool	timerRunning = false;
	unsigned int	newRows = this->m_rows;
	unsigned int	newCols = this->m_columns;

	// allow change of one dimension only, the other must be < 0
	if (rows > 0)
		newRows = static_cast<unsigned int>(rows);
	if (cols > 0)
		newCols = static_cast<unsigned int>(cols);

	if (newRows >= rows_min && newRows <= rows_max
		&& newCols >= columns_min && newCols <= columns_max)
	{
		if (this->m_rows != newRows
			|| this->m_columns != newCols
			|| this->m_ledColors != static_cast<ws2812_led_colors>(led_colors))
		{
			m_initDone = false;

			this->m_rows = newRows;
			this->m_columns = newCols;

			// kill the timer
			timerRunning = StopTimer();

			FreeBuffers();

			SetLedColors(static_cast<ws2812_led_colors>(led_colors));

			// allocate buffers
			m_initDone = AllocateBuffers();
		}

		if (m_initDone) {
			// update led mode
			m_ledMode = GetLedMode(start_led, led_dir);
			InitIndexLut();

			// restart the output timer
			if (timerRunning) {
				StartTimer();
			}
		}
	}
	return m_initDone;
}

bool ws2812::InitColorTab(void)
{
	unsigned int	i;
	unsigned int	r, g, b;
	double			r_start, g_start, b_start;
	double			r_end, g_end, b_end;
	double			r_step, g_step, b_step;

	if (m_colorTab.size() >= m_colorNo) {
		// create default color tab: green to red
		r_start = 0;
		g_start = 255;
		b_start = 0;

		r_end = 255;
		g_end = 0;
		b_end = 0;

		// dividing by colorNo - 1 ensures that the last entry is filled with the end color
		r_step = (r_end - r_start) / (m_colorNo - 1);
		g_step = (g_end - g_start) / (m_colorNo - 1);
		b_step = (b_end - b_start) / (m_colorNo - 1);

		for (i = 0; i < m_colorNo; i++) {
			r = static_cast<unsigned int>(floor(r_start));
			g = static_cast<unsigned int>(floor(g_start));
			b = static_cast<unsigned int>(floor(b_start));
			// ARGB
			m_colorTab[i] = MAKE_COLOR(r, g, b, 0);

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
	unsigned int	r, g, b, w;
	double			colors_per_segment;
	double			r_start, g_start, b_start, w_start;
	double			r_end, g_end, b_end, w_end;
	double			r_step, g_step, b_step, w_step;

	if (m_colorTab.size() >= m_colorNo && initTab && tabElements > 0) {
		if (tabElements > 1) {
			// divide colorTab into segments
			colors_per_segment = (double)m_colorNo / (double)(tabElements - 1);

			i = 0;
			j = 0;
			i_end = 0;

			for (i = 0; i < m_colorNo; i++) {
				if (i == i_end && j < (tabElements - 1)) {
					// next initTab entry
					r_start = GET_COLOR_R(initTab[j]);
					g_start = GET_COLOR_G(initTab[j]);
					b_start = GET_COLOR_B(initTab[j]);
					w_start = GET_COLOR_W(initTab[j]);

					r_end = GET_COLOR_R(initTab[j + 1]);
					g_end = GET_COLOR_G(initTab[j + 1]);
					b_end = GET_COLOR_B(initTab[j + 1]);
					w_end = GET_COLOR_W(initTab[j + 1]);

					// last colorTab index of this segment
					j++;
					i_end = static_cast<unsigned int>(floor(j * colors_per_segment));

					r_step = (r_end - r_start);
					g_step = (g_end - g_start);
					b_step = (b_end - b_start);
					w_step = (w_end - w_start);

					if (j < (tabElements - 1)) {
						// dividing by count because the first entry of the next segment will hold the end color
						r_step /= (double)(i_end - i);
						g_step /= (double)(i_end - i);
						b_step /= (double)(i_end - i);
						w_step /= (double)(i_end - i);
					}
					else {
						// dividing by count - 1 ensures that the last entry is filled with the end color
						r_step /= (double)((i_end - i) - 1);
						g_step /= (double)((i_end - i) - 1);
						b_step /= (double)((i_end - i) - 1);
						w_step /= (double)((i_end - i) - 1);
					}
				}

				r = static_cast<unsigned int>(floor(r_start + 0.5));
				g = static_cast<unsigned int>(floor(g_start + 0.5));
				b = static_cast<unsigned int>(floor(b_start + 0.5));
				w = static_cast<unsigned int>(floor(w_start + 0.5));

				m_colorTab[i] = MAKE_COLOR(r, g, b, w);

				if (i < i_end) {
					r_start += r_step;
					g_start += g_step;
					b_start += b_step;
					w_start += w_step;
				}
			}
		}
		else {
			// fill colorTab
			for (i = 0; i < m_colorNo; i++)
				m_colorTab[i] = initTab[0];
		}
		return true;
	}
	return false;
}

bool ws2812::InitColorTab(const char *pattern)
{
	// web color format expected (#RRGGBB[WW]), separated by colons or semicolons
	// max eight colors
	unsigned int	initTab[8];
	unsigned int	no;
	unsigned int	r, g, b, w;
	unsigned int	i;
	unsigned int	digits;
	char			chr;

	if (pattern == nullptr || pattern[0] == '\0') {
		// invalid parameter
		return false;
	}

	// constant scaling factors used in color lookups
	this->m_colorsPerRow = (audio_sample)m_colorNo / (audio_sample)m_rows;
	this->m_colorsPerCol = (audio_sample)m_colorNo / (audio_sample)m_columns;

	no = 0;
	digits = 255;
	r = 0;
	g = 0;
	b = 0;
	w = 0;
	i = 0;
	while ((chr = pattern[i]) != '\0') {
		i++;

		if (chr == '#') {
			// start of a color
			digits = 0;
			r = 0;
			g = 0;
			b = 0;
			w = 0;
		}
		else if (chr == ',' || chr == ';') {
			if (digits == 6) {
				// store RGB
				if (no < CALC_TAB_ELEMENTS(initTab)) {
					initTab[no] = MAKE_COLOR(r, g, b, 0);
					no++;
				}
			}
			else if (digits == 8) {
				// store RGBW
				if (no < CALC_TAB_ELEMENTS(initTab)) {
					initTab[no] = MAKE_COLOR(r, g, b, w);
					no++;
				}
			}
			else {
				// invalid chars
			}
			// wait for #
			digits = 255;
		}
		else if (digits < 8) {
			unsigned int tmp = 0;

			if (chr >= '0' && chr <= '9') {
				tmp = (unsigned int)(chr - '0');
				digits++;
			}
			else if (chr >= 'A' && chr <= 'F') {
				tmp = (unsigned int)(chr - 'A' + 10);
				digits++;
			}
			else if (chr >= 'a' && chr <= 'f') {
				tmp = (unsigned int)(chr - 'a' + 10);
				digits++;
			}
			else if (chr == ' ') {
				// spaces are ignored
			}
			else {
				// invalid char
				digits = 255;
			}

			if (digits <= 2) {
				r = (r << 4) + tmp;
			}
			else if (digits <= 4) {
				g = (g << 4) + tmp;
			}
			else if (digits <= 6) {
				b = (b << 4) + tmp;
			}
			else if (digits <= 8) {
				w = (w << 4) + tmp;
			}
			else {
				// invalid char
			}
		}
	}

	// last color, no seperator needed
	if (digits == 6) {
		// store RGB
		if (no < CALC_TAB_ELEMENTS(initTab)) {
			initTab[no] = MAKE_COLOR(r, g, b, 0);
			no++;
		}
	}
	else if (digits == 8) {
		// store RGBW
		if (no < CALC_TAB_ELEMENTS(initTab)) {
			initTab[no] = MAKE_COLOR(r, g, b, w);
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
		ws2812_global->GetScaling(logFrequency, logAmplitude, peakValues);
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
		this->m_logFrequency = false;
	else if (logFrequency > 0)
		this->m_logFrequency = true;

	if (logAmplitude == 0)
		this->m_logAmplitude = false;
	else if (logAmplitude > 0)
		this->m_logAmplitude = true;

	if (peakValues == 0)
		this->m_peakValues = false;
	else if (peakValues > 0)
		this->m_peakValues = true;
}

bool GetScaling(int &logFrequency, int &logAmplitude, int &peakValues)
{
	if (ws2812_global) {
		ws2812_global->GetScaling(logFrequency, logAmplitude, peakValues);
		return true;
	}
	return false;
}

void ws2812::GetScaling(int &logFrequency, int &logAmplitude, int &peakValues)
{
	logFrequency = this->m_logFrequency;
	logAmplitude = this->m_logAmplitude;
	peakValues = this->m_peakValues;
}

bool SetStyle(unsigned int lineStyle)
{
	if (ws2812_global) {
		ws2812_global->SetStyle(static_cast<ws2812_style>(lineStyle));

		// save new value
	//	ws2812_global->GetLineStyle(&lineStyle);
	//	SetCfgLineStyle(lineStyle);

		return true;
	}
	return false;
}

void ws2812::SetStyle(ws2812_style style)
{
	bool	timerRunning;

	if (style < ws2812_style::eNo) {
		this->m_lineStyle = style;

		timerRunning = StopTimer();

		switch (this->m_lineStyle)
		{
		default:
		case ws2812_style::spectrum_simple:
			ClearImageBuffer();
			ClearPersistence();

			// init default spectrum colors
			InitColorTab(oscilloscopeColorTab, CALC_TAB_ELEMENTS(oscilloscopeColorTab));
			break;

		case ws2812_style::spectrum_green_red_bars:
		case ws2812_style::spectrum_fire_lines:
			ClearImageBuffer();
			ClearPersistence();

			// init default spectrum colors
			InitColorTab(spectrumColorTab, CALC_TAB_ELEMENTS(spectrumColorTab));
			break;

		case ws2812_style::spectrogram_horizontal:
		case ws2812_style::spectrogram_vertical:
			ClearImageBuffer();

			// init default spectrogram colors
			InitColorTab(spectrogramColorTab, CALC_TAB_ELEMENTS(spectrogramColorTab));
			break;

		case ws2812_style::oscilloscope_yt:
		case ws2812_style::oscilloscope_xy:
		case ws2812_style::oscillogram_horizontal:
		case ws2812_style::oscillogram_vertical:
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

bool GetLineStyle(unsigned int &lineStyle)
{
	if (ws2812_global) {
		ws2812_global->GetLineStyle(lineStyle);
		return true;
	}
	return false;
}

void ws2812::GetLineStyle(unsigned int &lineStyle)
{
	lineStyle = static_cast<unsigned int>(m_lineStyle);
}

#if 0
bool SetLedColors(unsigned int ledColors)
{
	if (ws2812_global) {
		ws2812_global->SetLedColors(static_cast<ws2812_led_colors>(ledColors));

		return true;
	}
	return false;
}
#endif

void ws2812::SetLedColors(ws2812_led_colors ledColors)
{
	if (ledColors < ws2812_led_colors::eNo) {
		m_ledColors = ledColors;

		// update output buffer size
		if (m_ledColors > ws2812_led_colors::eRGB)
		{
			m_outputSize = 1 + 4 * m_ledNo;
			m_rgbw = true;
		}
		else
		{
			m_outputSize = 1 + 3 * m_ledNo;
			m_rgbw = false;
		}
	}
}

bool GetLedColors(unsigned int &ledColors)
{
	if (ws2812_global) {
		ws2812_global->GetLedColors(ledColors);
		return true;
	}
	return false;
}

void ws2812::GetLedColors(unsigned int &ledColors)
{
	ledColors = static_cast<unsigned int>(m_ledColors);
}

bool GetMinAmplitudeIsOffset(void)
{
	unsigned int	lineStyle = 0;

	if (ws2812_global) {
		ws2812_global->GetLineStyle(lineStyle);

		switch (static_cast<ws2812_style>(lineStyle))
		{
		case ws2812_style::oscilloscope_yt:
		case ws2812_style::oscilloscope_xy:
		case ws2812_style::oscillogram_horizontal:
		case ws2812_style::oscillogram_vertical:
			return true;

		default:
			return false;
		}
	}
	return false;
}

bool GetMaxAmplitudeIsGain(void)
{
	unsigned int	lineStyle = 0;

	if (ws2812_global) {
		ws2812_global->GetLineStyle(lineStyle);

		switch (static_cast<ws2812_style>(lineStyle))
		{
		case ws2812_style::oscilloscope_yt:
		case ws2812_style::oscilloscope_xy:
		case ws2812_style::oscillogram_horizontal:
		case ws2812_style::oscillogram_vertical:
			return true;

		default:
			return false;
		}
	}
	return false;
}

bool GetLedTestMode(void)
{
	unsigned int	lineStyle = 0;

	if (ws2812_global) {
		ws2812_global->GetLineStyle(lineStyle);

		if (static_cast<ws2812_style>(lineStyle) == ws2812_style::led_test)
			return true;
	}
	return false;
}

bool SetLedTestVal(unsigned int idx, unsigned int val)
{
	if (ws2812_global) {
		ws2812_global->SetLedTestVal(idx, val);

		return true;
	}
	return false;
}

void ws2812::SetLedTestVal(unsigned int idx, unsigned int val)
{
	// WRGB
	switch (idx)
	{
	case 0:		// red
		if (val < 256)
			m_testColor = SET_COLOR_R(m_testColor, val);
		break;

	case 1:		// green
		if (val < 256)
			m_testColor = SET_COLOR_G(m_testColor, val);
		break;

	case 2:		// blue
		if (val < 256)
			m_testColor = SET_COLOR_B(m_testColor, val);
		break;

	case 3:		// white
		if (val < 256)
			m_testColor = SET_COLOR_W(m_testColor, val);
		break;

	default:
		break;
	}
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
		if (this->m_brightness != brightness) {
			this->m_brightness = brightness;
		}
	}
}

void ws2812::GetBrightness(unsigned int &brightness)
{
	brightness = this->m_brightness;
}

bool GetBrightness(unsigned int &brightness)
{
	if (ws2812_global) {
		ws2812_global->GetBrightness(brightness);
		return true;
	}
	return false;
}

bool SetCurrentLimit(unsigned int limit)
{
	if (ws2812_global) {
		ws2812_global->SetCurrentLimit(limit);

		return true;
	}
	return false;
}

void ws2812::SetCurrentLimit(unsigned int limit)
{
	if (this->m_currentLimit != limit) {
		this->m_currentLimit = limit;
	}

	if (this->m_currentLimit > 0) {
		// update max. brightness
		double ledCurrent;
		double totalCurrent;
		// 20mA per single led (?)
		if (m_rgbw) {
			ledCurrent = 4 * 0.020;
		}
		else {
			ledCurrent = 3 * 0.020;
		}
		totalCurrent = ledCurrent * m_ledNo;
		if (totalCurrent > this->m_currentLimit) {
			// scale the maximum value
			double maxPwm = floor((this->m_currentLimit / totalCurrent) * led_pwm_limit_max + 0.5);

			if (maxPwm <= led_pwm_limit_min) {
				this->m_ledPwmLimit = led_pwm_limit_min;
			}
			else if (maxPwm >= led_pwm_limit_max) {
				this->m_ledPwmLimit = led_pwm_limit_max;
			}
			else {
				this->m_ledPwmLimit = static_cast<unsigned int>(maxPwm);
			}
		}
		else {
			// default
			this->m_ledPwmLimit = led_pwm_limit_def;
		}
	}
	else {
		// default
		this->m_ledPwmLimit = led_pwm_limit_def;
	}
}

void ws2812::GetCurrentLimit(unsigned int &limit)
{
	limit = this->m_currentLimit;
}

bool GetCurrentLimit(unsigned int &limit)
{
	if (ws2812_global) {
		ws2812_global->GetCurrentLimit(limit);
		return true;
	}
	return false;
}

bool SetLedStripeSof(const char *pattern)
{
	if (ws2812_global) {
		ws2812_global->SetLedStripeSof(pattern);
		return true;
	}
	return false;
}

bool ws2812::SetLedStripeSof(const char * pattern)
{
	// decimal or hexadecimal values, separated by colons or semicolons
	// max eight stripes
	unsigned int	initTab[8];
	unsigned int	no;
	unsigned int	value;
	unsigned int	i;
	unsigned int	digits;
	bool			hexVal;
	char			chr;

	if (pattern == nullptr || pattern[0] == '\0') {
		// invalid parameter
		return false;
	}

	no = 0;
	value = 0;
	hexVal = false;
	i = 0;
	digits = 0;
	while ((chr = pattern[i]) != '\0') {
		i++;

		if (chr == ',' || chr == ';') {
			// seperator
			if (digits > 0 && digits < 255) {
				if (no < CALC_TAB_ELEMENTS(initTab)) {
					initTab[no] = value;
					no++;
				}
			}
			else {
				// invalid chars
			}
			value = 0;
			digits = 0;
			hexVal = false;
		}
		else if (chr == 'x' || chr == 'X') {
			// signals hexadecimal value
			if (digits < 255) {
				hexVal = true;
				value = 0;
				digits = 0;
			}
		}
		else if (digits < 255) {
			unsigned int tmp = 0;

			if (chr >= '0' && chr <= '9') {
				tmp = (unsigned int)(chr - '0');
				digits++;
			}
			else if (chr >= 'A' && chr <= 'F') {
				if (hexVal) {
					tmp = (unsigned int)(chr - 'A' + 10);
					digits++;
				}
				else {
					// invalid char
					digits = 255;
				}
			}
			else if (chr >= 'a' && chr <= 'f') {
				if (hexVal) {
					tmp = (unsigned int)(chr - 'a' + 10);
					digits++;
				}
				else {
					// invalid char
					digits = 255;
				}
			}
			else if (chr == ' ') {
				// spaces are ignored
			}
			else {
				// invalid char
				digits = 255;
			}

			if (digits < 255) {
				if (hexVal) {
					value = (value << 4) + tmp;
				}
				else {
					value = (value * 10) + tmp;
				}
			}
		}
	}

	// last value, no seperator needed
	if (digits > 0 && digits < 255) {
		if (no < CALC_TAB_ELEMENTS(initTab)) {
			initTab[no] = value;
			no++;
		}
	}

	if (no > 0) {
		// apply values
		m_ledSof.resize(no);
		for (unsigned int i = 0; i < no; i++) {
			m_ledSof.at(i) = initTab[i];
		}
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
		if (m_comPort != port) {
			m_comPort = port;

			timerRunning = StopTimer();

			ClosePort();

			if (timerRunning) {
				if (OpenPort(NULL, m_comPort)) {
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
		return ws2812_global->SetComBaudrate((enum ws2812_baudrate)baudrate);
	}
	return false;
}

bool ws2812::SetComBaudrate(ws2812_baudrate baudrate)
{
	bool	result = false;
	bool	timerRunning;

	if (baudrate < ws2812_baudrate::eNo) {
		if (m_comBaudrate != baudrate) {
			m_comBaudrate = baudrate;

			timerRunning = StopTimer();

			ClosePort();

			if (timerRunning) {
				if (OpenPort(NULL, m_comPort)) {
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
		if (this->m_timerInterval != interval) {
			this->m_timerInterval = interval;

			if (m_hTimer != INVALID_HANDLE_VALUE) {
				ChangeTimerQueueTimer(NULL, m_hTimer, this->m_timerStartDelay, this->m_timerInterval);
			}
		}
	}
}

bool GetInterval(unsigned int &interval)
{
	if (ws2812_global) {
		ws2812_global->GetInterval(interval);
		return true;
	}
	return false;
}

void ws2812::GetInterval(unsigned int &interval)
{
	interval = this->m_timerInterval;
}

void ws2812::SetAmplitudeMinMax(ws2812_style style, int min, int max)
{
	if (min < max) {
		const unsigned int	style_idx = static_cast<unsigned int>(style);

		this->m_amplMin[style_idx] = min;
		this->m_amplMax[style_idx] = max;
	}
}

void ws2812::SetAmplitudeMinMax(int min, int max)
{
	if (min < max) {
		const ws2812_style	style = m_lineStyle;
		const unsigned int	style_idx = static_cast<unsigned int>(m_lineStyle);

		bool changed = false;

		switch (style)
		{
		case ws2812_style::oscilloscope_yt:
		case ws2812_style::oscilloscope_xy:
		case ws2812_style::oscillogram_horizontal:
		case ws2812_style::oscillogram_vertical:
			// min is offset
			if (min >= offset_oscilloscope_min && min <= offset_oscilloscope_max) {
				changed |= (this->m_amplMin[style_idx] != min);
			}
			else {
				min = this->m_amplMin[style_idx];
			}
			// max is gain
			if (max >= gain_oscilloscope_min && max <= gain_oscilloscope_max) {
				changed |= (this->m_amplMax[style_idx] != max);
			}
			else {
				max = this->m_amplMax[style_idx];
			}
			break;

		default:
			if (min >= amplitude_min && min <= amplitude_max) {
				changed |= (this->m_amplMin[style_idx] != min);
			}
			else {
				min = this->m_amplMin[style_idx];
			}

			if (max >= amplitude_min && max <= amplitude_max) {
				changed |= (this->m_amplMax[style_idx] != max);
			}
			else {
				max = this->m_amplMax[style_idx];
			}
		}

		if (changed) {
			switch (style)
			{
			case ws2812_style::spectrum_simple:
			case ws2812_style::spectrum_green_red_bars:
			case ws2812_style::spectrum_fire_lines:
				// common values for spectrum
				SetAmplitudeMinMax(ws2812_style::spectrum_simple, min, max);
				SetAmplitudeMinMax(ws2812_style::spectrum_green_red_bars, min, max);
				SetAmplitudeMinMax(ws2812_style::spectrum_fire_lines, min, max);
				break;

			case ws2812_style::spectrogram_horizontal:
			case ws2812_style::spectrogram_vertical:
				// common values for spectrogram
				SetAmplitudeMinMax(ws2812_style::spectrogram_horizontal, min, max);
				SetAmplitudeMinMax(ws2812_style::spectrogram_vertical, min, max);
				break;

			case ws2812_style::oscilloscope_yt:
			case ws2812_style::oscilloscope_xy:
			case ws2812_style::oscillogram_horizontal:
			case ws2812_style::oscillogram_vertical:
				// common values for oscilloscope and oscillogram
				SetAmplitudeMinMax(ws2812_style::oscilloscope_yt, min, max);
				SetAmplitudeMinMax(ws2812_style::oscilloscope_xy, min, max);
				SetAmplitudeMinMax(ws2812_style::oscillogram_horizontal, min, max);
				SetAmplitudeMinMax(ws2812_style::oscillogram_vertical, min, max);
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

		ws2812_global->GetLineStyle(style);
		ws2812_global->GetAmplitudeMinMax(min, max);

		switch (static_cast<ws2812_style>(style))
		{
		default:
		case ws2812_style::spectrum_simple:
		case ws2812_style::spectrum_green_red_bars:
		case ws2812_style::spectrum_fire_lines:
			SetCfgSpectrumAmplitudeMinMax(min, max);
			break;

		case ws2812_style::spectrogram_horizontal:
		case ws2812_style::spectrogram_vertical:
			SetCfgSpectrogramAmplitudeMinMax(min, max);
			break;

		case ws2812_style::oscilloscope_yt:
		case ws2812_style::oscilloscope_xy:
		case ws2812_style::oscillogram_horizontal:
		case ws2812_style::oscillogram_vertical:
			SetCfgOscilloscopeOffsetAmplitude(min, max);
			break;
		}
	}
}


void ws2812::GetAmplitudeMinMax(int &min, int &max)
{
	const unsigned int	style_idx = static_cast<unsigned int>(m_lineStyle);

	min = this->m_amplMin[style_idx];
	max = this->m_amplMax[style_idx];
}

bool GetAmplitudeMinMax(int &min, int &max)
{
	if (ws2812_global) {
		ws2812_global->GetAmplitudeMinMax(min, max);
		return true;
	}
	return false;
}

void ws2812::SetFrequencyMinMax(ws2812_style style, int min, int max)
{
	if (min < max) {
		const unsigned int	style_idx = static_cast<unsigned int>(style);

		this->m_freqMin[style_idx] = min;
		this->m_freqMax[style_idx] = max;
	}
}

void ws2812::SetFrequencyMinMax(int min, int max)
{
	if (min < max) {
		const ws2812_style	style = m_lineStyle;
		const unsigned int	style_idx = static_cast<unsigned int>(m_lineStyle);

		bool changed = false;

		if (min >= frequency_min && min <= frequency_max) {
			changed |= (this->m_freqMin[style_idx] != min);
		}
		else {
			min = this->m_freqMin[style_idx];
		}

		if (max >= frequency_min && max <= frequency_max) {
			changed |= (this->m_freqMax[style_idx] != max);
		}
		else {
			max = this->m_freqMax[style_idx];
		}

		if (changed) {
			switch (style)
			{
			case ws2812_style::spectrum_simple:
			case ws2812_style::spectrum_green_red_bars:
			case ws2812_style::spectrum_fire_lines:
				// common values for spectrum
				SetFrequencyMinMax(ws2812_style::spectrum_simple, min, max);
				SetFrequencyMinMax(ws2812_style::spectrum_green_red_bars, min, max);
				SetFrequencyMinMax(ws2812_style::spectrum_fire_lines, min, max);
				break;

			case ws2812_style::spectrogram_horizontal:
			case ws2812_style::spectrogram_vertical:
				// common values for spectrogram
				SetFrequencyMinMax(ws2812_style::spectrogram_horizontal, min, max);
				SetFrequencyMinMax(ws2812_style::spectrogram_vertical, min, max);
				break;

			case ws2812_style::oscilloscope_yt:
			case ws2812_style::oscilloscope_xy:
			case ws2812_style::oscillogram_horizontal:
			case ws2812_style::oscillogram_vertical:
				// common values for oscilloscope and oscillogram
				SetFrequencyMinMax(ws2812_style::oscilloscope_yt, min, max);
				SetFrequencyMinMax(ws2812_style::oscilloscope_xy, min, max);
				SetFrequencyMinMax(ws2812_style::oscillogram_horizontal, min, max);
				SetFrequencyMinMax(ws2812_style::oscillogram_vertical, min, max);
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

void ws2812::GetFrequencyMinMax(int &min, int &max)
{
	const unsigned int	style_idx = static_cast<unsigned int>(this->m_lineStyle);

	min = this->m_freqMin[style_idx];
	max = this->m_freqMax[style_idx];
}

bool GetFrequencyMinMax(int &min, int &max)
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

		ws2812_global->GetLineStyle(style);
		ws2812_global->GetFrequencyMinMax(min, max);

		switch (static_cast<ws2812_style>(style))
		{
		default:
		case ws2812_style::spectrum_simple:
		case ws2812_style::spectrum_green_red_bars:
		case ws2812_style::spectrum_fire_lines:
			SetCfgSpectrumFrequencyMinMax(min, max);
			break;

		case ws2812_style::spectrogram_horizontal:
		case ws2812_style::spectrogram_vertical:
			SetCfgSpectrogramFrequencyMinMax(min, max);
			break;

		case ws2812_style::oscilloscope_yt:
		case ws2812_style::oscilloscope_xy:
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
	m_ledNo = m_rows * m_columns;

	// start byte + RGBW for each LED
	m_bufferSize = 1 + 4 * m_ledNo;

	m_outputBuffer.resize(m_bufferSize);
	m_imageBuffer.resize(m_ledNo);
	m_persistenceBuffer.resize(m_ledNo);
	m_counterBuffer.resize(m_ledNo);
	m_indexLut.resize(m_ledNo);
	m_colorTab.resize(m_colorNo);

	if (m_ledSof.size() == 0) {
		m_ledSof.resize(1);
		m_ledSof.at(0) = sof_def;
	}

	unsigned int styleNo = static_cast<unsigned int>(ws2812_style::eNo);
	this->m_freqMin.resize(styleNo);
	this->m_freqMax.resize(styleNo);
	this->m_amplMin.resize(styleNo);
	this->m_amplMax.resize(styleNo);

	return true;
}

void ws2812::FreeBuffers()
{
	m_outputBuffer.resize(0);
	m_imageBuffer.resize(0);
	m_persistenceBuffer.resize(0);
	m_counterBuffer.resize(0);
	m_indexLut.resize(0);
	m_colorTab.resize(0);
}

void ws2812::ClearOutputBuffer(void)
{
	for (auto &i : m_outputBuffer)
		i = 0;
}

void ws2812::ClearImageBuffer(void)
{
	for (auto &i : m_imageBuffer)
		i = 0;
}

void ws2812::ClearPersistence(void)
{
	for (auto &i : m_persistenceBuffer)
		i = 0;
}

void ws2812::ClearCounterBuffer(void)
{
	for (auto &i : m_counterBuffer)
		i = 0;
}


void ws2812::InitAmplitudeMinMax()
{
	bool changed = false;
	int min = amplitude_min, max = amplitude_max;

	// common limits for all spectrum modes
	GetCfgSpectrumAmplitudeMinMax(min, max);

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

	SetAmplitudeMinMax(ws2812_style::spectrum_simple, min, max);
	SetAmplitudeMinMax(ws2812_style::spectrum_green_red_bars, min, max);
	SetAmplitudeMinMax(ws2812_style::spectrum_fire_lines, min, max);

	// common limits for all spectrogram modes
	GetCfgSpectrogramAmplitudeMinMax(min, max);

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

	SetAmplitudeMinMax(ws2812_style::spectrogram_horizontal, min, max);
	SetAmplitudeMinMax(ws2812_style::spectrogram_vertical, min, max);

	min = (offset_oscilloscope_max - offset_oscilloscope_max) / 2;
	max = gain_oscilloscope_min;

	// oscilloscope
	GetCfgOscilloscopeOffsetAmplitude(min, max);

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

	SetAmplitudeMinMax(ws2812_style::oscilloscope_yt, min, max);
	SetAmplitudeMinMax(ws2812_style::oscilloscope_xy, min, max);
	SetAmplitudeMinMax(ws2812_style::oscillogram_horizontal, min, max);
	SetAmplitudeMinMax(ws2812_style::oscillogram_vertical, min, max);
}

void ws2812::InitFrequencyMinMax()
{
	bool changed = false;
	int min = frequency_min, max = frequency_max;

	// common limits for all spectrum modes
	GetCfgSpectrumFrequencyMinMax(min, max);

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

	SetFrequencyMinMax(ws2812_style::spectrum_simple, min, max);
	SetFrequencyMinMax(ws2812_style::spectrum_green_red_bars, min, max);
	SetFrequencyMinMax(ws2812_style::spectrum_fire_lines, min, max);

	// common limits for all spectrogram modes
	GetCfgSpectrogramFrequencyMinMax(min, max);

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

	SetFrequencyMinMax(ws2812_style::spectrogram_horizontal, min, max);
	SetFrequencyMinMax(ws2812_style::spectrogram_vertical, min, max);


	// oscilloscope
	SetFrequencyMinMax(ws2812_style::oscilloscope_yt, min, max);
	SetFrequencyMinMax(ws2812_style::oscilloscope_xy, min, max);
	SetFrequencyMinMax(ws2812_style::oscillogram_horizontal, min, max);
	SetFrequencyMinMax(ws2812_style::oscillogram_vertical, min, max);
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

	m_hComm = INVALID_HANDLE_VALUE;
	m_hTimer = INVALID_HANDLE_VALUE;

	m_ledNo = 0;
	m_bufferSize = 0;
	m_outputSize = 0;

	m_timerStarted = false;
	m_timerActive = false;
	m_initDone = false;

	//	if (visStream == nullptr) {
			// ask the global visualisation manager to create a stream for us
	static_api_ptr_t<visualisation_manager>()->create_stream(visStream, visualisation_manager::KStreamFlagNewFFT);
	//	}
		// I probably should test this
	if (visStream != nullptr) {
		// mono is preferred, unless you want to use two displays ;-)
		visStream->set_channel_mode(visualisation_stream_v2::channel_mode_mono);

		// read configuration values
		m_rows = GetCfgMatrixRows();
		if (m_rows < rows_min || m_rows > rows_max)
			m_rows = rows_def;

		m_columns = GetCfgMatrixCols();
		if (m_columns < columns_min || m_columns > columns_max)
			m_columns = columns_def;

		// allocate output buffer
		m_initDone = AllocateBuffers();

		m_comPort = GetCfgComPort();
		if (m_comPort < port_min || m_comPort > port_max)
			m_comPort = port_def;

		tmp = GetCfgComBaudrate();
		if (tmp >= static_cast<unsigned int>(ws2812_baudrate::eNo))
			m_comBaudrate = ws2812_baudrate::e115200;
		else
			m_comBaudrate = static_cast<ws2812_baudrate>(tmp);

		m_timerInterval = GetCfgUpdateInterval();
		if (m_timerInterval < 50 || m_timerInterval > 1000)
			m_timerInterval = 250;

		m_brightness = GetCfgBrightness();
		if (m_brightness < 1 || m_brightness > 100)
			m_brightness = 25;

		SetCurrentLimit(GetCfgCurrentLimit());

		if (m_initDone) {
			const char *colors = nullptr;

			tmp = GetCfgLineStyle();
			if (tmp >= static_cast<unsigned int>(ws2812_style::eNo))
				tmp = static_cast<unsigned int>(ws2812_style::spectrum_simple);

			SetStyle(static_cast<ws2812_style>(tmp));

			m_logFrequency = GetCfgLogFrequency() != 0;
			m_logAmplitude = GetCfgLogAmplitude() != 0;
			m_peakValues = GetCfgPeakValues() != 0;

			m_ledMode = GetLedMode(GetCfgStartLed(), GetCfgLedDirection());

			tmp = GetCfgLedColors();
			if (tmp >= static_cast<unsigned int>(ws2812_led_colors::eNo))
				tmp = static_cast<unsigned int>(ws2812_led_colors::eGRB);
			SetLedColors(static_cast<ws2812_led_colors>(tmp));

			SetLedStripeSof(GetCfgLedStripeSof());

			InitIndexLut();

			m_timerStartDelay = 500;

			// fast FFT
			m_fftSize = 8 * 1024;

			// oscilloscope display time
			m_audioLength = 10 * 1e-3;

			// init color tab
			switch (m_lineStyle)
			{
			case ws2812_style::spectrum_simple:			colors = GetCfgSpectrumColors();		break;
			case ws2812_style::spectrum_green_red_bars:	colors = GetCfgSpectrumBarColors();		break;
			case ws2812_style::spectrum_fire_lines:		colors = GetCfgSpectrumFireColors();	break;
			case ws2812_style::spectrogram_horizontal:	colors = GetCfgSpectrogramColors();		break;
			case ws2812_style::spectrogram_vertical:	colors = GetCfgSpectrogramColors();		break;
			case ws2812_style::oscilloscope_yt:			colors = GetCfgOscilloscopeColors();	break;
			case ws2812_style::oscilloscope_xy:			colors = GetCfgOscilloscopeColors();	break;
			case ws2812_style::oscillogram_horizontal:	colors = GetCfgOscilloscopeColors();	break;
			case ws2812_style::oscillogram_vertical:	colors = GetCfgOscilloscopeColors();	break;
			}
			if (colors)
				InitColorTab(colors);

			InitAmplitudeMinMax();
			InitFrequencyMinMax();
		}
	}
}

#if 0
ws2812::ws2812(unsigned int rows, unsigned int cols, unsigned int port, ws2812_baudrate baudrate, unsigned int interval, ws2812_style style)
{
	m_hComm = INVALID_HANDLE_VALUE;
	m_hTimer = INVALID_HANDLE_VALUE;

	m_ledNo = 0;
	m_bufferSize = 0;
	m_outputSize = 0;

	m_timerStarted = false;
	m_timerActive = false;
	m_initDone = false;

	//	if (visStream == nullptr) {
			// ask the global visualisation manager to create a stream for us
	static_api_ptr_t<visualisation_manager>()->create_stream(visStream, visualisation_manager::KStreamFlagNewFFT);
	//	}
		// I probably should test this
	if (visStream != nullptr) {
		// mono is preferred, unless you want to use two displays ;-)
		visStream->set_channel_mode(visualisation_stream_v2::channel_mode_mono);

		// read configuration values
		if (rows >= rows_min && rows <= rows_max)
			this->m_rows = rows;
		else
			this->m_rows = rows_def;

		if (cols >= columns_min && cols <= columns_max)
			this->m_columns = cols;
		else
			this->m_columns = columns_def;

		// allocate output buffer
		m_initDone = AllocateBuffers();

		if (port >= port_min && port < port_max)
			this->m_comPort = port;
		else
			this->m_comPort = port_def;

		//if (baudrate < ws2812_baudrate_no)
		this->m_comBaudrate = baudrate;
		//else
		//	this->m_comBaudrate = ws2812_baudrate_115200;

		if (interval >= timerInterval_min && interval <= timerInterval_max)
			this->m_timerInterval = interval;
		else
			this->m_timerInterval = timerInterval_def;

		this->m_brightness = brightness_def;

		if (m_initDone) {
			const char *colors = NULL;

			//if (style >= 0 && style < ws2812_line_style_no)
			//	style = ws2812_style::spectrum_simple;

			SetStyle(style);

			m_logFrequency = true;
			m_logAmplitude = true;
			m_peakValues = true;

			m_ledMode = GetLedMode(0, 0);
			SetLedColors(ws2812_led_colors::eGRB);

			InitIndexLut();

			m_timerStartDelay = 500;

			// fast FFT
			m_fftSize = 8 * 1024;

			// oscilloscope display time
			m_audioLength = 10 * 1e-3;

			// init color tab
			switch (m_lineStyle)
			{
			case ws2812_style::spectrum_simple:			colors = GetCfgSpectrumColors();		break;
			case ws2812_style::spectrum_green_red_bars:	colors = GetCfgSpectrumBarColors();		break;
			case ws2812_style::spectrum_fire_lines:		colors = GetCfgSpectrumFireColors();	break;
			case ws2812_style::spectrogram_horizontal:	colors = GetCfgSpectrogramColors();		break;
			case ws2812_style::spectrogram_vertical:	colors = GetCfgSpectrogramColors();		break;
			case ws2812_style::oscilloscope_yt:			colors = GetCfgOscilloscopeColors();	break;
			case ws2812_style::oscilloscope_xy:			colors = GetCfgOscilloscopeColors();	break;
			case ws2812_style::oscillogram_horizontal:	colors = GetCfgOscilloscopeColors();	break;
			case ws2812_style::oscillogram_vertical:	colors = GetCfgOscilloscopeColors();	break;
			}
			InitColorTab(colors);

			InitAmplitudeMinMax();
			InitFrequencyMinMax();
		}
	}
}
#endif

ws2812::~ws2812()
{
	m_initDone = false;

	// how to delete the stream?
	visStream.release();

	// kill the timer
	StopTimer();

	// close the COM port
	ClosePort();

	FreeBuffers();
}

void InitOutput()
{
	if (!ws2812_global) {
		ws2812_global = std::make_unique<ws2812>();
	}

	if (ws2812_global) {

	}
}

void DeinitOutput()
{
	if (ws2812_global) {
		ws2812_global.release();
	}
}
