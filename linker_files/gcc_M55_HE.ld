/* Copyright (C) 2024 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

/*
* M55-HE application specific settings. These need to changed based on
* application and system memory requirements
*/
__STACK_SIZE = 0x00007000;
__HEAP_SIZE  = 0x00007000;

/*
* This linker script is for application to be executed by the
* M55-HE core.
*/

MEMORY
{
  ITCM  (rwx) : ORIGIN = 0x00000000, LENGTH = 0x00040000
  DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 0x00040000
  MRAM  (rx)  : ORIGIN = 0x80000000, LENGTH = 0x00580000
  SRAM0 (rw)  : ORIGIN = 0x02000000, LENGTH = 0x00400000
  SRAM1 (rw)  : ORIGIN = 0x08000000, LENGTH = 0x00280000
}


ENTRY(Reset_Handler)

SECTIONS
{
  .text.at_mram : ALIGN(16)
  {
    KEEP(*(.vectors))

    KEEP(*(.init))
    KEEP(*(.fini))

    /* .ctors */
    *crtbegin.o(.ctors)
    *crtbegin?.o(.ctors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
    *(SORT(.ctors.*))
    *(.ctors)

    /* .dtors */
    *crtbegin.o(.dtors)
    *crtbegin?.o(.dtors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
    *(SORT(.dtors.*))
    *(.dtors)

    *(.rodata*)
    *(startup_ro_data)

    KEEP(*(.eh_frame*))
    . = ALIGN(16);
  } > MRAM

   .text.at_itcm : ALIGN(16)
  {
    /* lvgl fast code */
    *(fast_code)

    /* mp3 processing */
    *minimp3.c.obj(.text*)
    *audio.c.obj(.text*)

    . = ALIGN(16);
  } > ITCM AT> MRAM

  .text.at_mram : ALIGN(16)
  {
    *(.text*)
    . = ALIGN(16);
  } > MRAM

  .ARM.extab : ALIGN(16)
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(16);
  } > MRAM

  .ARM.exidx : ALIGN(16)
  {
  __exidx_start = .;
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  __exidx_end = .;
  . = ALIGN(16);
  } > MRAM

  .copy.table (READONLY) : ALIGN(16)
  {
    __copy_table_start__ = .;
    LONG (LOADADDR(.data.at_dtcm))
    LONG (ADDR(.data.at_dtcm))
    LONG (SIZEOF(.data.at_dtcm)/4)
    LONG (LOADADDR(.text.at_itcm))
    LONG (ADDR(.text.at_itcm))
    LONG (SIZEOF(.text.at_itcm)/4)
    __copy_table_end__ = .;
    . = ALIGN(16);
  } > MRAM

  .zero.table (READONLY) : ALIGN(16)
  {
    __zero_table_start__ = .;
    /* Add each additional bss section here */
    LONG (ADDR(.bss.at_dtcm))
    LONG (SIZEOF(.bss.at_dtcm)/4)
    LONG (ADDR(.bss.at_sram1))
    LONG (SIZEOF(.bss.at_sram1)/4)
    __zero_table_end__ = .;
    . = ALIGN(16);
  } > MRAM

  __etext = .;

  .data.at_dtcm : ALIGN(16)
  {
    __data_start__ = .;
    *(vtable)
    *(.data)
    *(.data*)

    . = ALIGN(4);
    /* preinit data */
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);

    . = ALIGN(4);
    /* init data */
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);

    . = ALIGN(4);
    /* finit data */
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array))
    PROVIDE_HIDDEN (__fini_array_end = .);

    KEEP(*(.jcr*))

    __data_end__ = .;
    . = ALIGN(16);
  } > DTCM AT> MRAM

  .tdata.at.dtcm : ALIGN(16)
  {
    /* thread-local data */
    __tls_base = .;
    __arm32_tls_tcb_offset = .;
    *(.tdata*)
    . = ALIGN(16);
  } > DTCM AT> MRAM

 /* Force alignment of load region as SETools requires multiple of 16 */
  .padding : ALIGN(16)
  {
    LONG (0)
    LONG (0)
    LONG (0)
    LONG (0)
  } > MRAM

  .bss.at_disp (NOLOAD) : ALIGN(8)
  {
    /* Display buffer needs to be in SRAM0 when using default MPU configuration from CMSIS-dfp pack */
    *(.bss.disp_buff)
  } > SRAM0

  .bss.at_sram1 : ALIGN(16)
  {
    /* LVGL big RAM buffer */
    *(big_ram)

    /* fault handler data */
    *(.bss.slow)

    /* other random data that doesn't feel perf critical to fit important stuff to DTCM */
    *(.bss.se_services_packet_buffer)
    *(.bss.retarget_buf)
    *(.bss.__atexit0)
  } > SRAM1

   .bss.at_dtcm (NOLOAD) : ALIGN(8)
  {
    __bss_start__ = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    __bss_end__ = .;
  } > DTCM

  .heap (NOLOAD) : ALIGN(8)
  {
    __end__ = .;
    PROVIDE(end = .);
    __heap_start = .;
    . = . + __HEAP_SIZE;
    . = ALIGN(8);
    __HeapLimit = .;
    __heap_end = .;
  } > DTCM

  .stack (ORIGIN(DTCM) + LENGTH(DTCM) - __STACK_SIZE) (NOLOAD) : ALIGN(8)
  {
    __StackLimit = .;
    __stack_limit = .;
    . = . + __STACK_SIZE;
    . = ALIGN(8);
    __StackTop = .;
  } > DTCM
  PROVIDE(__stack = __StackTop);

  /* Check if data + heap + stack exceeds RAM limit */
  ASSERT(__StackLimit >= __HeapLimit, "region DTCM overflowed with stack")
  /* Picolibc _start routine provides only single data and bss areas.
       as we have several (itcm/dtcm at least), we have to do the
       copying and zeroing ourselves (see .zero.table and .copy.table),
       same way it's done with GCC by CMSIS.
       These defines are for disabling the picolibc system. */
  PROVIDE(__data_start = 0);
  PROVIDE(__data_source = 0);
  PROVIDE(__data_size = 0);
  PROVIDE(__bss_start = 0);
  PROVIDE(__bss_end = 0);
  PROVIDE(__bss_size = 0);
}
