# x86_64.mk - extension for building i386 code

# We'll add a few extra flags for the kernel
LDFLAGS += -melf_x86_64
CFLAGS += -m64 -DARCH_X86_64


# IMPORTANT: YOU CAN ADD -DFORCE_VGA TO THIS TO FORCE NOT TO SET A GRUB FRAMEBUFFER
# This will require you to either clean and rebuild or delete the obj/arch/x86_64/boot.o file.
NASMFLAGS += -f elf64 -DARCH_X86_64


# We'll also add our directory to the kernel
SOURCE_DIRECTORIES += arch
SOURCE_SUBDIRECTORIES += arch/x86_64
ASM_SOURCE_DIRECTORIES += arch/x86_64

# Specify the linker.ld file to use
ARCH_LINK_LD := arch/x86_64/linker.ld