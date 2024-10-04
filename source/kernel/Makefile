include ./make.config

# Other tools
MKDIR = mkdir
CP = cp -F
RM = rm
GRUB_FILE = grub-file

# Source directories

# Other Directories
OUT_OBJ = $(PROJECT_ROOT)/obj
OUT_ASMOBJ = $(OUT_OBJ)/assembly
OUT_KERNEL = .

ASM_SOURCE_DIRECTORIES = assembly


# Directories (EDIT ME)
SOURCE_DIRECTORIES = kernel drivers gfx fs mem tasks misc
SOURCE_SUBDIRECTORIES = mem/liballoc kernel/doomgeneric # We do this because shell find will pick up anything in subdirectories twice


# Before object files and other things are done, we need to include our architecture-specific Makefile.
BUILD_ARCH ?= $(shell $(BUILDSCRIPTS_ROOT)/target_triplet_to_arch.sh $(shell $(BUILDSCRIPTS_ROOT)/default_host.sh))
include build/$(BUILD_ARCH).mk


DIRECTORIES = $(addprefix $(OUT_OBJ)/,$(SOURCE_DIRECTORIES)) $(addprefix $(OUT_OBJ)/,$(SOURCE_SUBDIRECTORIES))







# Source files
C_SRCS = $(shell find $(SOURCE_DIRECTORIES) -name "*.c")
C_OBJS = $(patsubst %.c, $(OUT_OBJ)/%.o, $(C_SRCS))

ASM_SRCS = $(shell find $(ASM_SOURCE_DIRECTORIES) -name "*.asm")
ASM_SRCS2 = $(shell find $(ASM_SOURCE_DIRECTORIES) -name "*.S")
ASM_OBJS = $(patsubst %.asm, $(OUT_OBJ)/%.o, $(ASM_SRCS)) $(patsubst %.S, $(OUT_OBJ)/%.o, $(ASM_SRCS2)) $(OUT_ASMOBJ)/crtbegin.o $(OUT_ASMOBJ)/crtend.o

DIRECTORIES += $(addprefix $(OUT_OBJ)/,$(ASM_SOURCE_DIRECTORIES))


# Targets
# kernel_release.elf - builds a proper kernel.elf file for release
# kernel_debug.elf - builds a kernel.elf  for use with objdump or other examination tools (this target is most commonly used while testing new linking)

all: TARGET_DISABLED

TARGET_DISABLED:
	@echo "!! THIS TARGET IS DISABLED !!"
	@echo "Please use make install."



$(OUT_KERNEL)/kernel_debug.elf:  MAKE_OUTPUT_DIRS DELETE_OLD_KERNEL_OBJ BUILDSCRIPTS_DEBUG PRINT_ASM_HEADER $(ASM_OBJS) PRINT_C_HEADER $(C_OBJS)
	@echo "-- Linking $(OUT_KERNEL)/kernel.elf for DEBUG target..."
	$(LD) $(LDFLAGS) -T $(ARCH_LINK_LD) $(C_OBJS) $(ASM_OBJS) $(OUT_OBJ)/fonts/*.o -o kernel.elf $(LIBS)

$(OUT_KERNEL)/kernel_release.elf:  MAKE_OUTPUT_DIRS DELETE_OLD_KERNEL_OBJ BUILDSCRIPTS_RELEASE PRINT_ASM_HEADER $(ASM_OBJS) PRINT_C_HEADER $(C_OBJS)
	@echo "-- Linking $(OUT_KERNEL)/kernel.elf for RELEASE target..."
	$(LD) $(LDFLAGS) -T $(ARCH_LINK_LD) $(C_OBJS) $(ASM_OBJS) $(OUT_OBJ)/fonts/*.o -o kernel.elf $(LIBS)

	@echo "-- Stripping debug symbols..."
	objcopy --strip-debug kernel.elf


# Other targets for the Makefile

MAKE_OUTPUT_DIRS:
	@-$(MKDIR) -pv $(DIRECTORIES)


# Fixes a CONFIG.h not updating bug
# Not good
DELETE_OLD_KERNEL_OBJ:
	-@$(RM) $(OUT_OBJ)/kernel/config.o

BUILDSCRIPTS_RELEASE:
	@printf "[ Running buildscripts for RELEASE build of reduceOS... ]\n"
	$(PYTHON) $(BUILDSCRIPTS_ROOT)/update_build_no_and_date.py RELEASE

BUILDSCRIPTS_DEBUG:
	@printf "[ Running buildscripts for DEBUG build of reduceOS... ]\n"
	$(PYTHON) $(BUILDSCRIPTS_ROOT)/update_build_no_and_date.py DEBUG

PATCH_TARGETS:
	@echo
	@echo "--- Applying patches to reduceOS files..."


# These are just quick hacks to make things look nicer
PRINT_C_HEADER:
	@echo "-- Compiling C files..."

PRINT_ASM_HEADER:
	@echo "-- Compiling assembly..."


# Actual compilation functions

$(OUT_OBJ)/%.o: %.asm Makefile
	$(NASM) $(NASMFLAGS) $< -o $@

$(OUT_OBJ)/%.o: %.S Makefile
	$(CC) -m32 -c $< -o $@

$(OUT_OBJ)/%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@


$(OUT_ASMOBJ)/crtbegin.o $(OUT_ASMOBJ)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

# Installation functions

install-headers:
	@echo "-- Installing kernel headers..."
	-mkdir -p $(DESTDIR)$(INCLUDE_DIR)
	cp -R --preserve=timestamps include/kernel/. $(DESTDIR)$(INCLUDE_DIR)/.

install: $(OUT_KERNEL)/kernel_$(TARGET).elf
	@echo "-- Installing kernel..."
	-mkdir -p $(DESTDIR)$(BOOT_OUTPUT)
	cp $(OUT_KERNEL)/kernel.elf $(DESTDIR)$(BOOT_OUTPUT) 

	@echo "-- Creating symbol map..."
	$(NM) kernel.elf > kernel.map
	cp kernel.map $(PROJECT_ROOT)/source/initial_ramdisk/rootdir/
	rm kernel.map
	rm kernel.elf



# Clean function
clean:
	-$(RM) -rf $(DIRECTORIES)
	-$(RM) $(OUT_ASMOBJ)/*.o
