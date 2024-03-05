#include "board.h"
#include "fault_handler.h"
#include "lv_port_disp.h"
#include "lvgl.h"
#include "lv_demo_music.h"
#include "lv_demo_music_main.h"
#include "uart_tracelib.h"
#include "RTE_Components.h"
#include "audio.h"
#include CMSIS_device_header
#include "mpu.h"

#include <stdio.h>

// from retarget.c
void clk_init();
void flush_uart();

#define TICKS_PER_SECOND    1000

void audio_ended(uint32_t error)
{
    (void)error;
    // at the moment, just make sure GUI doesn't show audio progressing anymore
    _lv_demo_music_playback_stopped();
}


int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    mpu_init(); // pull mpu in
    clk_init(); // pull retarget in
    enable_cgu_clk100m();
    sys_busy_loop_init();
    BOARD_Pinmux_Init(); // initialize board
    tracelib_init(0, 0); // initialize tracelib
    fault_dump_enable(true); // initialize fault handler
    printf("Starting app\n");

    SysTick_Config(SystemCoreClock/TICKS_PER_SECOND);

    uint32_t ret = lv_port_disp_init();
    if (ret) {
        printf("lv_port_disp_init: %" PRIu32 "\n", ret);
        while(1);
    }

    int32_t audio_ret = audio_init(audio_ended);
    if(audio_ret)
    {
        printf("audio_init: %" PRId32 "\n", audio_ret);
        while(1);
    }

    printf("Initialized.\n");

    lv_demo_music();

    while (1)
    {
        audio_process_nexts(10);
        lv_timer_handler_run_in_period(20);
        flush_uart();
    }

    return 0;
}
