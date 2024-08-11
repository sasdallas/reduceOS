include ./make.config

# Makefile will thankfully auto-detect the Python executable name
# This file basically just calls the geninitrd python script

# List of directories
INITRD_ROOT = rootdir
GENINITRD_SCRIPT = geninitrd.py
OUTPUT_DIR = .


all: TARGET_DISABLED

TARGET_DISABLED:
	@echo "!! THIS TARGET IS DISABLED !!"
	@echo "Please use make install."

$(OUTPUT_DIR)/ramdisk.img:
	@echo "-- Creating initial ramdisk..."
	$(PYTHON) $(GENINITRD_SCRIPT) $(OUTPUT_DIR)/ramdisk.img $(INITRD_ROOT)

install: $(OUTPUT_DIR)/ramdisk.img
	@echo "-- Copying initial ramdisk to ${BOOT_OUTPUT}..."
	-mkdir -p $(DESTDIR)$(BOOT_OUTPUT)
	cp $(OUTPUT_DIR)/ramdisk.img $(DESTDIR)$(BOOT_OUTPUT)
	rm $(OUTPUT_DIR)/ramdisk.img

clean:
	-rm $(OUTPUT_DIR)/ramdisk.img
