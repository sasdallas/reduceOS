.PHONY: all targets clean build help buildkernel buildfonts updateheaders iso qemu qemu_kernel qemu_dbg_gdb qemu_drive make_drive make_drive_blnk

export PROJECT_ROOT = $(CURDIR)
export BUILDSCRIPTS_ROOT = $(PROJECT_ROOT)/buildscripts

targets:
	@echo "Available targets for this makefile:"
	@echo " all			build the OS and package it in an ISO"
	@echo " build			build the project"
	@echo " clean			clean the project"
	@echo " buildkernel		build the kernel module"
	@echo " buildinitrd		build the initial ramdisk module"
	@echo " buildfonts		build the PSF objects for kernel (done by buildkernel)"
	@echo " iso 			build an ISO "
	@echo " updateheaders		installs headers for modules"
	@echo " qemu			launch QEMU for the ISO file"
	@echo " qemu_kernel		launch QEMU for the kernel/initrd files"
	@echo " qemu_dbg_gdb 		launch QEMU with GDB waiting"
	@echo " qemu_drive 		launch QEMU with hda as drive.img"
	@echo " make_drive		create drive.img based off source/sysroot"
	@echo " make_drive_blnk 	create drive.img for size 1G"
	


help: targets

clean:
	$(MAKE) headerlog header="Cleaning build objects, please wait..."
	bash -c "$(BUILDSCRIPTS_ROOT)/clean.sh"
	@echo
	@echo


all: build iso



iso:
	$(MAKE) headerlog header="Building ISO file, please wait..."
	bash -c "$(BUILDSCRIPTS_ROOT)/iso.sh"




# Build targets

build:
	$(MAKE) headerlog header="Building OS, please wait..."
	bash -c "$(BUILDSCRIPTS_ROOT)/build.sh"

buildkernel: buildfonts
	$(MAKE) headerlog header="Building module 'kernel', please wait..."
	bash -c "$(BUILDSCRIPTS_ROOT)/build.sh kernel"


buildinitrd:
	$(MAKE) headerlog header="Building module 'initrd', please wait..."
	bash -c "$(BUILDSCRIPTS_ROOT)/build.sh initial_ramdisk"


buildfonts:
	$(MAKE) headerlog header="Building module 'fonts', please wait..."
	bash -c "$(BUILDSCRIPTS_ROOT)/build.sh fonts"


# Header targets

updateheaders:
	$(MAKE) headerlog header="Updating headers, please wait..."
	bash -c "$(BUILDSCRIPTS_ROOT)/install_headers.sh"



# QEMU Targets

qemu:
	qemu-system-x86_64 -cdrom out/iso/reduceOS.iso -serial stdio

qemu_kernel:
	qemu-system-x86_64 -kernel source/sysroot/boot/kernel.elf -initrd source/sysroot/boot/initrd.img -serial stdio

qemu_dbg: qemu # Muscle memory for me


# QEMU i386 is required because the OS is 32-bit and 64-bit qemu will throw back an invalid reply packet when debugging a 32-bit OS
qemu_dbg_gdb:
	qemu-system-i386 -cdrom out/iso/reduceOS.iso -serial stdio -s -S

qemu_drive:
	qemu-system-x86_64 -cdrom out/iso/reduceOS.iso -serial stdio -hda drive.img


# Other targets
make_drive:
	$(MAKE) headerlog header="Creating QEMU drive from sysroot (requires admin)"
	-qemu-img create drive.img 1G
	-mkfs.ext2 drive.img
	-sudo mkdir /mnt/test/
	sudo mount drive.img /mnt/test/ 
	sudo cp -r source/sysroot/* /mnt/test/
	sudo umount /mnt/test/

make_drive_blnk:
	qemu-img create drive.img 1G

headerlog:
	@echo
	@echo
	@echo "[ $(header) ]"
	@echo
	@echo
