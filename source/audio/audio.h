#ifndef AUDIO_H
#define AUDIO_H

#include <inttypes.h>
#include <stdbool.h>

int32_t audio_init(void);
int32_t audio_start_transmit(void);
bool transmit_done(void);


#endif // #ifndef AUDIO_H
