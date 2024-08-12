include ../make2.config

# Module Makefile - just copy and paste this file, it can handle everything.

# Source files
C_SRCS = $(shell find -name "*.c")
C_OBJS = $(patsubst %.c, %.o, $(C_SRCS))

MODOUTPUT = $(shell $(PYTHON) $(PROJECT_ROOT)/buildscripts/get_modoutput.py)

all: TARGET_DISABLED

TARGET_DISABLED:
	@echo "!! THIS TARGET IS DISABLED !!"
	@echo "Please use make install."




# WARNING: CFLAGS is overwritten for some reason..??
%.o: %.c
	$(CC) -ffreestanding -O2 -std=gnu99 -g -static -pedantic -c $< -o $@ -I$(DESTDIR)$(INCLUDE_DIR)


# WARNING: LDFLAGS is overwritten for some reason..??
./$(MODOUTPUT): $(C_OBJS)
	$(LD) -r -o $(MODOUTPUT) $(C_OBJS) 

install: ./$(MODOUTPUT)
	@echo "-- Installing $(MODOUTPUT)..."
	-mkdir -pv $(PROJECT_ROOT)/source/initial_ramdisk/rootdir/modules
	-mkdir -pv $(DESTDIR)$(BOOT_OUTPUT)/modules
	cp -r $(MODOUTPUT) $(PROJECT_ROOT)/source/initial_ramdisk/rootdir/modules/
	cp -r $(MODOUTPUT) $(DESTDIR)$(BOOT_OUTPUT)/modules/
	rm $(MODOUTPUT)

clean:
	@echo "-- Deleting object files..."
	-rm *.o
	-rm *.mod