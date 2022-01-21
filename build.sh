echo Start build...
mkdir build
echo [COMPILE] boot.s
./i686/bin/i686-elf-as reduceOS/boot.s -o build/boot.o
echo [DONE] boot.s
echo [COMPILE] kernel.c
./i686/bin/i686-elf-gcc -c reduceOS/kernel.c -o build/kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
echo [DONE] kernel.c
echo [LINK] reduce.bin
./i686/bin/i686-elf-gcc -T linker.ld -o build/reduce.bin -ffreestanding -O2 -nostdlib build/boot.o build/kernel.o -lgcc
echo [DONE] reduce.bin
echo [CHECK] reduce.bin

if grub-file --is-x86-multiboot build/reduce.bin; then
    echo [OK] reduce.bin
else
    echo [ERR] reduce.bin
    echo Not multiboot.
    exit
fi

