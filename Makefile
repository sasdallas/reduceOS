ASM = /usr/bin/nasm
CC = /usr/bin/cc
LD = /usr/bin/ld
RM = /usr/bin/rm

SRC = reduceOS
ASM_SRC = $(SRC)/assembly
OBJ = obj
ASM_OBJ = $(OBJ)/asm
INCLUDE = include
INCLUDE_FLAG = -I$(INCLUDE)
CONFIG = config
OUT = build


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


all: $(TARGET)


$(TARGET) : $(OBJS) $(ASMOBJS)
	@printf "[ linking into bin... ]\n"
	$(LD) $(LD_FLAGS) $(OBJS) $(ASMOBJS) -o $(TARGET)
	@printf "\n"

 


$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	@printf "[ Compiling C program $<... ]\n"
	$(CC) $(CC_FLAGS) -c $< -o $@
	@printf "\n"
$(ASM_OBJ)/%.o: $(ASM_SRC)/%.asm | $(ASM_OBJ)
	@printf "[ Compiling ASM program $<... ]\n"
	$(ASM) $(ASM_FLAGS) $< -o $@
	@printf "\n"

clean:
	@printf "[ deleting C objects... ]\n"
	$(RM) $(OBJ)/*.o
	@printf "[ deleting ASM objects... ]\n"
	$(RM) $(ASM_OBJ)/*.o
	@printf "[ deleting build files... ]\n"
	$(RM) -rf $(OUT)/*
	@printf "[ done ]\n"