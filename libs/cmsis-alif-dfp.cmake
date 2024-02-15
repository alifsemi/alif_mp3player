
set(CMSIS_DIR ${CMAKE_CURRENT_LIST_DIR}/CMSIS-alif-dfp)
set(ALIF_DRIVER_DIR             ${CMSIS_DIR}/Alif_CMSIS)
set(ALIF_DEVICE_DIR             ${CMSIS_DIR}/Device)
set(ALIF_COMPONENTS_DIR         ${CMSIS_DIR}/components)
set(ALIF_ENSEMBLE_DRIVERS_DIR   ${CMSIS_DIR}/drivers)

add_library(ensemblecmsis_interface INTERFACE)

target_include_directories(ensemblecmsis_interface INTERFACE
    ${ALIF_DRIVER_DIR}/Include
    ${ALIF_DRIVER_DIR}/Include/config
    ${ALIF_COMPONENTS_DIR}/Include
    ${ALIF_COMPONENTS_DIR}/Source
    ${ALIF_PACK_DIR}/drivers/include
    ${ALIF_DEVICE_DIR}/common/include
    ${ALIF_DEVICE_DIR}/core/${ENSEMBLE_CORE}/include
    ${ALIF_DEVICE_DIR}/core/${ENSEMBLE_CORE}/config
    ${ALIF_ENSEMBLE_DRIVERS_DIR}/include
    ${CMSIS_DIR}/se_services/port/include
    ${CMSIS_DIR}/se_services/include
)

target_link_libraries(ensemblecmsis_interface INTERFACE
    armcmsis_interface
)

add_library(ensemblecmsis)

target_sources(ensemblecmsis PRIVATE
    ${ALIF_DEVICE_DIR}/common/source/mpu_M55.c
    ${ALIF_DEVICE_DIR}/core/${ENSEMBLE_CORE}/source/startup_${ENSEMBLE_CORE}.c
    ${ALIF_DEVICE_DIR}/common/source/system_M55.c
    ${ALIF_DEVICE_DIR}/common/source/tgu_M55.c
    ${ALIF_DEVICE_DIR}/common/source/pm.c
    ${ALIF_DEVICE_DIR}/common/source/system_utils.c
    ${ALIF_DRIVER_DIR}/Source/Driver_USART.c
    ${ALIF_ENSEMBLE_DRIVERS_DIR}/source/uart.c
    ${ALIF_ENSEMBLE_DRIVERS_DIR}/source/pinconf.c
    ${ALIF_ENSEMBLE_DRIVERS_DIR}/source/dsi.c
    ${ALIF_ENSEMBLE_DRIVERS_DIR}/source/csi.c
    ${ALIF_ENSEMBLE_DRIVERS_DIR}/source/cdc.c
    ${ALIF_ENSEMBLE_DRIVERS_DIR}/source/i2c.c
    ${ALIF_ENSEMBLE_DRIVERS_DIR}/source/mhu_driver.c
    ${ALIF_ENSEMBLE_DRIVERS_DIR}/source/mhu_receiver.c
    ${ALIF_ENSEMBLE_DRIVERS_DIR}/source/mhu_sender.c
    ${ALIF_DRIVER_DIR}/Source/Driver_GPIO.c
    ${ALIF_DRIVER_DIR}/Source/Driver_CDC200.c
    ${ALIF_DRIVER_DIR}/Source/Driver_MIPI_DSI.c
    ${ALIF_DRIVER_DIR}/Source/DPHY_init.c
    ${ALIF_DRIVER_DIR}/Source/Driver_I2C.c
    ${ALIF_COMPONENTS_DIR}/Source/GT911_touch_driver.c
    ${ALIF_COMPONENTS_DIR}/Source/ILI9806E_LCD_panel.c
    ${CMSIS_DIR}/se_services/source/services_host_clocks.c
    ${CMSIS_DIR}/se_services/source/services_host_handler.c
    ${CMSIS_DIR}/se_services/source/services_host_power.c
    ${CMSIS_DIR}/se_services/source/services_host_maintenance.c
    ${CMSIS_DIR}/se_services/port/se_services_port.c
)

target_include_directories(ensemblecmsis PRIVATE
    ${ALIF_DRIVER_DIR}/Include/config
    ${ALIF_DRIVER_DIR}/Source
)

target_link_libraries(ensemblecmsis
    ensemblecmsis_interface
    rte_interface
)
