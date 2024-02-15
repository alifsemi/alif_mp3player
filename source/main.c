#include "board.h"
#include "fault_handler.h"
#include "lv_port_disp.h"
#include "lvgl.h"
#include "demos/widgets/lv_demo_widgets.h"
#include "uart_tracelib.h"
#include "RTE_Components.h"
#include CMSIS_device_header

#include <stdio.h>

// from retarget.c
void clk_init();

#define TICKS_PER_SECOND    1000

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    clk_init(); // pull retarget in
    BOARD_Pinmux_Init(); // initialize board
    tracelib_init(0, 0); // initialize tracelib
    fault_dump_enable(true); // initialize fault handler
    printf("Starting app\n");

    SysTick_Config(SystemCoreClock/TICKS_PER_SECOND);

    uint32_t ret = lv_port_disp_init();
    if (ret) {
        printf("lv_port_disp_init: " PRIu32 "\n");
        while(1);
    }

    lv_demo_music();

    while (1)
    {
        lv_task_handler();
    }

    return 0;
}
