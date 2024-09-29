file source/sysroot/boot/kernel.elf
target remote :1234

define debug-elf
	# GDB can use add-symbol-file <file> <.text offset>, and that can be calculated with readelf
	# This just makes my life easier!
	# https://stackoverflow.com/questions/20380204/how-to-load-multiple-symbol-files-in-gdb
	# Parse the .text address into a temporary file (that's a bash script)
	shell echo set \$text_address=$(readelf -WS $arg0 | grep .text | awk '{print "0x"$5 }') > /tmp/temp_gdb_text_address.txt
	
	# Load it and delete the temporary file
	source /tmp/temp_gdb_text_address.txt
	shell rm -f /tmp/temp_gdb_text_address.txt

	# Load the symbol file
	add-symbol-file $arg0 $text_address
end

define debug-module
	add-symbol-file $arg0 $arg1
end