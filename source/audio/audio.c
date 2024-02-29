
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
#include "audio_sample.h"
#include "wm8904_driver.h"

/* 1 to send the data stream continously, 0 to send data only once */
#define REPEAT_TX 1

/* Return flags */
#define ERROR  -1
#define SUCCESS 0

/* I2S_Driver */
#define I2S_DAC 2                    /* DAC I2S Controller 2 */

/* Callback events */
#define DAC_SEND_COMPLETE_EVENT    (1U << 0)

static uint32_t wlen = 24;
static uint32_t sampling_rate = 48000;

/* I2S_DAC Driver */
extern ARM_DRIVER_SAI ARM_Driver_SAI_(I2S_DAC);
/* Driver Instance */
static ARM_DRIVER_SAI *i2s_dac = &ARM_Driver_SAI_(I2S_DAC);

static const uint32_t sample_len = 572520 / 3;
static uint32_t sample[572520 / 3];


static atomic_bool sending = false;

void DAC_Callback(uint32_t event)
{
    if (event != ARM_SAI_EVENT_SEND_COMPLETE) {
        printf("DAC_Callback: %" PRIu32 "\n", event);
    }
    if(event & ARM_SAI_EVENT_SEND_COMPLETE)
    {
        sending = false;
    }
}

int32_t audio_init(void)
{
    ARM_DRIVER_VERSION   version;
    ARM_SAI_CAPABILITIES cap;
    int32_t              status;

    /* Verify the I2S API version for compatibility */
    version = i2s_dac->GetVersion();
    printf("I2S API version = %d\n", version.api);

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
                                ARM_SAI_DATA_SIZE(wlen), wlen*2, sampling_rate);
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

    for(uint32_t i = 0; i < sample_len; i++)
    {
        sample[i] = ((uint32_t)(go_stop_stereo_raw[i*3])) + ((uint32_t)(go_stop_stereo_raw[i*3+1]) << 8) + (go_stop_stereo_raw[i*3+2] << 16);
    }

    return status;
}

int32_t audio_start_transmit(void)
{
    if(sending)
        return -1;

    sending = true;

    /* Transmit the samples */
    int32_t status = i2s_dac->Send(sample, sample_len);
    if(status)
    {
        printf("DAC Send status = %ld\n", status);
        sending = false;
        return -1;
    }
    printf("transmit started\n");
    return 0;
}

bool transmit_done(void)
{
    return !sending;
}
