# i386.mk - extension for building i386 code

# We'll add a few extra flags for the kernel
LDFLAGS += -melf_i386
CFLAGS += -m32

# We'll also add our directory to the kernel
SOURCE_DIRECTORIES += arch
SOURCE_SUBDIRECTORIES += arch/i386
ASM_SOURCE_DIRECTORIES += arch/i386

