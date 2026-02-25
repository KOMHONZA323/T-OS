ENTRY(efi_main)
SECTIONS
{
    . = 0;
    .text : { *(.text) }
    .data : { *(.data) }
    .bss : { *(.bss) }
    .rel.dyn : { *(.rel.dyn) }
    .rela.dyn : { *(.rela.dyn) }
    .reloc : { *(.reloc) }
}
