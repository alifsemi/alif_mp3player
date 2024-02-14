#include "board.h"
#include "uart_tracelib.h"
#include "fault_handler.h"

#include <stdio.h>

void clk_init();

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    clk_init(); // pull retarget in
    BOARD_Pinmux_Init(); // initialize board
    tracelib_init(0, 0); // initialize tracelib
    fault_dump_enable(true); // initialize fault handler

    while(1);
    return 0;
}
