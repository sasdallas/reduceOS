# Programs
ASM = nasm
CC = gcc
LD = ld
GRUB_FILE = grub-file

RM = rm
MKDIR = mkdir -p
CP = cp -f

DD = dd
qemu-system-x86_64 = qemu-system-x86_64
QEMU_IMG = qemu-img
MKE2FS = sudo mke2fs

losetup = sudo /sbin/losetup
mount = sudo mount
sudo = sudo
unmount = sudo umount
cp_elevated = sudo cp

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

TMP_MNT = /tmp/mnt

# Files
OUT_IMG = out/img/build.img


# Misc. things
LOOP0 = /dev/loop0



# Flags for compilers
ASM_FLAGS = -f bin
CC_FLAGS = -ffreestanding -O2 -m32 -fno-pie -I$(KERNEL_SOURCE)/ -W
LD_FLAGS = -m elf_i386 -T linker.ld --defsym BUILD_DATE=$(shell date +'%m%d%Y') --defsym BUILD_TIME=$(shell date +'%H%M%S')


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

INITRD_SRCS = $(wildcard $(INITRD_SRC)/src/*.c)
INITRD_OBJS = $(patsubst $(INITRD_SRC)/src/%.c, $(INITRD_OBJ)/%.o, $(INITRD_SRCS))


# Targets
# all - builds a proper kernel.bin file for release
# dbg - builds a kernel file (non-binary) for use with objdump or other examination tools (this target is most commonly used while testing new linking)

all: $(OUT_KERNEL)/kernel.bin $(OUT_INITRD)/initrd.img
dbg: $(OUT_KERNEL)/kernel $(OUT_KERNEL)/kernel.elf $(OUT_INITRD)/initrd.img



$(OUT_KERNEL)/kernel: $(ASM_KLOADEROBJS) $(C_OBJS)
	@printf "[ Linking debug kernel... ]\n"
	$(LD) $(LD_FLAGS) $(ASM_KLOADEROBJS) $(C_OBJS) -o $(OUT_KERNEL)/kernel
	@printf "\n"

$(OUT_KERNEL)/kernel.elf: $(ASM_KLOADEROBJS) $(C_OBJS)
	@printf "[ Linking debug symbols... ]\n"
	$(LD) $(LD_FLAGS) $(ASM_KLOADEROBJS) $(C_OBJS) -o $(OUT_KERNEL)/kernel.elf

$(OUT_KERNEL)/kernel.bin: $(ASM_KLOADEROBJS) $(C_OBJS)
	@printf "[ Linking C kernel... ]\n"
	$(LD) $(LD_FLAGS) $(ASM_KLOADEROBJS) $(C_OBJS) -o $(OUT_KERNEL)/kernel.bin 
	@printf "[ Verifying kernel is valid... ]\n"
	$(GRUB_FILE) --is-x86-multiboot $(OUT_KERNEL)/kernel.bin
	@printf "\n"


$(OUT_OBJ)/%.o: $(KERNEL_SOURCE)/%.c | $(OUT_OBJ)
	@printf "[ Compiling C file $<... ]\n"
	@$(CC) $(CC_FLAGS) -c $< -o $@

$(OUT_ASMOBJ)/%.o: $(KLOADER_SOURCE)/%.asm | $(OUT_ASMOBJ)
	@printf "[ Compiling kernel ASM file... ]\n"
	$(ASM) -f elf32 $< -o $@
	@printf "\n"
	
$(OUT_ASMOBJ)/%.o: $(KLOADER_SOURCE)/%.s | $($OUT_ASMOBJ)
	$(ASM) -f elf32 $< -o $@


img: $(OUT_KERNEL)/kernel.bin $(OUT_INITRD)/initrd.img
	


	@printf "[ Creating image file... ]\n"
	@$(QEMU_IMG) create $(OUT_IMG) 50M
	@printf "\n"
	
	@printf "[ Formatting image file to ext2... ]\n"
	@$(MKE2FS) -F -m0 $(OUT_IMG)
	@printf "\n"

	@printf "[ Removing /dev/loop0 device... ]\n"
	-@$(losetup) -d $(LOOP0)
	@printf "\n"

	@printf "[ Setting up image file on /dev/loop0... ]\n"
	@$(losetup) $(LOOP0) $(OUT_IMG)
	@printf "\n"


	@printf "[ Creating /tmp/mnt directory... ]\n"
	-@$(sudo) $(MKDIR) $(TMP_MNT)
	@printf "\n"

	@printf "[ Mounting /dev/loop0 on /tmp/mnt... ]\n"
	@$(mount) $(LOOP0) $(TMP_MNT)
	@printf "\n"

	@printf "[ Copying files to /tmp/mnt... ]\n"
	$(cp_elevated) $(OUT_KERNEL)/kernel.bin $(TMP_MNT)/kernel
	$(cp_elevated) $(OUT_INITRD)/initrd.img $(TMP_MNT)/initrd
	@printf "\n"

	@printf "[ Unmounting /tmp/mnt... ]\n"
	@$(unmount) $(TMP_MNT)
	@printf "\n"

	@printf "[ Removing loop0 device... ]\n"
	@$(losetup) -d $(LOOP0)
	@printf "\n"

	@printf "[ Done! If you want to run this image file, you should use Bochs (not included) ]\n"

	


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
	@${qemu-system-x86_64} -kernel out/kernel/kernel.bin -initrd $(OUT_INITRD)/initrd.img

clean:
	@printf "[ Deleting assembly binary files... ]\n"
	@$(RM) $(OUT_ASM)/*.bin
	@printf "[ Deleting C object files... ]\n"
	@$(RM) $(OUT_OBJ)/*.o
	@$(RM) $(OUT_OBJ)/libc/*.o
	@printf "[ Deleting ASM object files... ]\n"
	@$(RM) $(OUT_ASMOBJ)/*.o

