/*
 * Linker script for the T-OS kernel.
 *
 * The bootloader loads kernel.bin - a flat objcopy of this image - at
 * KERNEL_BASE and jumps straight at the first byte, so _start has to be
 * exactly there. entry.s puts it in .text.boot for that reason; relying on
 * object file order instead would silently break the day a source file is
 * added ahead of entry.s.
 *
 * __kernel_end tells the loader how much memory to reserve and zero: the
 * flat binary carries no .bss, and .bss is where the 4 MB kernel heap lives.
 */

ENTRY(_start)

KERNEL_BASE = 0x1000000;

SECTIONS
{
    . = KERNEL_BASE;

    .text : {
        *(.text.boot)
        *(.text .text.*)
    }

    .rodata : {
        *(.rodata .rodata.*)
    }

    .data : {
        *(.data .data.*)
    }

    .bss : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    __kernel_end = .;

    /DISCARD/ : {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
    }
}
