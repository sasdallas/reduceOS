# Makefile for Newboot (known as Polyaniline)

include ./make.config


# Static directories
OUT_OBJ = $(PROJECT_ROOT)/obj/polyaniline
OUT_EFILDR = $(PROJECT_ROOT)/obj/polyaniline/efildr


# Directories (EDIT ME)
SOURCE_DIRECTORIES = efildr polyaniline
DIRECTORIES = $(addprefix $(OUT_OBJ)/,$(SOURCE_DIRECTORIES))

# Assembly directories
ASM_SOURCE_DIRECTORIES = biosldr
ASM_DIRECTORIES = $(addprefix $(OUT_OBJ)/, $(ASM_SOURCE_DIRECTORIES))

MBR_SOURCE = biosmbr/mbr.asm

# Source files
C_SRCS = $(shell find $(SOURCE_DIRECTORIES) -name "*.c")
C_OBJS = $(patsubst %.c, $(OUT_OBJ)/%.o, $(C_SRCS))

ASM_SRCS = $(shell find $(SOURCE_DIRECTORIES) -name "*.asm")
GAS_SRCS = $(shell find $(SOURCE_DIRECTORIES) -name "*.S")
ASM_OBJS = $(patsubst %.asm, $(OUT_OBJ)/%.o, $(ASM_SRCS)) $(patsubst %.S, $(OUT_OBJ)/%.o, $(GAS_SRCS))

all: TARGET_DISABLED

TARGET_DISABLED:
	@echo "!! THIS TARGET IS DISABLED !!"
	@echo "Please use make install."


# Compilation functions


# NASM compilation (FIXME)
$(OUT_OBJ)/%.o: %.asm Makefile
	nasm -f elf64 $< -o $@

# GAS compilation
$(OUT_OBJ)/%.o: %.S Makefile
	$(CC) -m32 -c $< -o $@

# .c compilation
$(OUT_OBJ)/%.o: %.c Makefile
	gcc $< -Ipolyaniline/include/ -c -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I /usr/include/efi/ -I /usr/include/efi/x86_64 -DEFI_FUNCTION_WRAPPER -o $@

# MBR compilation 
$(OUT_OBJ)/biosmbr/mbr.bin:
	$(NASM) -f bin $(MBR_SOURCE) -o $(OUT_OBJ)/biosmbr/mbr.bin


# Linking functions

$(OUT_EFILDR)/BOOTX64.EFI: $(C_OBJS) $(ASM_OBJS)
	$(LD) $(C_OBJS) $(ASM_OBJS) $(LDFLAGS)  -o $(OUT_EFILDR)/efi.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 $(OUT_EFILDR)/efi.so $(OUT_EFILDR)/BOOTX64.EFI

# Other functions
MAKE_OUTPUT_DIRS:
	-mkdir -pv $(OUT_OBJ)/polyaniline
	-mkdir -pv $(DIRECTORIES) 

# Installation functions

install: MAKE_OUTPUT_DIRS $(OUT_EFILDR)/BOOTX64.EFI
	@echo "Compilation successful!"

# Cleaning functions
clean:
	-rm -rf $(DIRECTORIES)/*.o
	-rm -rf $(OUT_EFILDR)/BOOTX64.EFI
	-rm -rf $(OUT_EFILDR)/efi.so

