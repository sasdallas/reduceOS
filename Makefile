# Programs
ASM = nasm
CC = gcc
LD = ld
AS = as
GRUB_FILE = grub-file
OBJCOPY = objcopy

RM = rm
MKDIR = mkdir -p
CP = cp -f
echo = echo

DD = dd
qemu-system-x86_64 = qemu-system-x86_64
qemu-img = qemu-img

MKIMAGE = grub-mkrescue

initrd_generator = ./initrd_img/generate_initrd


# Directories
KERNEL_SOURCE = source/kernel
ASM_LOADER = boot/loader
ASM_KERNEL = boot/asmkernel
KLOADER_SOURCE = source/kernel/assembly

OUT_OBJ = obj
OUT_ASMOBJ = obj/assembly
OUT_ASM = out/boot
OUT_KERNEL = out/kernel
OUT_INITRD = out/initrd

INITRD_SRC = initrd_img
INITRD_OBJ = $(INITRD_SRC)/obj
INITRD_DIR = $(INITRD_SRC)/initrd

FONT_SRC = source/fonts
OUT_FONT = $(OUT_OBJ)/fonts

IMAGE_SRC = source/images
OUT_IMAGES = $(OUT_OBJ)/images

# files
OUT_IMG = out/img

# Misc. things
LOOP0 = /dev/loop0
cat = cat


# Flags for compilers
ASM_FLAGS = -f elf32 -F dwarf -g
CC_FLAGS = -m32 -fno-stack-protector -g -std=gnu99 -ffreestanding -fno-pie -Wall -Wextra -I$(KERNEL_SOURCE)/
LD_FLAGS = -m elf_i386 -T linker.ld 


# Source files

# Assembly boot sector and kernel (stage 2 non-C kernel)
ASM_LOADER_SRCS = $(wildcard $(ASM_LOADER)/*.asm)
ASM_KERNEL_SRCS = $(wildcard $(ASM_KERNEL)/*.asm)

ASM_KLOADER = $(wildcard $(KLOADER_SOURCE)/*.asm)
GCC_KLOADER = $(wildcard $(KLOADER_SOURCE)/*.S)
ASM_KLOADEROBJS = $(patsubst $(KLOADER_SOURCE)/%.asm, $(OUT_ASMOBJ)/%.o, $(ASM_KLOADER)) $(patsubst $(KLOADER_SOURCE)/%.S, $(OUT_ASMOBJ)/%.o, $(GCC_KLOADER)) $(OUT_ASMOBJ)/crtbegin.o $(OUT_ASMOBJ)/crtend.o

FONT_SRCS = $(wildcard $(FONT_SRC)/*.psf)
FONT_OBJS = $(patsubst $(FONT_SRC)/%.psf, $(OUT_FONT)/%.o, $(FONT_SRCS))

IMAGE_SRCS = $(wildcard $(IMAGE_SRC)/*.bmp)
IMAGE_OBJS = $(patsubst $(IMAGE_SRC)/%.bmp, $(OUT_IMAGES)/%.o, $(IMAGE_SRCS))

# All LIBC and C sources - this will need an update when we properly organize directories.

C_SRCS = $(wildcard $(KERNEL_SOURCE)/*.c) 

LIBC_SRCS = $(wildcard $(KERNEL_SOURCE)/libc/*.c)
C_OBJS = $(patsubst $(KERNEL_SOURCE)/%.c, $(OUT_OBJ)/%.o, $(C_SRCS)) $(patsubst $(KERNEL_SOURCE)/libc/%.c, $(OUT_OBJ)/libc/%.o, $(LIBC_SRCS))

INITRD_SRCS = $(wildcard $(INITRD_SRC)/src/*.c)
INITRD_OBJS = $(patsubst $(INITRD_SRC)/src/%.c, $(INITRD_OBJ)/%.o, $(INITRD_SRCS))


# Targets
# all - builds a proper kernel.elf file for release
# dbg - builds a kernel file (non-binary) for use with objdump or other examination tools (this target is most commonly used while testing new linking)

all: $(OUT_KERNEL)/kernel.elf $(OUT_INITRD)/initrd.img
dbg: $(OUT_KERNEL)/kernel_debug.elf $(OUT_INITRD)/initrd.img


# I'm sorry for naming this target out/kernel/kernel_debug.elf when it does not produce a kernel_debug.elf
$(OUT_KERNEL)/kernel_debug.elf: $(ASM_KLOADEROBJS) $(C_OBJS) $(FONT_OBJS) $(IMAGE_OBJS) PATCH_TARGETS
	@printf "[ Linking debug kernel... ]\n"
	$(LD) $(LD_FLAGS) $(ASM_KLOADEROBJS) $(C_OBJS) $(FONT_OBJS) $(IMAGE_OBJS) -o $(OUT_KERNEL)/kernel.elf
	@printf "[ Copying debug symbols to kernel.sym... ]\n"
	$(OBJCOPY) --only-keep-debug $(OUT_KERNEL)/kernel.elf $(OUT_KERNEL)/kernel.sym
	@printf "[ Stripping debug symbols... ]\n"
	$(OBJCOPY) --strip-debug $(OUT_KERNEL)/kernel.elf
	@printf "\n"
	



$(OUT_KERNEL)/kernel.elf: $(ASM_KLOADEROBJS) $(C_OBJS) $(FONT_OBJS) $(IMAGE_OBJS) PATCH_TARGETS
	@printf "[ Linking C kernel... ]\n"
	$(LD) $(LD_FLAGS) $(ASM_KLOADEROBJS) $(C_OBJS) $(FONT_OBJS) $(IMAGE_OBJS) -o $(OUT_KERNEL)/kernel.elf
	@printf "[ Stripping debug symbols... ]"
	$(OBJCOPY) --strip-debug $(OUT_KERNEL)/kernel.elf
	@printf "\n"


$(OUT_OBJ)/%.o: $(KERNEL_SOURCE)/%.c | $(OUT_OBJ)
	@printf "[ Compiling C file $<... ]\n"
	@$(CC) $(CC_FLAGS) -c $< -o $@


$(OUT_ASMOBJ)/%.o: $(KLOADER_SOURCE)/%.asm | $(OUT_ASMOBJ)
	@printf "[ Compiling kernel ASM file... ]\n"
	$(ASM) $(ASM_FLAGS) $< -o $@
	@printf "\n"

$(OUT_ASMOBJ)/crtbegin.o $(OUT_ASMOBJ)/crtend.o:
	@printf "[ Copying GCC object $(@F)... ]\n"
	$(CP) $(OUT_OBJ)/CRTFILES/$(@F) $(OUT_ASMOBJ)/
	
#originally:	
#OBJ=`$(CC) -m32 -O2 -g -ffreestanding -print-file-name=$(@F)` && cp "$$OBJ" $@
#@printf "\n"
# i hate this hack but i can't do anything	


$(OUT_ASMOBJ)/%.o: $(KLOADER_SOURCE)/%.S | $(OUT_ASMOBJ)
	@printf "[ Compiling kernel .S file... ]\n"
	$(CC) -m32 -c $< -o $@
	@printf "\n"

$(OUT_FONT)/%.o: $(FONT_SRC)/%.psf | $(OUT_FONT)
	@printf "[ Converting font $< to binary object... ]\n"
	$(OBJCOPY) -O elf32-i386 -B i386 -I binary $< $@
	@printf "\n"

$(OUT_IMAGES)/%.o: $(IMAGE_SRC)/%.bmp | $(OUT_IMAGES)
	@printf "[ Converting bitmap $< to binary object... ]\n"
	$(OBJCOPY) -O elf32-i386 -B i386 -I binary $< $@
	@printf "\n"

img: $(OUT_KERNEL)/kernel.bin $(OUT_INITRD)/initrd.img
	@printf "[ Creating directory for building image... ]\n"
	-@$(MKDIR) $(OUT_IMG)/builddir/boot/grub

	@printf "[ Copying files... ]\n"
	$(CP) $(OUT_KERNEL)/kernel.bin $(OUT_IMG)/builddir/boot/
	$(CP) $(OUT_INITRD)/initrd.img $(OUT_IMG)/builddir/boot/

	@printf "[ Writing GRUB configuration file... ]\n"
	$(RM) $(OUT_IMG)/builddir/boot/grub/grub.cfg

	$(echo) "menuentry "reduceOS" {\n\tmultiboot /boot/kernel.bin\n\tmodule /boot/initrd.img\n}" >> $(OUT_IMG)/builddir/boot/grub/grub.cfg
	
	@printf "[ Creating reduceOS image (requires xorriso!)... ]\n"
	@$(MKIMAGE) -o $(OUT_IMG)/reduceOS.iso $(OUT_IMG)/builddir
	
	@printf "[ Done! You can flash this to a device to run it. ]\n"


	

# Needed for now because some files don't work without -O2
PATCH_TARGETS:
	@printf "[ Recompiling files that need optimization... ]\n"
	$(CC) $(CC_FLAGS) -O2 -c $(KERNEL_SOURCE)/isr.c -o $(OUT_OBJ)/isr.o 
	@printf "\n"
	

$(INITRD_OBJ)/%.o : $(INITRD_SRC)/src/%.c | $(INITRD_OBJ)
	@printf "[ Compiling initrd C file... ]\n"
	@$(CC) -std=c99 -D_BSD_SOURCE -o $@ -c $<
	@printf "\n"

$(INITRD_SRC)/generate_initrd: $(INITRD_OBJS)
	@printf "[ Linking initrd image generator... ]\n"
	@$(CC) -o $(INITRD_SRC)/generate_initrd $(INITRD_OBJS)
	@printf "\n"

$(OUT_INITRD)/initrd.img: $(INITRD_SRC)/generate_initrd
	@printf "[ Generating initrd image... ]\n"
	@$(initrd_generator) -d $(INITRD_DIR) -o $(OUT_INITRD)/initrd.img
	@printf "\n"

qemu:
	@printf "[ Launching QEMU... ]\n"
	@${qemu-system-x86_64} -initrd $(OUT_INITRD)/initrd.img -kernel out/kernel/kernel.elf 

qemu_dbg:
	@printf "[ Launching QEMU with debug options... ]\n"
	@${qemu-system-x86_64} -initrd $(OUT_INITRD)/initrd.img -kernel out/kernel/kernel.elf -serial stdio

qemu_dbg_gdb:
	@printf "[ Launching QEMU with GDB and debug options... ]\n"
	@${qemu-system-x86_64} -initrd $(OUT_INITRD)/initrd.img -kernel out/kernel/kernel.elf -serial stdio -s -S


qemu_drive:
	@printf "[ Launching QEMU with external drive (run make drive to get drive)... ]\n"
	@${qemu-system-x86_64} -initrd $(OUT_INITRD)/initrd.img -kernel out/kernel/kernel.elf -serial stdio -hda test.img

drive:
	@printf "[ Creating drive (requires qemu-img)... ]\n"
	@${qemu-img} create test.img 1M

clean:
	@printf "[ Deleting C object files... ]\n"
	@$(RM) $(OUT_OBJ)/*.o
	@$(RM) $(OUT_OBJ)/libc/*.o
	@printf "[ Deleting ASM object files... ]\n"
	@$(RM) $(OUT_ASMOBJ)/*.o
