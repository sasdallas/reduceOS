# Hexahedron drivers makefile
# NOTE: YOU DO NOT NEED TO MODIFY THIS!

include ./make_main.config

# Source directories
# MODDIR_TARGETS_xxx should contain the targets (e.g. install-storage-ahci), and then we can substitute the "-" for "/" in MODDIRS
MODDIRS_RAW := $(shell $(PYTHON) $(BUILDSCRIPTS_ROOT)/get_driver_directories.py $(BUILD_ARCH))
MODDIR_TARGETS_INSTALL := $(patsubst %, install-%, $(MODDIRS_RAW))
MODDIR_TARGETS_CLEAN := $(patsubst %, clean-%, $(MODDIRS_RAW))
MODDIRS := $(subst $e-,/,$(MODDIRS_RAW)) # such weird naming...

# GNU Make does not exit on a $(shell ...) failure
# Better have Make >4.2!
$(if $(filter-out 0,$(.SHELLSTATUS)),$(error Failed to get driver list - nonzero exit code))

all: TARGET_DISABLED

TARGET_DISABLED:
	@echo "!! THIS TARGET IS DISABLED !!"
	@echo "Please use make install."


install-%:
	$(MAKE) -C $(subst $e-,/,$(patsubst install-%, %, $@)) install

clean-%:
	$(MAKE) -C $(subst $e-,/,$(patsubst clean-%, %, $@)) clean

# Create build directories
MAKE_DIRS:
	-@mkdir -pv $(DESTDIR)$(BOOT_OUTPUT)/drivers
	-@mkdir -pv $(INITRD)/drivers
	-@mkdir -pv $(OBJ_OUTPUT_DIRECTORY)/drivers

# Cosmetic
PRINT_HEADER:
	@echo
	@echo
	@echo "[ Building drivers, please wait... ]"
	@echo
	@echo


# Install/clean targets

install: PRINT_HEADER MAKE_DIRS $(MODDIR_TARGETS_INSTALL)
	$(PYTHON) $(BUILDSCRIPTS_ROOT)/create_driver_data.py $(MODDIRS)
	cp  driver_conf.json $(INITRD)/drivers/
	cp  driver_conf.json $(DESTDIR)$(BOOT_OUTPUT)/drivers/
	rm driver_conf.json

clean: $(MODDIR_TARGETS_CLEAN)