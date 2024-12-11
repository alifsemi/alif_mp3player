/* This file was ported to work on Alif Semiconductor Ensemble family of devices. */

/* Copyright (C) 2024 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include "lv_port.h"
#include "lvgl.h"
#include <stdbool.h>
#include <stdio.h>

#include "RTE_Components.h"
#include CMSIS_device_header
#include <RTE_Device.h>
#include "Driver_CDC200.h" // Display driver
#include "Driver_UTIMER.h"

#include "dave_cfg.h"
#include "dave_d0lib.h"
#include "dave_driver.h"
#include "lv_draw_dave2d_utils.h"

#if defined(LV_USE_OS) && (LV_USE_OS == LV_OS_FREERTOS)
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "event_groups.h"
#define USE_EVT_GROUP
#endif // LV_OS_FREERTOS

#include "pinconf.h"

#define I2C_TOUCH_ENABLE         1

/*********************
 *      DEFINES
 *********************/
#ifndef MY_DISP_HOR_RES
    // Replace the macro MY_DISP_HOR_RES with the actual screen width.
    #define MY_DISP_HOR_RES    (RTE_PANEL_HACTIVE_TIME)
#endif

#ifndef MY_DISP_VER_RES
    // Replace the macro MY_DISP_HOR_RES with the actual screen height.
    #define MY_DISP_VER_RES    (RTE_PANEL_VACTIVE_LINE)
#endif

#if ((LV_COLOR_DEPTH == 16) && (RTE_CDC200_PIXEL_FORMAT != 2)) || \
    ((LV_COLOR_DEPTH == 24) && (RTE_CDC200_PIXEL_FORMAT != 1)) || \
    ((LV_COLOR_DEPTH == 32) && (RTE_CDC200_PIXEL_FORMAT != 0))
#error "The LV_COLOR_DEPTH and RTE_CDC200_PIXEL_FORMAT must match."
#endif

#if defined(USE_EVT_GROUP)
#define EVENT_DISP_BUFFER_READY     ( 1 << 0 )
#define EVENT_DISP_BUFFER_CHANGED   ( 1 << 1 )
#endif

#if (D1_MEM_ALLOC == D1_MALLOC_D0LIB)
// D/AVE D0 heap address and size
#define D1_HEAP_SIZE	0x100000
#endif

// UTIMER for outputting pwm signal to backlight led
#define UTIMER_COUNTER_VALUE 400000

// Target pwr on period when dimmed
#define UTIMER_DIMMED_TARGET_VALUE 10000

// Steps to change the on period towards UTIMER_DIMMED_TARGET_VALUE
#define UTIMER_DIMMING_STEP 1000

/**********************
 *      TYPEDEFS
 **********************/

#pragma pack(1)
#if RTE_CDC200_PIXEL_FORMAT == 0    // ARGB8888
typedef lv_color32_t Pixel;
#elif RTE_CDC200_PIXEL_FORMAT == 1  // RGB888
typedef lv_color_t Pixel;
#elif RTE_CDC200_PIXEL_FORMAT == 2  // RGB565
typedef lv_color16_t Pixel;
#else
#error "CDC200 Unsupported color format"
#endif
#pragma pack()

/**********************
 *  STATIC PROTOTYPES
 **********************/
static uint32_t disp_init(void);

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

static void disp_flush_wait(lv_display_t * disp);

static void disp_callback(uint32_t event);

#if(I2C_TOUCH_ENABLE == 1)
static uint32_t hw_touch_init(void);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

#if (D1_MEM_ALLOC == D1_MALLOC_D0LIB)
static uint8_t __attribute__((section(".bss.disp_buff"))) d0_heap[D1_HEAP_SIZE];
#endif

static Pixel lcd_buffer_1[MY_DISP_VER_RES][MY_DISP_HOR_RES]
            __attribute__((section(".bss.disp_buff"))) = {0};
static Pixel lcd_buffer_2[MY_DISP_VER_RES][MY_DISP_HOR_RES]
            __attribute__((section(".bss.disp_buff"))) = {0};

extern ARM_DRIVER_CDC200 Driver_CDC200;
static ARM_DRIVER_CDC200 *CDCdrv = &Driver_CDC200;

static volatile bool disp_flush_enabled = true;

#if defined(USE_EVT_GROUP)
static EventGroupHandle_t dispEventGroupHandle = NULL;
#else
static volatile bool disp_buf_ready = true;
static volatile bool disp_buf_changed = false;
#endif

extern ARM_DRIVER_UTIMER DRIVER_UTIMER0;
static ARM_DRIVER_UTIMER* utimer = &DRIVER_UTIMER0;
static uint32_t target_cmp_value = UTIMER_COUNTER_VALUE;
static uint32_t current_cmp_value = UTIMER_COUNTER_VALUE;


void utimer_cb(uint8_t event)
{
    if(event != ARM_UTIMER_EVENT_OVER_FLOW && event != ARM_UTIMER_EVENT_COMPARE_B) {
        // shouldn't be getting any other callbacks so print them
        printf("utimer cb: %" PRIu8 "\n", event);
    }
    if (event == ARM_UTIMER_EVENT_OVER_FLOW && current_cmp_value > target_cmp_value) {
        current_cmp_value -= UTIMER_DIMMING_STEP;
        utimer->SetCount(ARM_UTIMER_CHANNEL4, ARM_UTIMER_COMPARE_B_BUF1, current_cmp_value);
    }
}

void lv_port_disp_off()
{
    utimer->Stop(ARM_UTIMER_CHANNEL4, true);
}

void lv_port_disp_dim()
{
    target_cmp_value = UTIMER_DIMMED_TARGET_VALUE;
}

void lv_port_disp_on()
{
    // switch to full immediately (callback needs implementation too if changed to gradual poweron)
    current_cmp_value = UTIMER_COUNTER_VALUE;
    target_cmp_value = UTIMER_COUNTER_VALUE;
    utimer->SetCount(ARM_UTIMER_CHANNEL4, ARM_UTIMER_COMPARE_B_BUF1, UTIMER_COUNTER_VALUE);
    utimer->Start(ARM_UTIMER_CHANNEL4);
}

#if(I2C_TOUCH_ENABLE == 1)

/*touch screen driver */
#include "Driver_Touch_Screen.h"

/* Touch screen driver instance */
extern ARM_DRIVER_TOUCH_SCREEN GT911;
static ARM_DRIVER_TOUCH_SCREEN *Drv_Touchscreen = &GT911;

/**
  \function     static void lv_touch_get(lv_indev_drv_t * drv, lv_indev_data_t * data)
  \brief        Check touch screen is pressed or not
  \param[in]    drv: pointer to LVGL driver
                data: data to LVGL driver
  \return       none
  */

static ARM_TOUCH_COORDINATES last;

static void lv_touch_get(lv_indev_t * indev, lv_indev_data_t * data)
{
    (void)indev;
    ARM_TOUCH_STATE status;
    Drv_Touchscreen->GetState(&status);

    if(status.numtouches)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = status.coordinates[0].x;
        data->point.y = status.coordinates[0].y;
        last.x = status.coordinates[0].x;
        last.y = status.coordinates[0].y;

    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
        data->point.x = last.x;
        data->point.y = last.y;
    }
}

/**
  \function	   static void hw_touch_init(void)
  \brief	   This hardware initialization of GT911 touch screen does:
                 - initialize i3c AND gpio2 port hardware pins
                 - initialize GT911 Touch screen driver.
  \param[in]   none
  \return      none
  */
static uint32_t hw_touch_init(void)
{
    int ret = 0;

    ret = Drv_Touchscreen->Initialize();
    if(ret != ARM_DRIVER_OK) {
        return 12;
    }

    ret = Drv_Touchscreen->PowerControl(ARM_POWER_FULL);
    if(ret != ARM_DRIVER_OK) {
        return 13;
    }

    return 0;
}
#endif

uint32_t lv_port_disp_init(void)
{
    #if defined(USE_EVT_GROUP)
    // Create event group to sync display states
    dispEventGroupHandle = xEventGroupCreate();
    #endif

    /* Display hardware initialization */
    uint32_t ret = disp_init();
    if (ret) {
        return ret;
    }

#if defined(LV_USE_OS) && (LV_USE_OS == LV_OS_FREERTOS)
    /*-------------------------
     * Set display interrupt priority to FreeRTOS kernel level
     * -----------------------*/
    NVIC_SetPriority(CDC_SCANLINE0_IRQ_IRQn, configKERNEL_INTERRUPT_PRIORITY);
#endif

#if (D1_MEM_ALLOC == D1_MALLOC_D0LIB)
    /*-------------------------
     * Initialize D/AVE D0 heap
     * -----------------------*/
    if (!d0_initheapmanager(d0_heap, sizeof(d0_heap), d0_mm_fixed_range,
                            NULL, 0, 0, 0, d0_ma_unified))
    {
        printf("\r\nError: Heap manager initialization failed\n");
        return 0xFFFF;
    }
#endif

    lv_init();
    lv_tick_set_cb(clock);

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_flush_wait_cb(disp, disp_flush_wait);

    /* Two buffers screen sized buffer for double buffering.
     * Both LV_DISPLAY_RENDER_MODE_DIRECT and LV_DISPLAY_RENDER_MODE_FULL works, see their comments*/
    lv_display_set_buffers(disp, lcd_buffer_1, lcd_buffer_2,
                           sizeof(lcd_buffer_1),
                           LV_DISPLAY_RENDER_MODE_DIRECT);

#if(I2C_TOUCH_ENABLE == 1)
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lv_touch_get);

    /* Touch screen hardware initialization */
    ret = hw_touch_init();
#endif
	
	return ret;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static uint32_t disp_init(void)
{
    // set display backlight to utimer to drive pwm signal towards the display
	pinconf_set(PORT_(BOARD_LCD_BACKLIGHT_GPIO_PORT), BOARD_LCD_BACKLIGHT_PIN_NO, PINMUX_ALTERNATE_FUNCTION_4, PADCTRL_OUTPUT_DRIVE_STRENGTH_4MA);

    /* Initialize CDC driver */
    int ret = CDCdrv->Initialize(disp_callback);
    if(ret != ARM_DRIVER_OK){
        printf("\r\n Error: CDC init failed\n");
        return 1;
    }

    /* Power control CDC */
    ret = CDCdrv->PowerControl(ARM_POWER_FULL);
    if(ret != ARM_DRIVER_OK){
        printf("\r\n Error: CDC Power up failed\n");
        return 2;
    }

    /* configure CDC controller */
    ret = CDCdrv->Control(CDC200_CONFIGURE_DISPLAY, (uint32_t)lcd_buffer_1);
    if(ret != ARM_DRIVER_OK){
        printf("\r\n Error: CDC controller configuration failed\n");
        return 3;
    }

    /* Enable CDC SCANLINE0 event */
    ret = CDCdrv->Control(CDC200_SCANLINE0_EVENT, ENABLE);
    if(ret != ARM_DRIVER_OK){
        printf("\r\n Error: CDC200_SCANLINE0_EVENT enable failed\n");
        return 4;
    }

    /* Start CDC */
    ret = CDCdrv->Start();
    if(ret != ARM_DRIVER_OK){
        printf("\r\n Error: CDC Start failed\n");
        return 5;
    }

    ret = utimer->Initialize(ARM_UTIMER_CHANNEL4, utimer_cb);
    if(ret != ARM_DRIVER_OK) {
        return 15;
    }

    ret = utimer->PowerControl(ARM_UTIMER_CHANNEL4, ARM_POWER_FULL);
    if(ret != ARM_DRIVER_OK) {
        return 16;
    }
    ret = utimer->ConfigCounter(ARM_UTIMER_CHANNEL4, ARM_UTIMER_MODE_COMPARING, ARM_UTIMER_COUNTER_UP);
    if(ret != ARM_DRIVER_OK) {
        return 17;
    }
    ret = utimer->SetCount(ARM_UTIMER_CHANNEL4, ARM_UTIMER_CNTR, 0);
    if(ret != ARM_DRIVER_OK) {
        return 18;
    }
    ret = utimer->SetCount(ARM_UTIMER_CHANNEL4, ARM_UTIMER_CNTR_PTR, UTIMER_COUNTER_VALUE);
    if(ret != ARM_DRIVER_OK) {
        return 19;
    }
    // set utimer comparator value to same as counter, max on time = brightest display
    ret = utimer->SetCount(ARM_UTIMER_CHANNEL4, ARM_UTIMER_COMPARE_B_BUF1, UTIMER_COUNTER_VALUE);
    if(ret != ARM_DRIVER_OK) {
        return 20;
    }
    ret = utimer->Start(ARM_UTIMER_CHANNEL4);
    if(ret != ARM_DRIVER_OK) {
        return 21;
    }

    return 0;
}

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/
static void disp_flush(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    if (disp_flush_enabled && lv_disp_flush_is_last(disp_drv)) {
        d2_finish_rendering();

        //uint32_t size = lv_area_get_width(area) * lv_area_get_height(area)
        //                * lv_color_format_get_size(lv_display_get_color_format(disp_drv));
        //SCB_CleanInvalidateDCache_by_Addr(px_map, size);

        #if defined(USE_EVT_GROUP)
        xEventGroupClearBits(dispEventGroupHandle, EVENT_DISP_BUFFER_READY);
        #else
        disp_buf_ready = false;
        #endif

        CDCdrv->Control(CDC200_FRAMEBUF_UPDATE_VSYNC, (uint32_t)px_map);

        #if defined(USE_EVT_GROUP)
        xEventGroupSetBits(dispEventGroupHandle, EVENT_DISP_BUFFER_CHANGED);
        #else
        disp_buf_changed = true;
        #endif
    }
}

/* Display buffer flush waiting callback
 */
static void disp_flush_wait(lv_display_t * disp)
{
#if defined(USE_EVT_GROUP)
    xEventGroupWaitBits(dispEventGroupHandle, EVENT_DISP_BUFFER_READY,
                        pdFALSE, pdFALSE, (100/portTICK_PERIOD_MS));

#else
    while (!disp_buf_ready) {
        sys_busy_loop_us(3300);
    }
#endif
}

/* Display event handler
 */
static void disp_callback(uint32_t event)
{
    if (event & ARM_CDC_SCANLINE0_EVENT) {
        #if defined(USE_EVT_GROUP)
        if (xEventGroupGetBitsFromISR(dispEventGroupHandle)
           & EVENT_DISP_BUFFER_CHANGED) {
            xEventGroupClearBitsFromISR(
                dispEventGroupHandle,
                EVENT_DISP_BUFFER_CHANGED);

            BaseType_t context_switch = pdFALSE;
            xEventGroupSetBitsFromISR(
                dispEventGroupHandle,
                EVENT_DISP_BUFFER_READY,
                &context_switch);
            portYIELD_FROM_ISR(context_switch);
        }
        #else
        if (disp_buf_changed) {
            disp_buf_changed = false;
            disp_buf_ready = true;
        }
        #endif
    }

    if (event & ARM_CDC_DSI_ERROR_EVENT) {
        // Transfer Error: Received Hardware error.
        __BKPT(0);
    }
}
