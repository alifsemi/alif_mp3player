#/* Copyright (C) 2022 Alif Semiconductor - All Rights Reserved.
# * Use, distribution and modification of this code is permitted under the
# * terms stated in the Alif Semiconductor Software License Agreement
# *
# * You should have received a copy of the Alif Semiconductor Software
# * License Agreement with this file. If not, please write to:
# * contact@alifsemi.com, or visit: https://alifsemi.com/license
# *
# */
set(CMAKE_C_COMPILER                arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER              arm-none-eabi-g++)
SET(CMAKE_LINKER                    arm-none-eabi-ld)
set(CMAKE_CROSSCOMPILING            true)
set(CMAKE_SYSTEM_NAME               Generic)

if ("${ENSEMBLE_CORE}" STREQUAL "M55_HE" OR "${ENSEMBLE_CORE}" STREQUAL "M55_HP")
    set(CMAKE_SYSTEM_PROCESSOR          "cortex-m55")
    set(CMAKE_C_FLAGS                   "-mcpu=cortex-m55")
    set(CMAKE_CXX_FLAGS                 "-mcpu=cortex-m55")
    set(CMAKE_ASM_CPU_FLAG              cortex-m55)
    set(CPU_COMPILE_DEF                 CPU_CORTEX_M55)
endif()
if ("${ENSEMBLE_CORE}" STREQUAL "A32")
    set(CMAKE_SYSTEM_PROCESSOR          "cortex-a32")
    set(CMAKE_C_FLAGS                   "-mcpu=cortex-a32+crypto")
    set(CMAKE_CXX_FLAGS                 "-mcpu=cortex-a32+crypto")
    set(CMAKE_ASM_FLAGS                 "-mcpu=cortex-a32+crypto")
endif()

set(CPU_NAME                        ${CMAKE_SYSTEM_PROCESSOR})

# Generate .elf
set(BINARY_EXTENSION                ".elf")

find_program(CMAKE_C_COMPILER       arm-none-eabi-gcc)
find_program(CMAKE_CXX_COMPILER     arm-none-eabi-g++)

if (DEFINED XIP)
    set(MEM_LAYOUT                 "MRAM")
    set (XIP ${XIP}) # To avoid CMake warning about unused XIP variable
else()
    # Default is TCM
    set(MEM_LAYOUT                 "TCM")
endif()

set(CMAKE_C_FLAGS_DEBUG            "-O0 -g"          CACHE STRING "Flags used by the C compiler during DEBUG builds.")
set(CMAKE_C_FLAGS_MINSIZEREL       "-Os -g -DNDEBUG" CACHE STRING "Flags used by the C compiler during MINSIZEREL builds.")
set(CMAKE_C_FLAGS_RELEASE          "-O2 -g -DNDEBUG" CACHE STRING "Flags used by the C compiler during RELEASE builds.")

set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g"          CACHE STRING "Flags used by the CXX compiler during DEBUG builds.")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -g -DNDEBUG" CACHE STRING "Flags used by the CXX compiler during MINSIZEREL builds.")
set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -g -DNDEBUG" CACHE STRING "Flags used by the CXX compiler during RELEASE builds.")

add_compile_options(
    -mfloat-abi=hard
    -Wall
    -Wno-unused-function
    -Wextra
    -Wvla
    -Wno-error=cpp
    -c
    -fdata-sections
    -ffunction-sections
    -fshort-enums
    -funsigned-char
    $<$<COMPILE_LANGUAGE:C>:-std=c99>
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
    -MD
)

macro(set_warnings_as_errors target)
    target_compile_options(${target} PRIVATE
        -Werror
    )
endmacro()

macro(set_werror_implicit_function_declaration target)
    target_compile_options(${target} PRIVATE
        -Werror-implicit-function-declaration
    )
endmacro()

macro(set_sysroot)

endmacro()

macro(add_elf_to_bin_phase target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND arm-none-eabi-objcopy -O binary ${target}.elf ${target}.bin
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endmacro()

macro(add_linker_file_to_target target define)
    set(linker_file ${CMAKE_CURRENT_SOURCE_DIR}/linker_files/gcc_${ENSEMBLE_CORE}_${MEM_LAYOUT}.ld)

    message(STATUS "Using ld file: ${linker_file} for target ${target}")

    add_library(${target}_linkerfile OBJECT)

    target_link_options(${target}
        PRIVATE
        -T $<TARGET_OBJECTS:${target}_linkerfile>
        "SHELL:-Xlinker -Map=bin/${target}.map"
    )

    target_sources(${target}_linkerfile
        PRIVATE
        ${linker_file}
    )

    set_source_files_properties(${linker_file}
        PROPERTIES
        LANGUAGE C
    )

    add_dependencies(${target}
        ${target}_linkerfile
    )

    set_target_properties(${target} PROPERTIES LINK_DEPENDS $<TARGET_OBJECTS:${target}_linkerfile>)

    target_link_libraries(${target}_linkerfile
        rte_interface
        ensemblecmsis_interface
    )

    target_compile_options(${target}_linkerfile
        PRIVATE
            -E
            -P
            -xc
    )

    target_compile_definitions(${target}_linkerfile
        PRIVATE
            ${define}
    )
endmacro()

add_link_options(
    -mthumb
    -mfloat-abi=hard
    -mlittle-endian
    -specs=nosys.specs
    -Wl,--print-memory-usage
    --entry=Reset_Handler
)
