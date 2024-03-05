/* Copyright (C) 2023 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

/**************************************************************************//**
 * @file     lv_port_disp.c
 * @author   Ahmad Rashed
 * @email    ahmad.rashed@alifsemi.com
 * @version  V1.0.0
 * @date     28-March-2022
 * @brief    for lvgl library display init with touch input
 * @bug      None.
 * @Note     None
 ******************************************************************************/

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

/* PINMUX Driver */
#include "pinconf.h"

/* SE Services */
#include "se_services_port.h"
#include "lv_port_disp.h"
#include "lv_port.h"

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

volatile uint8_t line_irq_status = 0;
volatile uint8_t dsi_err = 0;

/**
  \fn          void hw_disp_cb(uint32_t event)
  \brief       Display callback
  \param[in]   event: Display Event
  \return      none
  */
void hw_disp_cb(uint32_t event)
{
    if(event & ARM_CDC_SCANLINE0_EVENT)
    {
        /* Received scan line event. */
        line_irq_status = 1;
    }

    if(event & ARM_CDC_DSI_ERROR_EVENT)
    {
        /* Transfer Error: Received Hardware error, Wake-up Thread. */
        dsi_err = 1;
    }
}

/**
 * @function    static void hw_disp_init(void)
  \brief        Initializes CDC200 controller
  \param[in]    none
  \return       none
  */
static uint32_t hw_disp_init(void)
{
    int ret = 0;
    uint32_t  service_error_code;
    uint32_t  error_code;
    run_profile_t runp = {0};

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
        goto error_disable_100mhz_clk;
    }

    /* Get the current run configuration from SE */
    error_code = SERVICES_get_run_cfg(se_services_s_handle,
                                      &runp,
                                      &service_error_code);
    if(error_code)
    {
        printf("\r\nSE: get_run_cfg error = %" PRIu32 "\n", error_code);
        goto error_disable_hfosc_clk;
    }

    runp.memory_blocks |= MRAM_MASK | SRAM0_MASK | SRAM1_MASK;

    runp.phy_pwr_gating |= MIPI_PLL_DPHY_MASK | MIPI_TX_DPHY_MASK | MIPI_RX_DPHY_MASK | LDO_PHY_MASK;

    /* Set the new run configuration */
    error_code = SERVICES_set_run_cfg(se_services_s_handle,
                                      &runp,
                                      &service_error_code);
    if(error_code)
    {
        printf("\r\nSE: set_run_cfg error = %" PRIu32 "\n", error_code);
        goto error_disable_hfosc_clk;
    }

    /* Initialize CDC200 controller */
    ret = CDCdrv->Initialize(hw_disp_cb);
    if(ret != ARM_DRIVER_OK)
    {
        /* Error in CDC200 initialize */
        printf("\r\n Error: CDC200 initialization failed.\r\n");
        goto error_disable_hfosc_clk;
    }

    /* Power ON CDC200 controller */
    ret = CDCdrv->PowerControl(ARM_POWER_FULL);
    if(ret != ARM_DRIVER_OK)
    {
        /* Error in CDC200 Power ON */
        printf("\r\n Error: CDC200 Power ON failed.\r\n");
        goto error_CDC200_uninitialize;
    }

    /* Configure CDC200 controller */
    ret = CDCdrv->Control(CDC200_CONFIGURE_DISPLAY, (uint32_t)lcd_image);
    if(ret != ARM_DRIVER_OK)
    {
        /* Error in CDC200 control configuration */
        printf("\r\n Error: CDC200 control configuration failed.\r\n");
        goto error_CDC200_poweroff;
    }

    /* Configure CDC200 controller */
    ret = CDCdrv->Control(CDC200_SCANLINE0_EVENT, ENABLE);
    if(ret != ARM_DRIVER_OK)
    {
        /* Error in CDC200 control configuration */
        printf("\r\n Error: CDC200 control configuration failed.\r\n");
    }

    /* Start CDC200 controller */
    ret = CDCdrv->Start();
    if(ret != ARM_DRIVER_OK)
    {
        /* Error in CDC200 start */
        printf("\r\n Error: CDC200 start failed.\r\n");
    }

    return 0;

error_CDC200_poweroff:
    /* Received error Power OFF CDC200 driver */
    ret = CDCdrv->PowerControl(ARM_POWER_OFF);
    if (ret != ARM_DRIVER_OK)
    {
        /* Error in CDC200 poweroff. */
        printf("ERROR: Could not power off CDC200\n");
        return 1;
    }

error_CDC200_uninitialize:
    /* Received error Un-initialize CDC200 */
    ret = CDCdrv->Uninitialize();
    if (ret != ARM_DRIVER_OK)
    {
        /* Error in CDC200 uninitialize. */
        printf("ERROR: Could not unintialize CDC200\n");
        return 2;
    }

error_disable_hfosc_clk:
    error_code = SERVICES_clocks_enable_clock(se_services_s_handle, CLKEN_HFOSC, false, &service_error_code);
    if(error_code != SERVICES_REQ_SUCCESS)
        printf("SE: MIPI 38.4Mhz(HFOSC)  clock disable = %" PRIu32 "\n", error_code);

error_disable_100mhz_clk:
    error_code = SERVICES_clocks_enable_clock(se_services_s_handle, CLKEN_CLK_100M, false, &service_error_code);
    if(error_code != SERVICES_REQ_SUCCESS)
        printf("SE: MIPI 100MHz clock disable = %" PRIu32 "\n", error_code);

    return 3;
}

#if(I2C_TOUCH_ENABLE == 1)

/*touch screen driver */
#include "Driver_Touch_Screen.h"

/* Touch screen driver instance */
extern ARM_DRIVER_TOUCH_SCREEN GT911;
static ARM_DRIVER_TOUCH_SCREEN *Drv_Touchscreen = &GT911;

/**
  \fn          int hardware_cfg(void)
  \brief       i2c hardware pin initialization:
                   -  PIN-MUX and PIN_PAD configuration
               GPIO9 initialization:
                   -  PIN-MUX and PIN-PAD configuration
  \param[in]   none
  \return      ARM_DRIVER_OK: success; -1: failure
  */
int hardware_cfg(void)
{
    int ret = 0;

    /* Configure GPIO Pin : P9_4 as gpio pin
     *   Pad function: PAD_FUNCTION_READ_ENABLE |
     *                 PAD FUNCTION_DRIVER_DISABLE_STATE_WITH_PULL_UP |
     *                 PADCTRL_SCHMITT_TRIGGER_ENABLE
     */
    ret = pinconf_set(PORT_9, PIN_4, PINMUX_ALTERNATE_FUNCTION_0, PADCTRL_READ_ENABLE | \
            PADCTRL_DRIVER_DISABLED_PULL_UP | PADCTRL_SCHMITT_TRIGGER_ENABLE);
    if(ret != ARM_DRIVER_OK)
    {
        return -1;
    }

   /* Configure GPIO Pin : P7_2 as i2c1_sda_c
    * Pad function: PADCTRL_READ_ENABLE |
    *               PAD PADCTRL_DRIVER_DISABLED_PULL_UP
    */
    ret = pinconf_set(PORT_7, PIN_2, PINMUX_ALTERNATE_FUNCTION_5, PADCTRL_READ_ENABLE | \
            PADCTRL_DRIVER_DISABLED_PULL_UP);
    if(ret != ARM_DRIVER_OK)
    {
        return -1;
    }

    /* Configure GPIO Pin : P7_3 as i2c1_scl_c
     * Pad function: PADCTRL_READ_ENABLE |
     *               PADCTRL_DRIVER_DISABLED_PULL_UP
     */
    ret = pinconf_set(PORT_7, PIN_3, PINMUX_ALTERNATE_FUNCTION_5, PADCTRL_READ_ENABLE | \
            PADCTRL_DRIVER_DISABLED_PULL_UP);
    if(ret != ARM_DRIVER_OK)
    {
        return -1;
    }

    return ARM_DRIVER_OK;
}

/**
  \function     static void lv_touch_get(lv_indev_drv_t * drv, lv_indev_data_t * data)
  \brief        Check touch screen is pressed or not
  \param[in]    drv: pointer to LVGL driver
                data: data to LVGL driver
  \return       none
  */
static void lv_touch_get(lv_indev_t * indev, lv_indev_data_t * data)
{
    (void)indev;
    ARM_TOUCH_STATE status;
    Drv_Touchscreen->GetState(&status);

    if(status.numtouches)
    {
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    data->point.x = status.coordinates[0].x;
    data->point.y = status.coordinates[0].y;
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

    /* Initialize i3c and GPIO2 hardware pins using PinMux Driver. */
    ret = hardware_cfg();
    if(ret != ARM_DRIVER_OK)
    {
        /* Error in hardware configuration */
        printf("\r\n Error: Hardware configuration failed.\r\n");
        return 90;
    }

    /* Initialize GT911 touch screen */
    ret = Drv_Touchscreen->Initialize();
    if(ret != ARM_DRIVER_OK)
    {
        /* Error in GT911 touch screen initialize */
        printf("\r\n Error: GT911 touch screen initialization failed.\r\n");
        goto error_GT911_uninitialize;
    }

    /* Power ON GT911 touch screen */
    ret = Drv_Touchscreen->PowerControl(ARM_POWER_FULL);
    if(ret != ARM_DRIVER_OK)
    {
        /* Error in GT911 touch screen power up */
        printf("\r\n Error: GT911 touch screen Power Up failed.\r\n");
        goto error_GT911_uninitialize;
    }

    return 0;

error_GT911_uninitialize:
    /* Received error Un-initialize Touch screen driver */
    ret = Drv_Touchscreen->Uninitialize();
    if (ret != ARM_DRIVER_OK)
    {
        /* Error in GT911 Touch screen uninitialize. */
        printf("ERROR: Could not unintialize touch screen\n");
        return 7;
    }
    return 99;
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

    if(line_irq_status == 0)
        return;

    line_irq_status = 0;

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
    if (ret)
        return ret;

#if(I2C_TOUCH_ENABLE == 1)
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lv_touch_get);

    /* Touch screen hardware initialization */
    ret = hw_touch_init();
#endif
    return ret;
}
