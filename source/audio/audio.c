
/* Copyright (c) 2024 ALIF SEMICONDUCTOR

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ALIF SEMICONDUCTOR nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission.
   *
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   ---------------------------------------------------------------------------*/


#include <stdio.h>
#include <string.h>
#include <stdatomic.h>

#include "Driver_SAI.h"
#include "audio.h"
#include "wm8904_driver.h"
#include "minimp3.h"

#include "RTE_Components.h"
#include CMSIS_device_header

/* I2S_Driver */
#define I2S_DAC 2                    /* DAC I2S Controller 2 */

/* Callback events */
#define DAC_SEND_COMPLETE_EVENT    (1U << 0)

#define SAMPLE_RATE 48000
#define MAX_SAMPLES_PER_FRAME 2304

// Half a second buffers for playback and decoding
#define PCM_BUFFER_LEN (SAMPLE_RATE)
static uint32_t wlen = 16;

/* I2S_DAC Driver */
extern ARM_DRIVER_SAI ARM_Driver_SAI_(I2S_DAC);
/* Driver Instance */
static ARM_DRIVER_SAI *i2s_dac = &ARM_Driver_SAI_(I2S_DAC);

static mp3dec_t mp3d;
static mp3dec_frame_info_t info;

// Status info while streaming / processing audio file
static atomic_bool sending = false;     // streaming audio file
static atomic_bool work_ready = false;  // working buffer filled with pcm data
static atomic_bool eof = false;         // end of file reached for the audio file
static atomic_bool paused = false;
static atomic_bool cancelling = false;

// Processed MP3 audio file, length and current decoding position
static uint32_t audio_file_decoding_position;
static unsigned char* audio_file;
static uint32_t audio_file_len;

// Decoded PCM data storage and amount
static mp3d_sample_t pcm1[PCM_BUFFER_LEN];
static mp3d_sample_t pcm2[PCM_BUFFER_LEN];
static uint32_t pcm_samples;            // number of pcm data samples ready in the pcm buffer (one sample is for single channel so stereo = 2 samples)
static mp3d_sample_t* pcm;              // pointer to the buffer where decoding is done, the other buffer is being sent via I2S

// Callback to signal when audio file has ended playing back
static audio_end_cb audio_callback;

// internal status
#define AUDIO_STATUS_SUCCESSFULLY_CONTINUING 0xFF

// internal functions
static int32_t start_transmit(void);
static void start_transmit_report_ending(void);
static void DAC_Callback(uint32_t event);
static bool decode_next(void);

static void DAC_Callback(uint32_t event)
{
    if (event != ARM_SAI_EVENT_SEND_COMPLETE) {
        printf("DAC_Callback: %" PRIu32 "\n", event);
    }
    if(event & ARM_SAI_EVENT_SEND_COMPLETE)
    {
        if(cancelling)
        {
            cancelling = false;
            return;
        }

        if(!paused) {
            start_transmit_report_ending();
        }
    }
}

static void start_transmit_report_ending(void)
{
    int32_t err = start_transmit();
    if(err != AUDIO_STATUS_SUCCESSFULLY_CONTINUING)
    {
        audio_callback(err);
    }
}

int32_t audio_init(audio_end_cb audio_cb)
{
    ARM_DRIVER_VERSION   version;
    ARM_SAI_CAPABILITIES cap;
    int32_t              status;

    /* Verify the I2S API version for compatibility */
    version = i2s_dac->GetVersion();
    printf("I2S API version = %d\n", version.api);

    if (audio_cb == NULL)
        return ARM_DRIVER_ERROR_PARAMETER;

    audio_callback = audio_cb;

    /* Verify if I2S protocol is supported */
    cap = i2s_dac->GetCapabilities();
    if(!cap.protocol_i2s)
    {
        printf("I2S is not supported\n");
        return -1;
    }

    /* Initializes I2S0 interface */
    status = i2s_dac->Initialize(DAC_Callback);
    if(status)
    {
        printf("DAC Init failed status = %ld\n", status);
        return -1;
    }

    /* Enable the power for I2S0 */
    status = i2s_dac->PowerControl(ARM_POWER_FULL);
    if(status)
    {
        printf("DAC Power Failed status = %ld\n", status);
        return -1;
    }

    /* configure I2S0 Transmitter to Asynchronous Master */
    status = i2s_dac->Control(ARM_SAI_CONFIGURE_TX |
                                ARM_SAI_MODE_MASTER  |
                                ARM_SAI_ASYNCHRONOUS |
                                ARM_SAI_PROTOCOL_I2S |
                                ARM_SAI_DATA_SIZE(wlen), wlen*2, SAMPLE_RATE);
    if(status)
    {
        printf("DAC Control status = %ld\n", status);
        return -1;
    }

    /* enable Transmitter */
    status = i2s_dac->Control(ARM_SAI_CONTROL_TX, 1, 0);
    if(status)
    {
        printf("DAC TX status = %ld\n", status);
        return -1;
    }

    status = WM8904_Codec_Init();

    mp3dec_init(&mp3d);

    return status;
}

static int32_t start_transmit(void)
{
    if(!work_ready) {
        if(!eof) {
            printf("MP3 decoding UNDERFLOW!\n");
            return AUDIO_END_STATUS_STREAM_UNDERFLOW;
        }
        sending = false;
        return AUDIO_END_STATUS_ENDED;
    }

    int32_t ret = i2s_dac->Send(pcm, pcm_samples);
    if(ret != ARM_DRIVER_OK)
    {
        // sending failed for some reason, abort.
        sending = false;
        eof = true;
        work_ready = false;
        printf("I2S send failed: %" PRId32 "\n", ret);
        return AUDIO_END_STATUS_DRIVER_ERROR;
    }

    // Swap working buffer and reset counter and work status
    if(pcm == &pcm1[0])
        pcm = &pcm2[0];
    else
        pcm = &pcm1[0];
    pcm_samples = 0;
    work_ready = false;
    return AUDIO_STATUS_SUCCESSFULLY_CONTINUING;
}

static bool decode_next(void)
{
    if(work_ready || eof)
        return true;
    int samples = mp3dec_decode_frame(&mp3d, audio_file + audio_file_decoding_position, audio_file_len - audio_file_decoding_position, pcm + pcm_samples, &info);
    audio_file_decoding_position += info.frame_bytes;
    pcm_samples += (2 * samples);
    if(audio_file_decoding_position == audio_file_len || pcm_samples + MAX_SAMPLES_PER_FRAME > PCM_BUFFER_LEN) {
        // either the end of file was reached or the pcm buffer is full
        work_ready = true;
    }
    if(audio_file_decoding_position == audio_file_len) {
        // end of file
        eof = true;
    }

    return work_ready;
}

int32_t audio_start_transmit(void)
{
    if(sending)
    {
        cancelling = true;
        while(cancelling) {
            __WFE();
        }
    }

    sending = true;
    // TODO: these could come from outside the function
    // but now we just expect them to be compiled in the binary somewhere
    extern uint32_t audio_sample_mp3_len;
    extern unsigned char* audio_sample;
    audio_file = audio_sample;
    audio_file_len = audio_sample_mp3_len;
    audio_file_decoding_position = 0;

    // reset data on start of new mp3 file playback
    pcm_samples = 0;
    work_ready = false;
    eof = false;
    paused = false;
    cancelling = false;
    pcm = &pcm1[0];

    while(!decode_next());

    /* Transmit the samples */
    return start_transmit();
}

bool audio_process_next(void)
{
    if(sending) {
        return decode_next();
    }
    return true;
}

bool audio_process_nexts(int n)
{
    if(sending)
    {
        bool ready = false;
        for(int i = 0; i < n; i++) {
            ready = decode_next();
            if(ready) {
                return true;
            }
        }
        return ready;
    }
    return true;
}

void audio_pause(void)
{
    // set to paused IF sending
    paused = sending;
}

void audio_resume(void)
{
    if(paused) {
        paused = false;
        while(!decode_next());
        start_transmit_report_ending();
    }
    else {
        audio_start_transmit();
    }
}
