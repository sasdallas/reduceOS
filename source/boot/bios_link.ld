OUTPUT_FORMAT("binary")
ENTRY("start")
SECTIONS
{
    . = 0x00010000;

    .text : 
	{
		code = .;
		*(.text)
	}

	.rodata BLOCK(1) : ALIGN(1)
	{
		*(.rodata)
	}

	.data BLOCK(1) : ALIGN(1)
	{
		data = .;
		*(.data)
	}

	.bss BLOCK(1) : ALIGN(1)
	{
		PROVIDE(_bss_start = .);
		bss = .;
		*(COMMON)
		*(.bss)
		*(.stack)
		PROVIDE(_bss_end = .);
	}

	end = .;

	/DISCARD/ :
	{
		*(.comment)
		*(.eh_frame)
		*(.note.gnu.build-id)
	}
}