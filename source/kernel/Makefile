BUILDSCRIPTS_DIR = ../../buildscripts

DEFAULT_HOST != $(BUILDSCRIPTS_DIR)/default_host.sh
HOST ?= $(DEFAULT_HOST)
HOSTARCH != $(BUILDSCRIPTS_DIR)/target_triplet_to_arch.sh  $(HOST)


# Other tools
MKDIR = mkdir
OBJCOPY = $(HOST)-objcopy
CP = cp -F
RM = RM
GRUB_FILE = grub-file

PYTHON = python3 # Should expose this so the user can modify. Sorry!



# Other Directories
OUT_OBJ = ../../obj
OUT_ASMOBJ = $(OUT_OBJ)/assembly
OUT_KERNEL = ../../out/kernel


ASM_SOURCE_DIR = assembly

# Directories (EDIT ME)
SOURCE_DIRECTORIES = kernel libc drivers base gfx fs mem tasks mem/liballoc misc
DIRECTORIES = $(addprefix $(OUT_OBJ)/,$(SOURCE_DIRECTORIES))



# Source files
C_SRCS = $(shell find $(KERNEL_SOURCE) -name "*.c")
C_OBJS = $(patsubst $(KERNEL_SOURCE)/%.c, $(OUT_OBJ)/%.o, $(C_SRCS))

ASM_SRCS = $(wildcard $(ASM_SOURCE_DIR)/*.asm)
ASM_OBJS = $(patsubst $(ASM_SOURCE_DIR)/%.asm, $(OUT_ASMOBJ)/%.o, $(ASM_SRCS))

# Flags


# Targets
# rel - builds a proper kernel.elf file for release
# dbg - builds a kernel.elf  for use with objdump or other examination tools (this target is most commonly used while testing new linking)

all: dbg
rel: $(OUT_KERNEL)/kernel.elf
dbg: $(OUT_KERNEL)/kernel_debug.elf # THIS DOES NOT PRODUCE A KERNEL_DEBUG.ELF FILE!!



$(OUT_KERNEL)/kernel_debug.elf: CONFIG_SH MAKE_OUTPUT_DIRS DELETE_OLD_KERNEL_OBJ BUILDSCRIPTS_DEBUG $(ASM_OBJS) $(C_OBJS) PATCH_TARGETS
	@printf "[ Linking $(OUT_KERNEL)/kernel.elf for DEBUG target... ]\n"
	

CONFIG_SH:
	@printf "[ Configuring reduceOS kernel for target $(HOST)... ]\n"
	$(BUILDSCRIPTS_DIR)/config.sh ../../

MAKE_OUTPUT_DIRS:
	-$(MKDIR) $(DIRECTORIES)


# Fixes a CONFIG.h not updating bug

DELETE_OLD_KERNEL_OBJ:
	-@$(RM) $(OUT_OBJ)/kernel.O 
	-@$(RM) $(OUT_OBJ)/panic.O

BUILDSCRIPTS_RELEASE:
	@printf "[ Running buildscripts for RELEASE build of reduceOS... ]\n"
	$(PYTHON) $(BUILDSCRIPTS_DIR)/update_build_no_and_date.py RELEASE

BUILDSCRIPTS_DEBUG:
	@printf "[ Running buildscripts for DEBUG build of reduceOS... ]\n"
	$(PYTHON) $(BUILDSCRIPTS_DIR)/update_build_no_and_date.py DEBUG

