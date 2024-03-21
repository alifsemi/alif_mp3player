#/* Copyright (C) 2022 Alif Semiconductor - All Rights Reserved.
# * Use, distribution and modification of this code is permitted under the
# * terms stated in the Alif Semiconductor Software License Agreement
# *
# * You should have received a copy of the Alif Semiconductor Software
# * License Agreement with this file. If not, please write to:
# * contact@alifsemi.com, or visit: https://alifsemi.com/license
# *
# */

set(CMAKE_C_COMPILER                armclang)
set(CMAKE_CXX_COMPILER              armclang)
set(CMAKE_ASM_COMPILER              armclang)
set(CMAKE_ASM_COMPILER_AR           armar)
SET(CMAKE_LINKER                    armlink)
set(CMAKE_CROSSCOMPILING            true)
set(CMAKE_SYSTEM_NAME               Generic)

# Skip compiler test execution
set(CMAKE_C_COMPILER_WORKS          1)
set(CMAKE_CXX_COMPILER_WORKS        1)


set(CMAKE_SYSTEM_PROCESSOR          "cortex-m55")
set(CMAKE_C_FLAGS                   "-mcpu=cortex-m55 -target=arm-arm-none-eabi")
set(CMAKE_CXX_FLAGS                 "-mcpu=cortex-m55 -target=arm-arm-none-eabi")
set(CMAKE_ASM_FLAGS                 "-mcpu=cortex-m55 -target=arm-arm-none-eabi")
set(CMAKE_ASM_CPU_FLAG              Cortex-M55)
set(CPU_COMPILE_DEF                 CPU_CORTEX_M55)

set(CPU_NAME                        ${CMAKE_SYSTEM_PROCESSOR})

set(BINARY_EXTENSION                ".axf")

set(CMAKE_C_FLAGS_DEBUG            "-O0 -g"          CACHE STRING "Flags used by the C compiler during DEBUG builds.")
set(CMAKE_C_FLAGS_MINSIZEREL       "-Oz -g -DNDEBUG" CACHE STRING "Flags used by the C compiler during MINSIZEREL builds.")
set(CMAKE_C_FLAGS_RELEASE          "-O3 -g -DNDEBUG" CACHE STRING "Flags used by the C compiler during RELEASE builds.")

set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g"          CACHE STRING "Flags used by the CXX compiler during DEBUG builds.")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Oz -g -DNDEBUG" CACHE STRING "Flags used by the CXX compiler during MINSIZEREL builds.")
set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -g -DNDEBUG" CACHE STRING "Flags used by the CXX compiler during RELEASE builds.")

add_compile_options(
    -Wno-ignored-optimization-argument
    -Wno-unused-command-line-argument
    -Wall
    -Wno-unused-function
    -Wextra
    -Wvla
    -Wno-error=cpp
    -c
    -fdata-sections
    -ffunction-sections
    -fshort-enums
    -fshort-wchar
    -funsigned-char
    -masm=auto
    $<$<COMPILE_LANGUAGE:C>:-std=gnu99>
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++11>
    -mfloat-abi=hard
    -MD
)

macro(set_warnings_as_errors target)
    target_compile_options(${target} PRIVATE
        -Werror
    )
endmacro()

macro(add_elf_to_bin_phase target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND fromelf -v --bin --output ${target}.bin ${target}.axf
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endmacro()

macro(set_werror_implicit_function_declaration target)
    target_compile_options(${target} PRIVATE
        -Werror-implicit-function-declaration
    )
endmacro()

macro(set_sysroot)

endmacro()

macro(add_linker_file_to_target target define)
    set(linker_file ${CMAKE_CURRENT_SOURCE_DIR}/linker_files/${ENSEMBLE_CORE}.sct)
    message(STATUS "Using scatter file: ${linker_file} for target ${target}")

    add_library(${target}_linkerfile OBJECT)

    target_link_options(${target}
        PRIVATE
        --scatter=$<TARGET_OBJECTS:${target}_linkerfile>
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
        ensemblecmsis_interface
    )

    target_compile_options(${target}_linkerfile
        PRIVATE
            -E
            -xc
    )

    target_compile_definitions(${target}_linkerfile PRIVATE
        ${define})

endmacro()

set(ARMCLANG_INFO_STR "sizes,totals,unused,veneers,summarysizes,inline,tailreorder")
if(CMAKE_BUILD_TYPE STREQUAL Debug)
    # For debug builds, we can add stack information too:
    set(ARMCLANG_INFO_STR "${ARMCLANG_INFO_STR},stack,summarystack")
endif()

add_link_options(
    "$<$<CONFIG:RELEASE>:--inline>"
    --tailreorder
    --diag_suppress=L6439W,L6314W
    --info ${ARMCLANG_INFO_STR}
    --map
    --callgraph
    --strict
    --symbols
    --xref
    --cpu=${CMAKE_ASM_CPU_FLAG}
    --entry=Reset_Handler
)
