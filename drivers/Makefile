# Hexahedron drivers makefile
# NOTE: YOU DO NOT NEED TO MODIFY THIS!

include ./make.config

# Source directories
# MODDIRS := $(shell find -mindepth 1 -maxdepth 1 -type d -not -path "./arch")
# MODDIRS := $(patsubst ./%, %, $(MODDIRS))

MODDIRS := $(shell $(PYTHON) $(BUILDSCRIPTS_ROOT)/get_driver_directories.py $(BUILD_ARCH))

all: TARGET_DISABLED

TARGET_DISABLED:
	@echo "!! THIS TARGET IS DISABLED !!"
	@echo "Please use make install."


install-%: %
	$(MAKE) -C $< install

clean-%: %
	$(MAKE) -C $< clean

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

install: PRINT_HEADER MAKE_DIRS $(patsubst %, install-%, $(MODDIRS))
	$(PYTHON) $(BUILDSCRIPTS_ROOT)/create_driver_data.py $(MODDIRS)
	cp  driver_conf.json $(INITRD)/drivers/
	cp  driver_conf.json $(DESTDIR)$(BOOT_OUTPUT)/drivers/
	rm driver_conf.json

clean: $(patsubst %, clean-%, $(MODDIRS))