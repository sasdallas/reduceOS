# Makefile for Newboot (known as Polyaniline)

include ./make.config


# Static directories
OUT_OBJ = $(PROJECT_ROOT)/obj/polyaniline
OUT_EFI = $(PROJECT_ROOT)/obj/polyaniline/efi
OUT_BIOS = $(PROJECT_ROOT)/obj/polyaniline/bios

POLYANILINE_SOURCE_DIRECTORIES = polyaniline

# EFI Directories (EDIT ME)
EFI_SOURCE_DIRECTORIES = efildr
EFI_DIRECTORIES = $(addprefix $(OUT_EFI)/,$(EFI_SOURCE_DIRECTORIES)) $(addprefix $(OUT_EFI)/,$(POLYANILINE_SOURCE_DIRECTORIES))


# BIOS directories (EDIT ME)
BIOS_SOURCE_DIRECTORIES = biosldr
BIOS_DIRECTORIES = $(addprefix $(OUT_OBJ)/,$(BIOS_SOURCE_DIRECTORIES)) $(addprefix $(OUT_BIOS)/,$(POLYANILINE_SOURCE_DIRECTORIES))


# Source files
POLYANILINE_SRCS = $(shell find $(POLYANILINE_SOURCE_DIRECTORIES) -name "*.c")

# Source files (EFI)
EFI_C_SRCS = $(shell find $(EFI_SOURCE_DIRECTORIES) -name "*.c")
EFI_C_OBJS = $(patsubst %.c, $(OUT_EFI)/%.o, $(EFI_C_SRCS)) $(patsubst %.c, $(OUT_EFI)/%.o, $(POLYANILINE_SRCS))

EFI_ASM_SRCS = $(shell find $(EFI_SOURCE_DIRECTORIES) -name "*.asm")
EFI_GAS_SRCS = $(shell find $(EFI_SOURCE_DIRECTORIES) -name "*.S")
EFI_ASM_OBJS = $(patsubst %.asm, $(OUT_EFI)/%.o, $(EFI_ASM_SRCS)) $(patsubst %.S, $(OUT_EFI)/%.o, $(EFI_GAS_SRCS))

# Source files (BIOS)
BIOS_C_OBJS = $(patsubst %.c, $(OUT_BIOS)/%.o, $(POLYANILINE_SRCS)) # No .c files except for polyaniline

#BIOS_ASM_SRCS = $(shell find $(BIOS_SOURCE_DIRECTORIES) -name "*.asm")
#BIOS_GAS_SRCS = $(shell find $(BIOS_SOURCE_DIRECTORIES) -name "*.S")
#BIOS_ASM_OBJS = $(patsubst %.asm, $(OUT_BIOS)/%.o, $(BIOS_ASM_SRCS)) $(patsubst %.S, $(OUT_BIOS)/%.o, $(BIOS_GAS_SRCS))


all: TARGET_DISABLED

TARGET_DISABLED:
	@echo "!! THIS TARGET IS DISABLED !!"
	@echo "Please use make install."


# Compilation functions (EFI)

# NASM compilation
$(OUT_EFI)/%.o: %.asm Makefile
	nasm -f elf64 $< -o $@

# GAS compilation
$(OUT_EFI)/%.o: %.S Makefile
	$(CC) -m32 -c $< -o $@

# .c compilation (polyaniline is totally part of the kernel, dw about it)
$(OUT_EFI)/%.o: %.c Makefile
	gcc $< -g -D__KERNEL__ -w -Ipolyaniline/include/ -c -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I /usr/include/efi/ -I /usr/include/efi/x86_64 -DEFI_FUNCTION_WRAPPER -DPLATFORM_IS_SELECTED -DEFI_PLATFORM -o $@

# Compilation functions (BIOS)
$(OUT_BIOS)/%.o: %.S Makefile
	$(CC) -m32 -c $< -o $@

$(OUT_BIOS)/%.o: %.c Makefile
	$(CC) $(BIOS_C_FLAGS) -w -c $< -o $@

$(OUT_BIOS)/mbr.bin:
	nasm -f bin biosmbr/mbr.asm -o $(OUT_BIOS)/mbr.bin

# Linking functions (EFI)
$(OUT_EFI)/BOOTX64.EFI: $(EFI_C_OBJS) $(EFI_ASM_OBJS)
	ld $(EFI_C_OBJS) $(EFI_ASM_OBJS) $(EFI_LD_FLAGS)  -o $(OUT_EFI)/efi.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 $(OUT_EFI)/efi.so $(OUT_EFI)/BOOTX64.EFI

# Linking functions (BIOS)
$(OUT_BIOS)/boot.sys:
	nasm -f bin -I biosldr/ biosldr/stage1.asm -o $(OUT_BIOS)/boot.sys

#$(OUT_BIOS)/poly.sys: $(BIOS_C_OBJS)
#	$(LD) $(BIOS_C_OBJS) $(BIOS_LD_FLAGS) -o $(OUT_BIOS)/poly.sys

$(OUT_BIOS)/poly.sys:
	@echo "!!!!!!!!!!!! BIOS TARGET DISABLED !!!!!!!!!!!!!!!"
	@echo "-- Polyaniline 2.0 is in active development. Please use that when it is ready."
	@echo "-- Polyaniline 1.0 Redline should most likely be burned to death in a fire"

# Other functions
MAKE_OUTPUT_DIRS:
	-mkdir -pv $(OUT_OBJ)/polyaniline
	-mkdir -pv $(EFI_DIRECTORIES) 
	-mkdir -pv $(OUT_BIOS)
	-mkdir -pv $(BIOS_DIRECTORIES)

# Installation functions

install: clean MAKE_OUTPUT_DIRS $(OUT_EFI)/BOOTX64.EFI $(OUT_BIOS)/boot.sys $(OUT_BIOS)/poly.sys $(OUT_BIOS)/mbr.bin
	#@echo "-- Building FAT image (EFI)..."
	#bash -c "$(BUILDSCRIPTS_ROOT)/mkfatimage.sh"
	#@echo "-- Building FAT image (BIOS)..."
	#bash -c "$(BUILDSCRIPTS_ROOT)/mkfatimage_bios.sh"
	@echo "-- Done building all loaders"
	
# Above is done because it wont detect changes

# Cleaning functions
clean:
	-rm -rf $(OUT_OBJ)

