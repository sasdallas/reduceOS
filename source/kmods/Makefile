include ./make.config

# Source directories
MODDIRS := $(shell find -mindepth 1 -maxdepth 1 -type d)
MODDIRS := $(patsubst ./%, %, $(MODDIRS))


all: TARGET_DISABLED

TARGET_DISABLED:
	@echo "!! THIS TARGET IS DISABLED !!"
	@echo "Please use make install."

install: $(patsubst %, install-%, $(MODDIRS)) ACTUAL_INSTALL
clean: $(patsubst %, clean-%, $(MODDIRS)) ACTUAL_CLEAN

install-%: %
	$(MAKE) -C $< install

clean-%: %
	$(MAKE) -C $< clean

# Hacky
ACTUAL_INSTALL:
	$(PYTHON) $(PROJECT_ROOT)/buildscripts/create_modconfs.py
	cp -r mod_*.conf $(PROJECT_ROOT)/source/initial_ramdisk/rootdir/
	-mkdir -pv $(DESTDIR)$(BOOTDIR)/conf/
	cp -r mod_*.conf $(DESTDIR)$(BOOTDIR)/conf/
	rm *.conf

ACTUAL_CLEAN:
	-rm *.conf