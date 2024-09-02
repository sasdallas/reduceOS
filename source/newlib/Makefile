# reduceOS Newlib Makefile
# Pretty simple stuff to be honest

include make.config

BUILDDIR=newlib-build
SRCDIR=newlib-src

TARGET_ARCH=$(shell $(BUILDSCRIPTS_ROOT)/default_host.sh)
TARGET := $(patsubst %-elf,%,$(TARGET_ARCH))

all: TARGET_DISABLED

TARGET_DISABLED:
	@echo "!! THIS TARGET IS DISABLED !!"
	@echo "Please use make install."

MAKE_DIRS:
	-mkdir -pv $(BUILDDIR)

TEST_REDUCEOS:
	@echo "Testing to see if we have executables for $(TARGET)-reduceOS..."
ifeq ($(shell which $(TARGET)-reduceOS-gcc),)
	@echo "It looks as though you don't have $(TARGET)-reduceOS ready."
	@echo "We can create symlinks for you, you just need to provide permissions."
	sudo ln -sf $(shell which $(TARGET)-elf-gcc) $(dir $(shell which $(TARGET)-elf-gcc))/$(TARGET)-reduceOS-gcc
	sudo ln -sf $(shell which $(TARGET)-elf-gcc) $(dir $(shell which $(TARGET)-elf-gcc))/$(TARGET)-reduceOS-cc
	sudo ln -sf $(shell which $(TARGET)-elf-ar) $(dir $(shell which $(TARGET)-elf-ar))/$(TARGET)-reduceOS-ar
	sudo ln -sf $(shell which $(TARGET)-elf-as) $(dir $(shell which $(TARGET)-elf-as))/$(TARGET)-reduceOS-as
	sudo ln -sf $(shell which $(TARGET)-elf-ranlib) $(dir $(shell which $(TARGET)-elf-ranlib))/$(TARGET)-reduceOS-ranlib
	@echo "Done!"
endif

BUILD_NEWLIB:
	@echo "-- Building newlib..."
ifeq ($(wildcard $(BUILDDIR)/Makefile),)
	(cd $(BUILDDIR) && ../$(SRCDIR)/configure --target=$(TARGET)-reduceOS --prefix /usr CFLAGS= CC=)
endif
	make -C $(BUILDDIR) all

install: MAKE_DIRS TEST_REDUCEOS BUILD_NEWLIB
	@echo "-- Installing newlib headers..."
	make -C $(BUILDDIR) DESTDIR=$(DESTDIR) install
	@echo "-- Copying files..."
	-mkdir -p $(DESTDIR)$(LIBDIR)
	cp -rp $(DESTDIR)/usr/$(TARGET)-reduceOS/include/* $(DESTDIR)$(INCLUDE_DIR)/
	cp -rp $(DESTDIR)/usr/$(TARGET)-reduceOS/lib/* $(DESTDIR)$(LIBDIR)
# The user might want to install the headers into their actual system
ifeq ($(NEWLIB_INSTALL_GLOBAL_HEADERS),TRUE)
	sudo make -C $(BUILDDIR) install
endif

clean:
	@echo "-- Cleaning up newlib..."
	-rm -rf $(BUILDDIR)/
