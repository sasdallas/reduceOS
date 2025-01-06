# Hexahedron Makefile for any driver
# Just drop this into your driver system, it will handle everything

include ../make_driver.config

# Output directory
OUTPUT_DIR = $(OBJ_OUTPUT_DIRECTORY)/drivers/$(notdir $(CURDIR))

# Source files
C_SRCS = $(shell find -name "*.c" -printf '%f')
C_OBJS = $(patsubst %.c, $(OUTPUT_DIR)/%.o, $(C_SRCS))

OUTPUT_FILE = $(shell $(PYTHON) $(PROJECT_ROOT)/buildscripts/get_driveroutput.py)

PRINT_HEADER:
	@echo "-- Building driver \"$(OUTPUT_FILE)\"..."

MAKE_OUTPUT:
	-mkdir $(OUTPUT_DIR)

# C compilation
$(OUTPUT_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -I$(DESTDIR)$(INCLUDE_DIR)

./$(OUTPUT_FILE): $(C_OBJS)
	$(LD) $(LDFLAGS) -o $(OUTPUT_FILE) $(C_OBJS)
	

install: PRINT_HEADER MAKE_OUTPUT ./$(OUTPUT_FILE)
	cp -r $(OUTPUT_FILE) $(DESTDIR)$(BOOT_OUTPUT)/drivers
	cp -r $(OUTPUT_FILE) $(INITRD)/drivers/
	rm ./$(OUTPUT_FILE)