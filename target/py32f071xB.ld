ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);
__StackTop = _estack;
__StackSize = 0x800;
__StackLimit = __StackTop - __StackSize;

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08002800, LENGTH = 118K
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 16K
}

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(128);
        KEEP(*(.isr_vector))
        . = ALIGN(128);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text .text.*)
        *(.glue_7 .glue_7t)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data .data.*)
        *(.RamFunc .RamFunc.*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    .noinit (NOLOAD) :
    {
        . = ALIGN(4);
        *(.noinit .noinit.*)
        . = ALIGN(4);
    } > RAM

    . = ALIGN(8);
    _end = .;
    PROVIDE(end = .);

    /DISCARD/ :
    {
        *(.comment)
        *(.note .note.*)
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}

ASSERT((ADDR(.isr_vector) & 0x7F) == 0,
       "Vector table must be 128-byte aligned")
ASSERT(SIZEOF(.isr_vector) >= 192,
       "PY32F071 vector table is incomplete")
ASSERT((_ebss + 0x800) <= _estack,
       "RAM overflow: 2 KiB minimum stack reservation not available")
ASSERT((LOADADDR(.data) + SIZEOF(.data)) <=
       (ORIGIN(FLASH) + LENGTH(FLASH)),
       "Application exceeds the configured flash region")
