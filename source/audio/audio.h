#ifndef AUDIO_H
#define AUDIO_H

#include <inttypes.h>
#include <stdbool.h>

// The played-back file has ended normally
#define AUDIO_END_STATUS_ENDED 0
// The playback ended due to underflow in MP3 decoding (call audio_process_next more often!)
#define AUDIO_END_STATUS_STREAM_UNDERFLOW 1
// The playback ended due to I2S driver error
#define AUDIO_END_STATUS_DRIVER_ERROR 2

typedef void (*audio_end_cb)(uint32_t status);

// Initialize audio pipeline - I2C, I2S, WM8904 and MP3 decoder
int32_t audio_init(audio_end_cb audio_cb);

// Start playing an MP3 file
int32_t audio_start_transmit(void);

// Process next frame of MP3 data
// returns true if full playback buffer is now decoded
bool audio_process_next(void);

// Process n frames of MP3 or until playback buffer is full
// returns true if full playback buffer is now decoded
bool audio_process_nexts(int n);

// Pause the playback
void audio_pause(void);

// Resume the playback
void audio_resume(void);

#endif // #ifndef AUDIO_H
