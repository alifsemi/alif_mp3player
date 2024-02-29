#include "board.h"
#include "fault_handler.h"
#include "lv_port_disp.h"
#include "lvgl.h"
#include "demos/widgets/lv_demo_widgets.h"
#include "uart_tracelib.h"
#include "RTE_Components.h"
#include "audio.h"
#include CMSIS_device_header

#include <stdio.h>

// from retarget.c
void clk_init();
void flush_uart();

#define TICKS_PER_SECOND    1000

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    clk_init(); // pull retarget in
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

    int32_t audio_ret = audio_init();
    if(audio_ret)
    {
        printf("audio_init: %" PRId32 "\n", audio_ret);
        while(1);
    }

    printf("Initialized.\n");

    lv_demo_music();
    audio_start_transmit();

    while (1)
    {
        lv_task_handler();
        flush_uart();
    }

    return 0;
}
