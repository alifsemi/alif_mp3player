#  Copyright (C) 2024 Alif Semiconductor - All Rights Reserved.
#  Use, distribution and modification of this code is permitted under the
#  terms stated in the Alif Semiconductor Software License Agreement
#
#  You should have received a copy of the Alif Semiconductor Software
#  License Agreement with this file. If not, please write to:
#  contact@alifsemi.com, or visit: https://alifsemi.com/license

cmake_minimum_required(VERSION 3.21.0)

# Build in release mode by default
if (NOT CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_BUILD_TYPE Release CACHE INTERNAL "")
endif()

message(STATUS "Build type is set to ${CMAKE_BUILD_TYPE}")

set(ENSEMBLE_CORE "M55_HP" CACHE STRING "Ensemble core selection")
set_property(CACHE ENSEMBLE_CORE PROPERTY STRINGS "M55_HE" "M55_HP")

# Set language standards.
set(CMAKE_C_STANDARD   99)
set(CMAKE_CXX_STANDARD 14)

# Make the standard a requirement => prevent fallback to previous
# supported standard
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# We want to pass standard C/C++ flags, without gnu extensions
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set(LIBRARIES_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/libs)

message(STATUS "Using Alif Ensemble core ${ENSEMBLE_CORE}")

if (${ENSEMBLE_CORE} STREQUAL "M55_HP")
    add_compile_definitions(
        _RTE_
        M55_HP
    )
elseif (${ENSEMBLE_CORE} STREQUAL "M55_HE")
    add_compile_definitions(
        _RTE_
        M55_HE
    )
else()
    message(FATAL_ERROR "Unsupported ENSEMBLE_CORE value!")
endif()

message(STATUS "Using CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")

if(POLICY CMP0123)
    cmake_policy(SET CMP0123 NEW)
endif()

project(alif_mp3player
        VERSION     0.0.1
        DESCRIPTION "Alif Semiconductor Ensemble MP3 player demo"
        LANGUAGES   C CXX ASM)

include(${LIBRARIES_DIR}/boardlib.cmake)

add_library(rte_interface INTERFACE)
target_include_directories(rte_interface INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/RTE
)

target_link_libraries(rte_interface INTERFACE
    boardlib
)

include(${LIBRARIES_DIR}/cmsis.cmake)

set(CMSIS_SRC_PATH ${LIBRARIES_DIR}/CMSIS)
set(CMSIS_DSP_SRC_PATH ${LIBRARIES_DIR}/CMSIS-dsp)
include(${LIBRARIES_DIR}/cmsis-dsp.cmake)
include(${LIBRARIES_DIR}/cmsis-alif-dfp.cmake)
include(${LIBRARIES_DIR}/common_app_utils.cmake)

# Include CMSIS-arm2d
set(ARM_2D_SRC_PATH ${LIBRARIES_DIR}/CMSIS-arm2d)
include(${LIBRARIES_DIR}/cmsis-arm2d.cmake)

# Include LVGL
set(LVGL_SRC_PATH ${LIBRARIES_DIR}/lvgl)
include(${LIBRARIES_DIR}/lvgl.cmake)

include(${LIBRARIES_DIR}/minimp3.cmake)

set(SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/source)

add_executable(${PROJECT_NAME})

# Generate .axf or .elf depending on the compiler
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    SUFFIX ${BINARY_EXTENSION}
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# add bin generation from axf/elf file
add_elf_to_bin_phase(${PROJECT_NAME})

# Add sources
target_sources(${PROJECT_NAME} PRIVATE
    ${SRC_PATH}/main.c

    # lvgl
    ${SRC_PATH}/lvgl/lv_port_disp.c

    # GUI
    ${SRC_PATH}/GUI/lv_demo_music_main.c
    ${SRC_PATH}/GUI/lv_demo_music.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/lv_demo_music_list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_cover_1_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_cover_2_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_icon_1_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_icon_2_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_wave_top_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_wave_bottom_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_corner_left_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_corner_right_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_btn_next_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_btn_prev_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_btn_play_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_btn_pause_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_btn_list_pause_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_btn_list_play_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_slider_knob_large.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_logo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/lvgl/demos/music/assets/img_lv_demo_music_list_border_large.c

    # audio
    ${SRC_PATH}/audio/audio.c
    ${SRC_PATH}/audio/wm8904_driver.c
    ${SRC_PATH}/audio/minimp3.c

    # mpu
    ${SRC_PATH}/mpu/mpu.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SRC_PATH}/lvgl
    ${SRC_PATH}/audio
    ${SRC_PATH}/GUI
    ${SRC_PATH}/mpu
    ${LVGL_SRC_PATH}/demos/music
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    rte_interface
    boardlib
    ensemblecmsis_interface
    common_app_utils
    ${LVGL_TARGET}
    ${CMSIS_DSP_TARGET}
    minimp3
)

add_linker_file_to_target(${PROJECT_NAME} ${ENSEMBLE_CORE})
