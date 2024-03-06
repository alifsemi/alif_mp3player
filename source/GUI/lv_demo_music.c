/**
 * @file lv_demo_music.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_demo_music.h"

#if LV_USE_DEMO_MUSIC

#include "lv_demo_music_main.h"
#include "lv_demo_music_list.h"
#include "../../src/core/lv_global.h"

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t * ctrl;
static lv_obj_t * list;

#if LV_USE_PERF_MONITOR || LV_DEMO_MUSIC_AUTO_PLAY
    #define sysmon_perf LV_GLOBAL_DEFAULT()->sysmon_perf
#endif

extern const char* audio_sample_artist;
extern const char* audio_sample_title;
extern const char* audio_sample_genre;
extern const uint32_t audio_sample_len_seconds;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_demo_music(void)
{
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x343247), 0);

    list = _lv_demo_music_list_create(lv_screen_active());
    ctrl = _lv_demo_music_main_create(lv_screen_active());
}

const char * _lv_demo_music_get_title(uint32_t track_id)
{
    if(track_id > 0) return NULL;
    return audio_sample_title;
}

const char * _lv_demo_music_get_artist(uint32_t track_id)
{
    if(track_id > 0) return NULL;
    return audio_sample_artist;
}

const char * _lv_demo_music_get_genre(uint32_t track_id)
{
    if(track_id > 0) return NULL;
    return audio_sample_genre;
}

uint32_t _lv_demo_music_get_track_length(uint32_t track_id)
{
    if(track_id > 0) return 0;
    return audio_sample_len_seconds;
}

#endif /*LV_USE_DEMO_MUSIC*/
