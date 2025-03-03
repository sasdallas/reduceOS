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

install:
	@echo "!!!! NEWLIB HAS BEEN DISABLED !!!!"

clean:
	@echo "-- Cleaning up newlib..."
	-rm -rf $(BUILDDIR)/
