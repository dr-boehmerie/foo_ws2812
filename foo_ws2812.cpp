// foo_ws2812.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

service_ptr_t<visualisation_stream_v3> ws2812_stream;

HANDLE ws2812_hComm = INVALID_HANDLE_VALUE;
HANDLE ws2812_hTimer = INVALID_HANDLE_VALUE;

LPCWSTR const	ws2812_port_str = L"COM7";

BOOL		ws2812_init_done = false;

const int	ws2812_rows = 8;
const int	ws2812_columns = 30;

int			ws2812_brightness = 64;

DWORD		ws2812_timerStartDelay = 500;
DWORD		ws2812_timerInverval = 100;

BOOL		ws2812_timerStarted = false;

BOOL		ws2812_logFrequency = false;
BOOL		ws2812_peakValues = true;


void OutputTest(const audio_sample *psample, int samples, audio_sample peak, unsigned char *buffer, int bufferSize);
void OutputSpectrumBars(const audio_sample *psample, int samples, audio_sample peak, unsigned char *buffer, int rows, int cols);


// COM port handling taken from the MSDN
BOOL OpenPort(LPCWSTR gszPort)
{
	ws2812_hComm = CreateFile(gszPort,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		0);

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
	double	abstime;

	// get current track time
	if (ws2812_stream->get_absolute_time(abstime)) {
		audio_chunk_fast_impl	chunk;

		// FFT data
		if (ws2812_stream->get_spectrum_absolute(chunk, abstime, 16 * 1024)) {
			// number of channels, should be 1
			//	int channels = chunk.get_channel_count();
			// number of samples in the chunk, fft_size / 2
			int samples = chunk.get_sample_count();
			// peak sample value
			audio_sample peak = chunk.get_peak();

			// 240 LEDs, RGB for each, plus one start byte
			unsigned char buffer[1 + 3 * ws2812_rows * ws2812_columns];

			// convert samples
			const audio_sample *psample = chunk.get_data();
			if (psample != nullptr && samples > 0) {

				// a value of 1 is used as start byte
				// and must not occur in other bytes in the buffer
				buffer[0] = 1;

				OutputSpectrumBars(psample, samples, peak, &buffer[1], ws2812_rows, ws2812_columns);

				// send buffer
				WriteABuffer(buffer, sizeof buffer);

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

BOOL StartTimer()
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

BOOL StopTimer()
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

		// open the COM port
		if (OpenPort(ws2812_port_str)) {
			// init done :-)
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

void OutputSpectrumBars(const audio_sample *psample, int samples, audio_sample peak, unsigned char *buffer, int rows, int cols)
{
	int		bar, cnt, row;
	int		bar_cnt;
	int		samples_per_bar;
	int		sample_index;
	int		led_index;
	int		r, g, b;
	int		bright = ws2812_brightness;
	audio_sample	sum;
	audio_sample	sample;
	audio_sample	db_per_row;
	audio_sample	db_max;
	audio_sample	db_min;

	// vertical bars
	bar_cnt = cols;

	// limits
	db_max = -10;
	db_min = (audio_sample)(-10 * rows);
	// 10 db per row
	db_per_row = (db_max - db_min) / rows;

	// multiple samples added together for one bar
	samples_per_bar = samples / bar_cnt;
	if (samples_per_bar < 1) {
		bar_cnt = samples;
		samples_per_bar = 1;
	}

	// fixed peak value
	peak = (audio_sample)(1.0 * samples_per_bar);

	sample_index = 0;

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

				// calc ratio to peak, logarithmic scale: volts to db
				sample = 20 * log10(sum / peak);
			}
			else {
				// find peak value of samples per bar
				peak = 0;
				for (cnt = 0; cnt < samples_per_bar; cnt++) {
					sample = psample[sample_index + cnt];
					sample = fabs(sample);
					if (peak < sample)
						peak = sample;
				}

				// logarithmic scale: volts to db
				sample = 20 * log10(peak);
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

				tmp = log10(samples);
				tmp /= (double)bar_cnt;
				tmp *= (double)(bar + 1);
				tmp = pow(10, tmp);

				// count of samples to the next index
				samples_per_bar = (int)tmp - sample_index;

				// minimum sample count (trial and error, should depend somehow on the max fft frequency...)
				if (samples_per_bar < 5)
					samples_per_bar = 5;

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

				// calc ratio to peak, logarithmic scale: volts to db
				sample = 20 * log10(sum / peak);
			}
			else {
				// find peak value of samples per bar
				peak = 0;
				for (cnt = 0; cnt < samples_per_bar; cnt++) {
					sample = psample[sample_index + cnt];
					sample = fabs(sample);
					if (peak < sample)
						peak = sample;
				}

				// logarithmic scale: volts to db
				sample = 20 * log10(peak);
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
		for (row = 1; row <= rows; row++) {
			if (row > (int)sample) {
				// off
				r = g = b = 0;
			} else if (row == (int)sample) {
				// peak
				r = g = b = 255;
			} else {
				// between
				r = g = b = 255 / 4;
			}

			// apply brightness
			r = (r * bright) / 255;
			g = (g * bright) / 255;
			b = (b * bright) / 255;

			// all values < 2 are replaced by 0 (1 is reserved for start of block)
			if (r < 2)	r = 0;
			if (g < 2)	g = 0;
			if (b < 2)	b = 0;

			// led order: start is top left, direction is left to right for all rows
			led_index = (rows - row) * cols + bar;

			if (led_index < (rows * cols)) {
				// write colors to buffer
				// color ouput order: GRB
				buffer[3 * led_index + 0] = (unsigned char)g;
				buffer[3 * led_index + 1] = (unsigned char)r;
				buffer[3 * led_index + 2] = (unsigned char)b;
			}
		}
	}

	// clear the rest of the buffer
//	for (; i < 3 * rows * cols; i++)
//		buffer[i] = 0;
}

void StartOutput()
{
	try {
		if (ws2812_init_done) {
#if 0
			double	abstime;

			// get current track time
			if (ws2812_stream->get_absolute_time(abstime)) {
				audio_chunk_fast_impl	chunk;

				// FFT data
				if (ws2812_stream->get_spectrum_absolute(chunk, abstime, 16 * 1024)) {
					// number of channels, should be 1
				//	int channels = chunk.get_channel_count();
					// number of samples in the chunk, fft_size / 2
					int samples = chunk.get_sample_count();
					// peak sample value
					audio_sample peak = chunk.get_peak();

					// 240 LEDs, RGB for each, plus one start byte
					unsigned char buffer[1 + 3 * ws2812_rows * ws2812_columns];

					// convert samples
					const audio_sample *psample = chunk.get_data();
					if (psample != nullptr && samples > 0) {

						// a value of 1 is used as start byte
						// and must not occur in other bytes in the buffer
						buffer[0] = 1;

						if (0) {
							OutputTest(psample, samples, peak, &buffer[1], sizeof(buffer) - 1);
						} else {
							OutputSpectrumBars(psample, samples, peak, &buffer[1], ws2812_rows, ws2812_columns);
						}

						// open the port
					//	if (OpenPort(ws2812_port_str)) {
							// send buffer
							WriteABuffer(buffer, sizeof buffer);
							// close the port
					//		ClosePort();
					//	}
					}
				}
				else {
					// playback hasn't started yet...
				}
			}
#else
			if (ws2812_timerStarted == false)
				StartTimer();
			else
				StopTimer();
#endif
		}
	}
	catch (std::exception const & e) {
		popup_message::g_complain("WS2812 Output exception", e);
	}
}




