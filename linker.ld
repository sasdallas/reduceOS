ENTRY(_start)

SECTIONS
{
    start =.;
    .text 0x100000 :
    {
        text = .;
        *(.text)
        . = ALIGN(4096);
<<<<<<< HEAD
        text_end = .;
=======
>>>>>>> 2f7635386167534d12bbf5bca5f7ae48aee9e6d1
    }

    .data :
    {
        data = .;
        *(.data)
        *(.rodata)
        . = ALIGN(4096);
<<<<<<< HEAD
        data_end = .;
=======
>>>>>>> 2f7635386167534d12bbf5bca5f7ae48aee9e6d1
    }

    .bss :
    {
        bss = .;
        *(.bss)
        . = ALIGN(4096);
<<<<<<< HEAD
        bss_end = .;
=======
>>>>>>> 2f7635386167534d12bbf5bca5f7ae48aee9e6d1
    }

    end = .;
}