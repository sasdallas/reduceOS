ENTRY(_start)

SECTIONS
{
    start =.;
    .text 0x100000 :
    {
        text = .;
        *(.text)
        . = ALIGN(4096);
        text_end = .;
    }

    .data :
    {
        data = .;
        *(.data)
        *(.rodata)
        . = ALIGN(4096);
        data_end = .;
    }

    .bss :
    {
        bss = .;
        *(.bss)
        . = ALIGN(4096);
        bss_end = .;
    }

    end = .;
}