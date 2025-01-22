=-=-=-=-=-=-=-=-=-=-=-=-=-=-= HEXAHEDRON DRIVER SYSTEM =-=-=-=-=-=-=-=-=-=-=-=-=-=-=

==== I. Overview

Hexahedron is designed as a modular kernel, where the kernel has basic device stacks (e.g. audio stack, USB stack),
and the drivers contain the actual interface code for talking to the devices.

The driver loader of Hexahedron is activated once all of the basic structures (HAL, VFS, initrd, and all other driver stacks)
have been initialized. Then, it loads an auto-generated JSON file from the initial ramdisk and loads the drivers.

==== II. Writing a device driver

To write a device driver, create a new folder anywhere. Then you'll want to copy the standard Makefile from any other driver, like the "example" driver.
After, create your driver.conf file. It should have four fields:
- FILENAME: The driver filename. Should ALWAYS end in ".sys".
- ENVIRONMENT: See III. Environments (NORMAL if you're not sure)
- PRIORITY: How important your driver is - dictates how the kernel will handle your driver failing to load. Can be CRITICAL, WARN, and IGNORE
- ARCH: What architectures your kernel is compatible with. Use "OR" directives to separate architectures, and use "ANY" to specify all architectures.

Example "driver.conf" file:
FILENAME = "example_driver.sys"
ENVIRONMENT = ANY
PRIORITY = WARN
ARCH = I386 OR X86_64


If you make the folder in a subdirectory, be sure to copy the standard subdirectory make.config into the subdirectory if it's not already there.
The above statement is weird, so let me explain:
- Drivers will include the make.config file in the previous directory.
- This make.config file will include the make.config file in the previous directory, continuing until it reaches the root make.config
TL;DR:  If you make a driver in a new subdirectory (e.g. audio/randomsoundcard), then copy make.config (e.g. from storage/make.config if you want)
        to your new subdirectory (in this case copy to audio/make.config)

==== III. Environments

Driver environments refers to the environment of the kernel when the driver is loaded.
This matters because the kernel does not have to load the drivers at the same time. Instead,
it can do what I call a "preload", where the bootloader (Polyaniline) passes loaded drivers as Multiboot modules.

The kernel can load these Multiboot modules really early in the boot process, but that means that the drivers do not
have all of their fancy devices loaded, just the bare minimum.

Exact specifications are to do. For now, always bet on NORMAL.