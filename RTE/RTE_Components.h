/*
 * Auto generated Run-Time-Environment Component Configuration File
 *      *** Do not modify ! ***
 *
 * Project: testapp
 * RTE configuration: testapp.rteconfig
*/
#ifndef RTE_COMPONENTS_H
#define RTE_COMPONENTS_H

/*
 * Define the Device Header File:
*/
#if defined(M55_HP)
    #define CMSIS_device_header "M55_HP.h"
#elif defined(M55_HE)
    #define CMSIS_device_header "M55_HE.h"
#elif defined(A32)
    #define CMSIS_device_header "a32_device.h"
    #include "system_utils.h"
#else
    #error "Undefined M55 CPU!"
#endif

/* AlifSemiconductor::BSP.External peripherals.OSPI Flash ISSI */
#define RTE_Drivers_ISSI_FLASH	1

/* AlifSemiconductor::CMSIS Driver.USART.USART */
#define RTE_Drivers_USART2   1           /* Driver UART2 */
#define RTE_Drivers_USART3   1           /* Driver UART3 */
#define RTE_Drivers_USART4   1           /* Driver UART4 */

/* AlifSemiconductor::Device.SOC Peripherals.GPIO */
#define RTE_Drivers_GPIO     1           /* Driver GPIO */
/* AlifSemiconductor::Device.SOC Peripherals.PINCONF */
#define RTE_Drivers_PINCONF  1           /* Driver PinPAD and PinMux */

#ifdef ENABLE_DMA
#define RTE_Drivers_DMA      1           /* DMA Driver */
#endif


#define RTE_Drivers_CANFD    1
#define RTE_Drivers_SAI      1           /* I2S Driver */
#define RTE_Drivers_OSPI     1           /* Driver OSPI */

#define RTE_Drivers_RTC      1
#define RTE_Drivers_LPTIMER  1

#define RTE_Drivers_CDC200   1 /* Display driver */
#define RTE_Drivers_MIPI_DSI 1
#define RTE_Drivers_MIPI_DSI_ILI9806E_PANEL 1

#define RTE_Drivers_CPI  1
#define RTE_Drivers_MIPI_CSI2 1

#endif /* RTE_COMPONENTS_H */
