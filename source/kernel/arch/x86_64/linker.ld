ENTRY(_start)

SECTIONS
{
    . = 1M;
    start =.;

    .text BLOCK(4K) : ALIGN(4K)
    {
        text_start = .;
        *(.multiboot)
        *(.text)
        . = ALIGN(4096);
        text_end = .;
    }


    .rodata BLOCK(4K) : ALIGN(4K)
    {
        rodata_start = .;
        *(.rodata)
        rodata_end = .;
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        data_start = .;
        *(.data)
        data_end = .;
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        bss_start = .;
        *(.bss)
        bss_end = .;
    }

    end = .;
}