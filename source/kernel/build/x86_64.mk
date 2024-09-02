# i386.mk - extension for building i386 code

# We'll add a few extra flags for the kernel
LDFLAGS += -melf_x86_64
CFLAGS += -m64

# We'll also add our directory to the kernel
SOURCE_DIRECTORIES += arch
SOURCE_SUBDIRECTORIES += arch/x86_64
ASM_SOURCE_DIRECTORIES += arch/x86_64

# Specify the linker.ld file to use
ARCH_LINK_LD := arch/x86_64/linker.ld