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

define dump-stack
	x/40x $esp
end

define procinfo
	if !currentProcess
		echo Process scheduling not enabled\n
	else
		printf "Current process: %s\n", currentProcess->name
		printf "\nPROCESS IMAGE INFORMATION:\n"
		printf "Entrypoint: 0x%x Stack: 0x%x\n", currentProcess->image.entrypoint, currentProcess->image.stack
		printf "Heap: 0x%x - 0x%x\n", currentProcess->image.heap_start, currentProcess->image.heap
		printf "Userstack: 0x%x\n", currentProcess->image.userstack
		printf "\nPROCESS THREAD INFORMATION:\n"
		printf "SP = 0x%x BP = 0x%x IP = 0x%x\n", currentProcess->thread.context.sp, currentProcess->thread.context.bp, currentProcess->thread.context.ip
		printf "SAVED = [0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x]\n", currentProcess->thread.context.saved[0], currentProcess->thread.context.saved[1], currentProcess->thread.context.saved[2], currentProcess->thread.context.saved[3], currentProcess->thread.context.saved[4], currentProcess->thread.context.saved[5]
		printf "PAGE DIR: 0x%x\n", currentProcess->thread.page_directory

	end
end

define hex
	printf "0x%x", $arg0
end

define addrok
	print *$arg0
end