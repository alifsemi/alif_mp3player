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

static const char * title_list[] = {
    "Becoming",
    "Need a Better Future",
};

static const char * artist_list[] = {
    "Pantera",
    "My True Name",
};

static const char * genre_list[] = {
    "Groove Metal - 1994",
    "Drum'n bass - 2016",
};

static const uint32_t time_list[] = {
    1 * 60 + 59,
    1 * 60 + 59,
};

#if LV_USE_PERF_MONITOR || LV_DEMO_MUSIC_AUTO_PLAY
    #define sysmon_perf LV_GLOBAL_DEFAULT()->sysmon_perf
#endif

/**********************
 *      MACROS
 **********************/

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
    if(track_id >= sizeof(title_list) / sizeof(title_list[0])) return NULL;
    return title_list[track_id];
}

const char * _lv_demo_music_get_artist(uint32_t track_id)
{
    if(track_id >= sizeof(artist_list) / sizeof(artist_list[0])) return NULL;
    return artist_list[track_id];
}

const char * _lv_demo_music_get_genre(uint32_t track_id)
{
    if(track_id >= sizeof(genre_list) / sizeof(genre_list[0])) return NULL;
    return genre_list[track_id];
}

uint32_t _lv_demo_music_get_track_length(uint32_t track_id)
{
    if(track_id >= sizeof(time_list) / sizeof(time_list[0])) return 0;
    return time_list[track_id];
}

#endif /*LV_USE_DEMO_MUSIC*/
