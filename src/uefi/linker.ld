ENTRY(efi_main)

SECTIONS
{
    . = 0x400000;
    .text : { *(.text) }
    .data : { *(.data) }
    .rodata : { *(.rodata) }
    .bss : { *(.bss) }
    /DISCARD/ : { *(.comment) *(.eh_frame) *(.note.gnu.build-id) }
}
