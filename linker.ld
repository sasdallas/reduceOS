SECTIONS {
    .text 0x1100 : { *(.text) }
    .data : { *(.data) }
    .bss : { *(.bss) }
    end = .;
}