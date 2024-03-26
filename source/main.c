#include "board.h"
#include "fault_handler.h"
#include "lv_port_disp.h"
#include "lvgl.h"
#include "lv_demo_music.h"
#include "lv_demo_music_main.h"
#include "uart_tracelib.h"
#include "RTE_Components.h"
#include "audio.h"
#include "se_services_port.h"
#include "power.h"

#include CMSIS_device_header

#include <stdio.h>

// from retarget.c
void clk_init();
void flush_uart();

#define TICKS_PER_SECOND    1000

static int init_clocks()
{
    uint32_t  service_error_code;
    uint32_t  error_code;
    

     /* Initialize the SE services */
    se_services_port_init();

    /* Enable MIPI Clocks */
    error_code = SERVICES_clocks_enable_clock(se_services_s_handle, CLKEN_CLK_100M, true, &service_error_code);
    if(error_code != SERVICES_REQ_SUCCESS)
    {
        printf("SE: MIPI 100MHz clock enable = %" PRIu32 "\n", error_code);
        return error_code;
    }

    error_code = SERVICES_clocks_enable_clock(se_services_s_handle, CLKEN_HFOSC, true, &service_error_code);
    if(error_code != SERVICES_REQ_SUCCESS)
    {
        printf("SE: MIPI 38.4Mhz(HFOSC) clock enable = %" PRIu32 "\n", error_code);
        return error_code;
    }
#if 0 // SERVICES_set_run_cfg breaks TCM/ETR so don't use it for now.
    run_profile_t runp = {0};

    /* Get the current run configuration from SE */
    error_code = SERVICES_get_run_cfg(se_services_s_handle,
                                      &runp,
                                      &service_error_code);
    if(error_code)
    {
        printf("\r\nSE: get_run_cfg error = %" PRIu32 "\n", error_code);
        return error_code;
    }

    // Only MRAM and SRAM0 are used (besides TCMs)
    runp.memory_blocks |= MRAM_MASK | SRAM0_MASK | SRAM1_MASK;
    runp.phy_pwr_gating |= MIPI_PLL_DPHY_MASK | MIPI_TX_DPHY_MASK | MIPI_RX_DPHY_MASK | LDO_PHY_MASK;
    runp.power_domains |= PD_VBAT_AON_MASK | PD_SRAM_CTRL_AON_MASK | PD_SSE700_AON_MASK | PD_RTSS_HE_MASK | PD_SRAMS_MASK | PD_SESS_MASK | PD_SYST_MASK | PD_RTSS_HP_MASK | PD_DBSS_MASK | PD2_APPS_MASK;

    /* Set the new run configuration */
    error_code = SERVICES_set_run_cfg(se_services_s_handle,
                                      &runp,
                                      &service_error_code);
    if(error_code)
    {
        printf("\r\nSE: set_run_cfg error = %" PRIu32 "\n", error_code);
        return error_code;
    }
#else
    enable_mipi_dphy_power();
    disable_mipi_dphy_isolation();
#endif
    return 0;
}

static void audio_ended(uint32_t error)
{
    (void)error;
    _lv_demo_music_playback_stopped();
    _lv_demo_music_album_next(true);
    audio_load_track();
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    clk_init(); // pull retarget in
    BOARD_Pinmux_Init(); // initialize board
    tracelib_init(0, 0); // initialize tracelib
    init_clocks();
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
    audio_load_track();

    while (1)
    {
        bool ready = audio_process_nexts(8);
        lv_timer_handler_run_in_period(20);
        flush_uart();
        if(ready) {
            __WFE();
        }
    }

    return 0;
}
