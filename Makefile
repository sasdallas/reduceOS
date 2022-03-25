ASM = /usr/bin/nasm
CC = /usr/bin/cc
LD = /usr/bin/ld
RM = /usr/bin/rm
GRUB = /usr/bin/grub-mkrescue
MKDIR = /usr/bin/mkdir
CP = /usr/bin/cp

SRC = reduceOS
ASM_SRC = $(SRC)/assembly
OBJ = obj
ASM_OBJ = $(OBJ)/asm
INCLUDE = include
INCLUDE_FLAG = -I$(INCLUDE)
CONFIG = config
OUT = build
ISODIR = $(OUT)/isodir/

MKDIR = mkdir -p
CP = cp -f

ASM_FLAGS = -f elf32
CC_FLAGS = $(INCLUDE_FLAG) -m32 -std=gnu99 -ffreestanding -Wall -Wextra
LD_FLAGS = -m elf_i386 -T $(CONFIG)/linker.ld -nostdlib


SRCS    := $(wildcard $(SRC)/*.c)
OBJS    := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
ASMS	:= $(wildcard $(ASM_SRC)/*.asm)
ASMOBJS := $(patsubst $(ASM_SRC)/%.asm,$(ASM_OBJ)/%.o,$(ASMS))




TARGET = $(OUT)/reduce.bin
TARGET_ISO = $(OUT)/reduce.iso

all: $(TARGET)


$(TARGET) : $(OBJS) $(ASMOBJS)
	@printf "[ linking into bin... ]\n"
	@$(LD) $(LD_FLAGS) $(OBJS) $(ASMOBJS) -o $(TARGET)
	
	@printf "[ testing for x86 multiboot... ]\n"
	@grub-file --is-x86-multiboot $(TARGET)
	@printf "[ packing into ISO... ]\n"
	@$(MKDIR) -pv $(ISODIR)/boot/grub/
	@$(CP) $(TARGET) $(ISODIR)/boot/
	@$(CP) $(CONFIG)/grub.cfg $(ISODIR)/boot/grub/
	@$(GRUB) /usr/lib/grub/i386-pc -o $(TARGET_ISO) $(ISODIR)
	@printf "[ done compiling reduceOS ]\n"
	@printf "\n"
 


$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	@printf "[ Compiling C program $<... ]\n"
	@$(CC) $(CC_FLAGS) -c $< -o $@
	@printf "\n"
$(ASM_OBJ)/%.o: $(ASM_SRC)/%.asm | $(ASM_OBJ)
	@printf "[ Compiling ASM program $<... ]\n"
	@$(ASM) $(ASM_FLAGS) $< -o $@
	@printf "\n"

clean:
	@printf "[ deleting C objects... ]\n"
	$(RM) $(OBJ)/*.o
	@printf "[ deleting ASM objects... ]\n"
	$(RM) $(ASM_OBJ)/*.o
	@printf "[ deleting build files... ]\n"
	$(RM) -rf $(OUT)/*
	@printf "[ done ]\n"