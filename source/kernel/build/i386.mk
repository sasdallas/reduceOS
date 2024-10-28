# i386.mk - extension for building i386 code

# We'll add a few extra flags for the kernel
LDFLAGS += -melf_i386
CFLAGS += -m32 -DARCH_I386

# IMPORTANT: YOU CAN ADD -DFORCE_VGA TO THIS TO FORCE NOT TO SET A GRUB FRAMEBUFFER
# This will require you to either clean and rebuild or delete the obj/arch/i386/boot.o file.
NASMFLAGS += -f elf32 -DARCH_I386


# We'll also add our directory to the kernel
SOURCE_DIRECTORIES += arch/i386/
ASM_SOURCE_DIRECTORIES += arch/i386

# Specify the linker.ld file to use
ARCH_LINK_LD := arch/i386/linker.ld
