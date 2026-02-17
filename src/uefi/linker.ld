ENTRY(efi_main)
SECTIONS
{
    . = 0;
    ImageBase = .;

    .text : {
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)
    }

    . = ALIGN(4096);
    .data : {
        *(.rodata*)
        *(.got.plt)
        *(.got)
        *(.data*)
        *(.sdata)
        *(.sbss)
        *(.scommon)
        *(.dynbss)
        *(.bss*)
        *(COMMON)
        *(.rel.local)
    }

    . = ALIGN(4096);
    .dynamic  : { *(.dynamic) }

    . = ALIGN(4096);
    .rela : {
        *(.rela.data*)
        *(.rela.got)
        *(.rela.stab)
    }

    . = ALIGN(4096);
    .reloc : {
        *(.reloc)
    }

    /DISCARD/ : {
        *(.note.GNU-stack)
        *(.eh_frame)
    }
}
