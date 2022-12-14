# Programs
ASM = nasm
CC = gcc
LD = ld

RM = rm
MKDIR = mkdir -p
CP = cp -f

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
LD_FLAGS = -m elf_i386 -Ttext 0x1000 --oformat binary

# Source files
ASM_LOADER_SRCS = $(wildcard $(ASM_LOADER)/*.asm)
ASM_KERNEL_SRCS = $(wildcard $(ASM_KERNEL)/*.asm)

ASM_KLOADER = $(wildcard $(KLOADER_SOURCE)/*.asm)
ASM_KLOADEROBJS = $(patsubst $(KLOADER_SOURCE)/%.asm, $(OUT_ASMOBJ)/%.o,$(ASM_KLOADER))

C_SRCS = $(wildcard $(KERNEL_SOURCE)/*.c) 
LIBC_SRCS = $(wildcard $(KERNEL_SOURCE)/libc/*.c)
C_OBJS = $(patsubst $(KERNEL_SOURCE)/%.c, $(OUT_OBJ)/%.o, $(C_SRCS)) $(patsubst $(KERNEL_SOURCE)/libc/%.c, $(OUT_OBJ)/libc/%.o, $(LIBC_SRCS))




# Targets

all: $(OUT_ASM)/loader.bin $(OUT_ASM)/asmkernel.bin  $(OUT_KERNEL)/kernel.bin


$(OUT_KERNEL)/kernel.bin: $(ASM_KLOADEROBJS) $(C_OBJS)
	@printf "[ Linking C kernel... ]\n"
	$(LD) $(LD_FLAGS) $(ASM_KLOADEROBJS) $(C_OBJS) -o $(OUT_KERNEL)/kernel.bin
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


clean:
	@printf "[ Deleting assembly binary files... ]\n"
	@$(RM) $(OUT_ASM)/*.bin
	@printf "[ Deleting C object files... ]\n"
	@$(RM) $(OUT_OBJ)/*.o
	@$(RM) $(OUT_OBJ)/libc/*.o
	@printf "[ Deleting ASM object files... ]\n"
	@$(RM) $(OUT_ASMOBJ)/*.o

