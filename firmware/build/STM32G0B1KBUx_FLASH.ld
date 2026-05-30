/*
 * STM32G0B1KBUx_FLASH.ld — DSR-1 Linker Script
 *
 * WM-D6C Servo Replacement Module (DSR-1)
 * Target: STM32G0B1KBU6, UFQFPN32 package
 *
 * Memory map:
 *
 *   FLASH   0x08000000  128 KB total
 *           0x08000000  126 KB — firmware code and read-only data
 *           0x0801F000    2 KB — settings sector (last page, RESERVED)
 *
 *   RAM     0x20000000  144 KB — stack, BSS, and initialised data
 *
 * Settings sector reservation:
 *   The last 2 KB page (0x0801F000–0x0801FFFF) is reserved for the
 *   settings block written by flash.c. This region is excluded from the
 *   FLASH region seen by the linker so the compiler can never place code
 *   or read-only data there. If it did, a flash_save() erase would
 *   destroy that code, causing a hard fault on the next execution.
 *
 *   The DFU update process preserves this sector (dfu-util and
 *   STM32CubeProgrammer both stop at the end of the firmware image and
 *   do not erase pages beyond it). Settings survive firmware updates.
 *
 * Stack placement:
 *   Stack grows downward from the top of RAM (0x20024000).
 *   Minimum stack size is 2 KB — sufficient for the ISR + main loop
 *   with comfortable margin. A _Min_Stack_Size symbol allows the linker
 *   to verify stack fits within RAM at link time.
 *
 * Startup sequence requirements:
 *   .data section must be copied from flash (LMA) to RAM (VMA) by startup.s.
 *   .bss section must be zeroed by startup.s before main() is called.
 *   Both are handled by startup_stm32g0b1xx.s.
 */

/* Entry point — reset handler in startup file */
ENTRY(Reset_Handler)

/* Stack and heap sizes — verified at link time */
_Min_Stack_Size = 0x800;   /* 2 KB minimum stack */
_Min_Heap_Size  = 0x000;   /* No heap — all memory is statically allocated */

MEMORY
{
  /* Firmware code region: 126 KB (128 KB total minus 2 KB settings sector) */
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 126K

  /* Settings sector: reserved, NOT available to linker for code placement */
  /* SETTINGS (r) : ORIGIN = 0x0801F000, LENGTH = 2K  -- intentionally absent */

  /* RAM: full 144 KB available */
  RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 144K
}

SECTIONS
{
  /* -------------------------------------------------------------------------
   * .isr_vector — interrupt vector table
   * Must be first in flash at exactly 0x08000000. The Cortex-M0+ loads the
   * initial stack pointer from offset 0 and the reset handler address from
   * offset 4 on power-on. Any other placement causes a hard fault at boot.
   * ----------------------------------------------------------------------- */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* -------------------------------------------------------------------------
   * .text — code and read-only data
   * ----------------------------------------------------------------------- */
  .text :
  {
    . = ALIGN(4);
    *(.text)            /* Code */
    *(.text*)           /* Code in named sections (from -ffunction-sections) */
    *(.rodata)          /* Read-only data (string literals, const arrays) */
    *(.rodata*)

    /* ARM exception table — required even if exceptions are not used */
    KEEP(*(.eh_frame))

    . = ALIGN(4);
    _etext = .;         /* Symbol marking end of text/rodata in flash */
  } >FLASH

  /* -------------------------------------------------------------------------
   * .ARM — exception unwinding tables (minimal, for C++ compatibility)
   * ----------------------------------------------------------------------- */
  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } >FLASH

  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  /* -------------------------------------------------------------------------
   * .preinit_array / .init_array / .fini_array
   * C++ static constructors/destructors and GCC init hooks.
   * Not used in this firmware (no C++, no global constructors) but included
   * for correctness — omitting them causes link warnings with some toolchains.
   * ----------------------------------------------------------------------- */
  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } >FLASH

  /* -------------------------------------------------------------------------
   * .data — initialised variables
   * Stored in flash (LMA = load address), copied to RAM (VMA) at startup.
   * startup_stm32g0b1xx.s copies from _sidata to _sdata.._edata.
   * ----------------------------------------------------------------------- */
  _sidata = LOADADDR(.data);   /* Flash address where .data image is stored */
  /* Computed after .data so ASSERT below can check the true flash end */

  .data :
  {
    . = ALIGN(4);
    _sdata = .;          /* RAM start of .data — used by startup.s */
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;          /* RAM end of .data — used by startup.s */
  } >RAM AT>FLASH

  /* -------------------------------------------------------------------------
   * .bss — uninitialised variables (zero-filled at startup)
   * startup_stm32g0b1xx.s zeroes from _sbss to _ebss before calling main().
   * ----------------------------------------------------------------------- */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;           /* Start of BSS — used by startup.s */
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;           /* End of BSS — used by startup.s */
  } >RAM

  /* -------------------------------------------------------------------------
   * Stack and heap
   *
   * Stack grows downward from the top of RAM. The initial stack pointer is
   * placed at _estack and loaded into the vector table by startup.s.
   *
   * No heap is allocated (_Min_Heap_Size = 0). If heap is ever added,
   * place it between _ebss and the stack guard region.
   *
   * The linker verifies that _ebss + _Min_Stack_Size + _Min_Heap_Size fits
   * within RAM. If it doesn't, the build fails with "region RAM overflowed".
   * ----------------------------------------------------------------------- */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);      /* Newlib sbrk() uses 'end' as heap start */
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* Top of RAM — initial stack pointer loaded by vector table */
  _estack = ORIGIN(RAM) + LENGTH(RAM);   /* 0x20024000 */

  /* -------------------------------------------------------------------------
   * Discard sections not needed in embedded firmware
   * ----------------------------------------------------------------------- */
  /DISCARD/ :
  {
    libc.a(*)
    libm.a(*)
    libgcc.a(*)
  }

  /* -------------------------------------------------------------------------
   * Settings sector guard
   *
   * This ASSERT verifies at link time that the firmware image does not
   * overflow into the reserved settings sector at 0x0801F000.
   * If the firmware grows beyond 126 KB, this check fails with a clear
   * error message rather than silently corrupting the settings sector.
   * ----------------------------------------------------------------------- */
  /* '.' here is a RAM address — must check flash end instead.
   * Flash usage ends at the LMA tail of .data (initialised variables stored in flash). */
  ASSERT(LOADADDR(.data) + SIZEOF(.data) <= 0x0801F000,
    "ERROR: Firmware image exceeds 126 KB and overlaps the settings sector. \
     Reduce firmware size or move the settings sector.")
}
