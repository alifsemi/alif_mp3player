/* Copyright (C) 2024 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

/* System Includes */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

/*RTE configuration includes */
#include <RTE_Device.h>
#include <RTE_Components.h>
#include CMSIS_device_header

/* LVGL driver */
#include "lvgl.h"

/* CDC200 driver */
#include "Driver_CDC200.h"
#include "Driver_UTIMER.h"

#include "lv_port_disp.h"
#include "lv_port.h"
#include "display.h"
#include "pinconf.h"

#define I2C_TOUCH_ENABLE         1

/* Selecting LVGL color depth in matching with CDC200 controller pixel format */
#if ((LV_COLOR_DEPTH == 16) && (RTE_CDC200_PIXEL_FORMAT != 2)) || \
    ((LV_COLOR_DEPTH == 32) && (RTE_CDC200_PIXEL_FORMAT != 0))
#error "The LV_COLOR_DEPTH and RTE_CDC200_PIXEL_FORMAT must match."
#endif

#if RTE_CDC200_PIXEL_FORMAT   == 0
#define PIXEL_BYTES    (4)
#elif RTE_CDC200_PIXEL_FORMAT == 2
#define PIXEL_BYTES    (2)
#elif RTE_CDC200_PIXEL_FORMAT == 1
#error "Using 24-bit image buffer is not supported by LVGL, use 16-bit or 32-bit only."
#endif

#define DIMAGE_X                 (RTE_PANEL_HACTIVE_TIME)
#define DIMAGE_Y                 (RTE_PANEL_VACTIVE_LINE)

static uint8_t lcd_image[DIMAGE_Y][DIMAGE_X][PIXEL_BYTES] __attribute__((section(".bss.disp_buff")));
static uint8_t lcd_image2[DIMAGE_Y][DIMAGE_X][PIXEL_BYTES] __attribute__((section(".bss.disp_buff")));

/* CDC200 driver instance */
extern ARM_DRIVER_CDC200 Driver_CDC200;
static ARM_DRIVER_CDC200 *CDCdrv = &Driver_CDC200;

// UTIMER for outputting pwm signal to backlight led
#define UTIMER_COUNTER_VALUE 400000

// Target pwr on period when dimmed
#define UTIMER_DIMMED_TARGET_VALUE 10000

// Steps to change the on period towards UTIMER_DIMMED_TARGET_VALUE
#define UTIMER_DIMMING_STEP 1000

extern ARM_DRIVER_UTIMER DRIVER_UTIMER0;
static ARM_DRIVER_UTIMER* utimer = &DRIVER_UTIMER0;
static uint32_t target_cmp_value = UTIMER_COUNTER_VALUE;
static uint32_t current_cmp_value = UTIMER_COUNTER_VALUE;

/**
  \fn          void hw_disp_cb(uint32_t event)
  \brief       Display callback
  \param[in]   event: Display Event
  \return      none
  */
void hw_disp_cb(uint32_t event)
{
    (void)event;
}

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

/**
 * @function    static void hw_disp_init(void)
  \brief        Initializes CDC200 controller
  \param[in]    none
  \return       none
  */
static uint32_t hw_disp_init(void)
{
    int32_t ret = 0;
    // set display backlight to utimer to drive pwm signal towards the display
	pinconf_set(PORT_(BOARD_LCD_BACKLIGHT_GPIO_PORT), BOARD_LCD_BACKLIGHT_PIN_NO, PINMUX_ALTERNATE_FUNCTION_4, PADCTRL_OUTPUT_DRIVE_STRENGTH_4MA);
   
    ret = CDCdrv->Initialize(hw_disp_cb);
    if(ret != ARM_DRIVER_OK) {
        return 1;
    }

    ret = CDCdrv->PowerControl(ARM_POWER_FULL);
    if(ret != ARM_DRIVER_OK) {
        return 2;
    }

    ret = CDCdrv->Control(CDC200_CONFIGURE_DISPLAY, (uint32_t)lcd_image);
    if(ret != ARM_DRIVER_OK) {
        return 3;
    }

    ret = CDCdrv->Start();
    if(ret != ARM_DRIVER_OK) {
        return 4;
    }

    ret = utimer->Initialize(ARM_UTIMER_CHANNEL4, utimer_cb);
    if(ret != ARM_DRIVER_OK) {
        return 5;
    }

    ret = utimer->PowerControl(ARM_UTIMER_CHANNEL4, ARM_POWER_FULL);
    if(ret != ARM_DRIVER_OK) {
        return 6;
    }
    ret = utimer->ConfigCounter(ARM_UTIMER_CHANNEL4, ARM_UTIMER_MODE_COMPARING, ARM_UTIMER_COUNTER_UP);
    if(ret != ARM_DRIVER_OK) {
        return 7;
    }
    ret = utimer->SetCount(ARM_UTIMER_CHANNEL4, ARM_UTIMER_CNTR, 0);
    if(ret != ARM_DRIVER_OK) {
        return 8;
    }
    ret = utimer->SetCount(ARM_UTIMER_CHANNEL4, ARM_UTIMER_CNTR_PTR, UTIMER_COUNTER_VALUE);
    if(ret != ARM_DRIVER_OK) {
        return 9;
    }
    // set utimer comparator value to same as counter, max on time = brightest display
    ret = utimer->SetCount(ARM_UTIMER_CHANNEL4, ARM_UTIMER_COMPARE_B_BUF1, UTIMER_COUNTER_VALUE);
    if(ret != ARM_DRIVER_OK) {
        return 10;
    }
    ret = utimer->Start(ARM_UTIMER_CHANNEL4);
    if(ret != ARM_DRIVER_OK) {
        return 11;
    }

    return 0;
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

/**
  \function    void lv_disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p))
  \brief       Flush the data in buffer to display.
  \param[in]   disp_drv Pointer descriptor of a display driver.
  \param[in]   area     Area of the buffer containing data to be displayed.
  \param[in]   color_p  buffer containing the data to be flushed to display.
  \return      none
  */
static void lv_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    (void)area;
    int ret = 0 ;

    /* Configure CDC200 controller */
    ret = CDCdrv->Control(CDC200_FRAMEBUF_UPDATE, (uint32_t) px_map);
    if(ret != ARM_DRIVER_OK)
    {
        /* Error in CDC200 control configuration */
        printf("\r\n Error: CDC200 control configuration failed.\r\n");
    }

    /* Indicating flushing is done to display */
    lv_disp_flush_ready(disp);
}

/**
  \function void lv_port_disp_init(void)
  \brief    This function does:
                - storing data in temporary buffer.
                - flushing the data to display using callback function
                - enabling a touch pointer as a input device to display
  \param[in]   none
  \return      none
  */
uint32_t lv_port_disp_init(void)
{
    /* Initialize LVGL */
    lv_init();
    lv_tick_set_cb(clock);

    lv_display_t * disp = lv_display_create(DIMAGE_X, DIMAGE_Y);
    lv_display_set_flush_cb(disp, lv_disp_flush);
    lv_display_set_buffers(disp, lcd_image, lcd_image2, DIMAGE_Y*DIMAGE_X*PIXEL_BYTES, LV_DISPLAY_RENDER_MODE_FULL);

    /* Display hardware initialization */
    uint32_t ret = hw_disp_init();
    if (ret) {
        return ret;
    }

#if(I2C_TOUCH_ENABLE == 1)
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lv_touch_get);

    /* Touch screen hardware initialization */
    ret = hw_touch_init();
#endif
    return ret;
}
