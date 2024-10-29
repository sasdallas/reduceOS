# Hexahedron Makefile - Adapted from reduceOS Makefile

include ./make.config

.PHONY: all targets clean build help

targets:
	@echo "Available targets for this makefile:"
	@echo " all			build the kernel and package it in an ISO"
	@echo " build			build the project (DOES NOT PACKAGE INTO ISO)"
	@echo " clean			clean the project"
	@echo " iso 			build an ISO "

help: targets


all: build iso


iso:
	$(MAKE) headerlog header="Packing ISO, please wait..."
	bash -c "$(BUILDSCRIPTS_ROOT)/iso.sh"
	@echo
	@echo

build:
	$(MAKE) headerlog header="Building OS, please wait..."
	bash -c "$(BUILDSCRIPTS_ROOT)/build.sh"
	@echo
	@echo
	@echo "[ Finished building. You may need to pack it into an ISO file. ]"

clean:
	$(MAKE) headerlog header="Cleaning build, please wait..."
	bash -c "$(BUILDSCRIPTS_ROOT)/clean.sh"
	@echo
	@echo
	@echo "[ Finished cleaning. ]"


headerlog:
	@echo
	@echo
	@echo "[ $(header) ]"
	@echo
	@echo