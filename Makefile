# Programs
ASM = nasm
CC = gcc
LD = ld

RM = rm
MKDIR = mkdir -p
CP = cp -f

dd = dd
qemu-system-x86_64 = qemu-system-x86_64
qemu-img = qemu-img


# Directories
KERNEL_SOURCE = source/kernel
ASM_LOADER = boot/loader
ASM_KERNEL = boot/asmkernel
KLOADER_SOURCE = source/kernel/assembly

OUT_OBJ = obj
OUT_ASMOBJ = obj/assembly
OUT_ASM = out/boot
OUT_KERNEL = out/kernel

# Files
OUT_IMG = out/img/build.img


# Flags for compilers
ASM_FLAGS = -f bin
CC_FLAGS = -ffreestanding -O2 -m32 -fno-pie -I$(KERNEL_SOURCE)/ -W
LD_FLAGS = -m elf_i386 -Ttext 0x1100

# Source files

# Assembly boot sector and kernel (stage 2 non-C kernel)
ASM_LOADER_SRCS = $(wildcard $(ASM_LOADER)/*.asm)
ASM_KERNEL_SRCS = $(wildcard $(ASM_KERNEL)/*.asm)

# ASM_KLOADER includes isr.asm
ASM_KLOADER = $(wildcard $(KLOADER_SOURCE)/*.asm)
ASM_KLOADEROBJS = $(patsubst $(KLOADER_SOURCE)/%.asm, $(OUT_ASMOBJ)/%.o,$(ASM_KLOADER))

# All LIBC and C sources - this will need an update when we properly organize directories.
C_SRCS = $(wildcard $(KERNEL_SOURCE)/*.c) 
LIBC_SRCS = $(wildcard $(KERNEL_SOURCE)/libc/*.c)
C_OBJS = $(patsubst $(KERNEL_SOURCE)/%.c, $(OUT_OBJ)/%.o, $(C_SRCS)) $(patsubst $(KERNEL_SOURCE)/libc/%.c, $(OUT_OBJ)/libc/%.o, $(LIBC_SRCS))




# Targets
# all - builds a proper kernel.bin file for release
# dbg - builds a kernel file (non-binary) for use with objdump or other examination tools (this target is most commonly used while testing new linking)

all: $(OUT_ASM)/loader.bin $(OUT_ASM)/asmkernel.bin  $(OUT_KERNEL)/kernel.bin
dbg: $(OUT_ASM)/loader.bin $(OUT_ASM)/asmkernel.bin $(OUT_KERNEL)/kernel



$(OUT_KERNEL)/kernel: $(ASM_KLOADEROBJS) $(C_OBJS)
	@printf "[ Linking debug kernel... ]\n"
	$(LD) $(LD_FLAGS) $(ASM_KLOADEROBJS) $(C_OBJS) -o $(OUT_KERNEL)/kernel
	@printf "\n"

$(OUT_KERNEL)/kernel.bin: $(ASM_KLOADEROBJS) $(C_OBJS)
	@printf "[ Linking C kernel... ]\n"
	$(LD) $(LD_FLAGS)  --oformat binary $(ASM_KLOADEROBJS) $(C_OBJS) -o $(OUT_KERNEL)/kernel.bin 
	@printf "\n"

$(OUT_OBJ)/%.o: $(KERNEL_SOURCE)/%.c | $(OUT_OBJ)
	@printf "[ Compiling C file $<... ]\n"
	@$(CC) $(CC_FLAGS) -c $< -o $@

$(OUT_ASMOBJ)/%.o: $(KLOADER_SOURCE)/%.asm | $(OUT_ASMOBJ)
	@printf "[ Compiling ASM kernel loader... ]\n"
	$(ASM) -f elf32 $< -o $@
	@printf "\n"
	

$(OUT_ASM)/loader.bin: $(ASM_LOADER_SRCS)
	@printf "[ Compiling bootloader... ]\n"
	$(ASM) $(ASM_FLAGS) $< -o $@
	@printf "\n"

$(OUT_ASM)/asmkernel.bin: $(ASM_KERNEL_SRCS)
	@printf "[ Compiling assembly kernel... ]\n"
	$(ASM) $(ASM_FLAGS) $< -I $(ASM_KERNEL)/ -o $@ 
	@printf "\n"


img: $(OUT_ASM)/asmkernel.bin $(OUT_ASM)/loader.bin $(OUT_KERNEL)/kernel.bin
	@printf "[ Creating image file (requires qemu-img)... ]\n"
	@$(qemu-img) create out/img/reduceOS.img 100M
	@printf "[ Writing boot sector to image... ]\n"
	@$(dd) if=out/boot/loader.bin of=out/img/reduceOS.img bs=512 count=1
	@printf "[ Writing assembly kernel to image... ]\n"
	@$(dd) if=out/boot/asmkernel.bin of=out/img/reduceOS.img bs=512 seek=1
	@printf "[ Writing kernel to image... ]\n"
	@$(dd) if=out/kernel/kernel.bin of=out/img/reduceOS.img bs=512 seek=4
	@printf "[ Done! Run make qemu to execute QEMU... ]\n"

qemu:
	@printf "[ Launching QEMU... ]\n"
	@${qemu-system-x86_64} -hda out/img/reduceOS.img

clean:
	@printf "[ Deleting assembly binary files... ]\n"
	@$(RM) $(OUT_ASM)/*.bin
	@printf "[ Deleting C object files... ]\n"
	@$(RM) $(OUT_OBJ)/*.o
	@$(RM) $(OUT_OBJ)/libc/*.o
	@printf "[ Deleting ASM object files... ]\n"
	@$(RM) $(OUT_ASMOBJ)/*.o

