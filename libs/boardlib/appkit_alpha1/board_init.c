/* Copyright (C) 2022 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

/******************************************************************************
 * @file     board_AppKit_Alpha1.c
 * @brief    BOARD API implementation for Alif AI/ML Application Kit (Rev. A)
 ******************************************************************************/

#include "board.h"

#if defined(BOARD_IS_ALIF_APPKIT_ALPHA1_VARIANT)
#include "app_map.h"
#include "global_map.h"
#include "Driver_GPIO.h"
#include "Driver_PINMUX_AND_PINPAD.h"

#include "drv_i2c_bitbang.h"

extern ARM_DRIVER_GPIO ARM_Driver_GPIO_(BOARD_LED1_GPIO_PORT);
extern ARM_DRIVER_GPIO ARM_Driver_GPIO_(BOARD_LED2_GPIO_PORT);

void BOARD_Pinmux_Init()
{
	extern ARM_DRIVER_GPIO ARM_Driver_GPIO_(BOARD_LCD_RESET_GPIO_PORT);
	extern ARM_DRIVER_GPIO ARM_Driver_GPIO_(BOARD_LCD_BACKLIGHT_GPIO_PORT);
	extern ARM_DRIVER_GPIO ARM_Driver_GPIO_(BOARD_TOUCH_RESET_GPIO_PORT);
	extern ARM_DRIVER_GPIO ARM_Driver_GPIO_(BOARD_TOUCH_INT_GPIO_PORT);
	extern ARM_DRIVER_GPIO ARM_Driver_GPIO_(BOARD_CAMERA_RESET_GPIO_PORT);
	extern ARM_DRIVER_GPIO ARM_Driver_GPIO_(BOARD_CAMERA_POWER_GPIO_PORT);

	ARM_DRIVER_GPIO *BOARD_LED1_GPIOdrv = &ARM_Driver_GPIO_(BOARD_LED1_GPIO_PORT);
	ARM_DRIVER_GPIO *BOARD_LED2_GPIOdrv = &ARM_Driver_GPIO_(BOARD_LED2_GPIO_PORT);
	ARM_DRIVER_GPIO *BOARD_LCD_RESET_GPIOdrv = &ARM_Driver_GPIO_(BOARD_LCD_RESET_GPIO_PORT);
	ARM_DRIVER_GPIO *BOARD_LCD_BACKLIGHT_GPIOdrv = &ARM_Driver_GPIO_(BOARD_LCD_BACKLIGHT_GPIO_PORT);
	ARM_DRIVER_GPIO *BOARD_TOUCH_RESET_GPIOdrv = &ARM_Driver_GPIO_(BOARD_TOUCH_RESET_GPIO_PORT);
	ARM_DRIVER_GPIO *BOARD_TOUCH_INT_GPIOdrv = &ARM_Driver_GPIO_(BOARD_TOUCH_INT_GPIO_PORT);
	ARM_DRIVER_GPIO *BOARD_CAMERA_RESET_GPIOdrv = &ARM_Driver_GPIO_(BOARD_CAMERA_RESET_GPIO_PORT);
	ARM_DRIVER_GPIO *BOARD_CAMERA_POWER_GPIOdrv = &ARM_Driver_GPIO_(BOARD_CAMERA_POWER_GPIO_PORT);

	/* GPIO interfaces - initial GPIO state is lowest power */

	HW_REG32(PINMUX_BASE, 0x10) = 0;
	HW_REG32(PINMUX_BASE, 0x14) = 0;
	HW_REG32(PINMUX_BASE, 0x18) = 0;
	HW_REG32(PINMUX_BASE, 0x1C) = 0x11110011;	/* PINMUX for JTAG and SE-UART */
	HW_REG32(PINMUX_BASE, 0x20) = 0;
	HW_REG32(PINMUX_BASE, 0x24) = 0;
	HW_REG32(PINMUX_BASE, 0x28) = 0;
	HW_REG32(PINMUX_BASE, 0x2C) = 0;
	HW_REG32(PINMUX_BASE, 0x30) = 0;
	HW_REG32(PINMUX_BASE, 0x34) = 0;
	HW_REG32(PINMUX_BASE, 0x38) = 0;

	BOARD_LED1_GPIOdrv->SetValue(BOARD_LED1_PIN_NO, GPIO_PIN_OUTPUT_STATE_LOW);
	BOARD_LED1_GPIOdrv->SetDirection(BOARD_LED1_PIN_NO, GPIO_PIN_DIRECTION_OUTPUT);
	PINMUX_Config(BOARD_LED1_GPIO_PORT-1, BOARD_LED1_PIN_NO, PINMUX_ALTERNATE_FUNCTION_0);

	BOARD_LED2_GPIOdrv->SetValue(BOARD_LED2_PIN_NO, GPIO_PIN_OUTPUT_STATE_LOW);
	BOARD_LED2_GPIOdrv->SetDirection(BOARD_LED2_PIN_NO, GPIO_PIN_DIRECTION_OUTPUT);
	PINMUX_Config(BOARD_LED2_GPIO_PORT-1, BOARD_LED2_PIN_NO, PINMUX_ALTERNATE_FUNCTION_0);

	BOARD_LCD_RESET_GPIOdrv->SetValue(BOARD_LCD_RESET_PIN_NO, GPIO_PIN_OUTPUT_STATE_LOW);
	BOARD_LCD_RESET_GPIOdrv->SetDirection(BOARD_LCD_RESET_PIN_NO, GPIO_PIN_DIRECTION_OUTPUT);
	PINMUX_Config(BOARD_LCD_RESET_GPIO_PORT-1, BOARD_LCD_RESET_PIN_NO, PINMUX_ALTERNATE_FUNCTION_0);

	BOARD_LCD_BACKLIGHT_GPIOdrv->SetValue(BOARD_LCD_BACKLIGHT_PIN_NO, GPIO_PIN_OUTPUT_STATE_LOW);
	BOARD_LCD_BACKLIGHT_GPIOdrv->SetDirection(BOARD_LCD_BACKLIGHT_PIN_NO, GPIO_PIN_DIRECTION_OUTPUT);
	PINMUX_Config(BOARD_LCD_BACKLIGHT_GPIO_PORT-1, BOARD_LCD_BACKLIGHT_PIN_NO, PINMUX_ALTERNATE_FUNCTION_0);

	BOARD_TOUCH_RESET_GPIOdrv->SetValue(BOARD_TOUCH_RESET_PIN_NO, GPIO_PIN_OUTPUT_STATE_LOW);
	BOARD_TOUCH_RESET_GPIOdrv->SetDirection(BOARD_TOUCH_RESET_PIN_NO, GPIO_PIN_DIRECTION_OUTPUT);
	PINMUX_Config(BOARD_TOUCH_RESET_GPIO_PORT-1, BOARD_TOUCH_RESET_PIN_NO, PINMUX_ALTERNATE_FUNCTION_0);

	BOARD_TOUCH_INT_GPIOdrv->SetValue(BOARD_TOUCH_INT_PIN_NO, GPIO_PIN_OUTPUT_STATE_LOW);
	BOARD_TOUCH_INT_GPIOdrv->SetDirection(BOARD_TOUCH_INT_PIN_NO, GPIO_PIN_DIRECTION_OUTPUT);
	PINMUX_Config(BOARD_TOUCH_INT_GPIO_PORT-1, BOARD_TOUCH_INT_PIN_NO, PINMUX_ALTERNATE_FUNCTION_0);

	BOARD_CAMERA_RESET_GPIOdrv->SetValue(BOARD_CAMERA_RESET_PIN_NO, GPIO_PIN_OUTPUT_STATE_LOW);
	BOARD_CAMERA_RESET_GPIOdrv->SetDirection(BOARD_CAMERA_RESET_PIN_NO, GPIO_PIN_DIRECTION_OUTPUT);
	PINMUX_Config(BOARD_CAMERA_RESET_GPIO_PORT-1, BOARD_CAMERA_RESET_PIN_NO, PINMUX_ALTERNATE_FUNCTION_0);

	BOARD_CAMERA_POWER_GPIOdrv->SetValue(BOARD_CAMERA_POWER_PIN_NO, GPIO_PIN_OUTPUT_STATE_LOW);
	BOARD_CAMERA_POWER_GPIOdrv->SetDirection(BOARD_CAMERA_POWER_PIN_NO, GPIO_PIN_DIRECTION_OUTPUT);
	PINMUX_Config(BOARD_CAMERA_POWER_GPIO_PORT-1, BOARD_CAMERA_POWER_PIN_NO, PINMUX_ALTERNATE_FUNCTION_0);

	/* OSPI1 interface */
	PINMUX_Config(PORT_NUMBER_2, PIN_NUMBER_8, PINMUX_ALTERNATE_FUNCTION_4);  // D0
	PINMUX_Config(PORT_NUMBER_2, PIN_NUMBER_9, PINMUX_ALTERNATE_FUNCTION_4);  // D1
	PINMUX_Config(PORT_NUMBER_2, PIN_NUMBER_10, PINMUX_ALTERNATE_FUNCTION_4); // D2
	PINMUX_Config(PORT_NUMBER_2, PIN_NUMBER_11, PINMUX_ALTERNATE_FUNCTION_4); // D3
	PINMUX_Config(PORT_NUMBER_2, PIN_NUMBER_12, PINMUX_ALTERNATE_FUNCTION_4); // D4
	PINMUX_Config(PORT_NUMBER_2, PIN_NUMBER_13, PINMUX_ALTERNATE_FUNCTION_4); // D5
	PINMUX_Config(PORT_NUMBER_2, PIN_NUMBER_14, PINMUX_ALTERNATE_FUNCTION_3); // D6
	PINMUX_Config(PORT_NUMBER_2, PIN_NUMBER_15, PINMUX_ALTERNATE_FUNCTION_4); // D7
	PINMUX_Config(PORT_NUMBER_2, PIN_NUMBER_16, PINMUX_ALTERNATE_FUNCTION_4); // RXDS
	PINMUX_Config(PORT_NUMBER_2, PIN_NUMBER_17, PINMUX_ALTERNATE_FUNCTION_3); // CS
	PINMUX_Config(PORT_NUMBER_2, PIN_NUMBER_19, PINMUX_ALTERNATE_FUNCTION_4); // SCLK
	PINMUX_Config(PORT_NUMBER_3, PIN_NUMBER_5, PINMUX_ALTERNATE_FUNCTION_0);  // RESET
	PINPAD_Config(PORT_NUMBER_2, PIN_NUMBER_8, PAD_FUNCTION_READ_ENABLE);
	PINPAD_Config(PORT_NUMBER_2, PIN_NUMBER_9, PAD_FUNCTION_READ_ENABLE);
	PINPAD_Config(PORT_NUMBER_2, PIN_NUMBER_10, PAD_FUNCTION_READ_ENABLE);
	PINPAD_Config(PORT_NUMBER_2, PIN_NUMBER_11, PAD_FUNCTION_READ_ENABLE);
	PINPAD_Config(PORT_NUMBER_2, PIN_NUMBER_12, PAD_FUNCTION_READ_ENABLE);
	PINPAD_Config(PORT_NUMBER_2, PIN_NUMBER_13, PAD_FUNCTION_READ_ENABLE);
	PINPAD_Config(PORT_NUMBER_2, PIN_NUMBER_14, PAD_FUNCTION_READ_ENABLE);
	PINPAD_Config(PORT_NUMBER_2, PIN_NUMBER_15, PAD_FUNCTION_READ_ENABLE);
	PINPAD_Config(PORT_NUMBER_2, PIN_NUMBER_16, PAD_FUNCTION_READ_ENABLE);
	PINPAD_Config(PORT_NUMBER_2, PIN_NUMBER_17, PAD_FUNCTION_READ_ENABLE);
	PINPAD_Config(PORT_NUMBER_2, PIN_NUMBER_19, PAD_FUNCTION_READ_ENABLE);
	PINPAD_Config(PORT_NUMBER_3, PIN_NUMBER_5, PAD_FUNCTION_READ_ENABLE);

	/* CAMERA clock output */
	PINMUX_Config(PORT_NUMBER_3, PIN_NUMBER_15, PINMUX_ALTERNATE_FUNCTION_7);

	i2c_init();
}

void BOARD_Power_Init()
{
	/* Configure any board level power supplies */
}

void BOARD_Clock_Init()
{
	/* Configure any SoC clock muxes and dividers
	 * if not already covered in driver code
	 */
}

void BOARD_BUTTON1_Init(BOARD_Callback_t user_cb) { /* not implemented */ }
void BOARD_BUTTON2_Init(BOARD_Callback_t user_cb) { /* not implemented */ }

void BOARD_BUTTON1_Control(BOARD_BUTTON_CONTROL control) { /* not implemented */ }
void BOARD_BUTTON2_Control(BOARD_BUTTON_CONTROL control) { /* not implemented */ }

void BOARD_BUTTON1_GetState(BOARD_BUTTON_STATE *state)
{
    /* not implemented */
	*state = BOARD_BUTTON_STATE_LOW;
}

void BOARD_BUTTON2_GetState(BOARD_BUTTON_STATE *state)
{
    /* not implemented */
	*state = BOARD_BUTTON_STATE_LOW;
}

void BOARD_LED1_Control(BOARD_LED_STATE state){
	ARM_DRIVER_GPIO *BOARD_LED1_GPIOdrv = &ARM_Driver_GPIO_(BOARD_LED1_GPIO_PORT);
	BOARD_LED1_GPIOdrv->SetValue(BOARD_LED1_PIN_NO, state);
}

void BOARD_LED2_Control(BOARD_LED_STATE state)
{
	ARM_DRIVER_GPIO *BOARD_LED2_GPIOdrv = &ARM_Driver_GPIO_(BOARD_LED2_GPIO_PORT);
	BOARD_LED2_GPIOdrv->SetValue(BOARD_LED2_PIN_NO, state);
}

#endif