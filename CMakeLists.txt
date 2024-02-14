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
include(${LIBRARIES_DIR}/cmsis-alif-dfp.cmake)
include(${LIBRARIES_DIR}/common_app_utils.cmake)

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
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    rte_interface
    boardlib
    ensemblecmsis_interface
    common_app_utils
)

set_warnings_as_errors(${PROJECT_NAME})

add_linker_file_to_target(${PROJECT_NAME} ${ENSEMBLE_CORE})
