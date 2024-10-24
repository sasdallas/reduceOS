# reduceOS
[![CodeFactor](https://www.codefactor.io/repository/github/sasdallas/reduceos/badge/rewrite)](https://www.codefactor.io/repository/github/sasdallas/reduceos/overview/rewrite)

Welcome to the in progress development of reduceOS.\
If you would like to learn more about the development of the kernel, scroll down.

![reduceOS image](reduceOSDemo.png)


# Features
- VESA/VBE graphics drivers
- Filesystem support for EXT2, FAT, and many other UNIX-style device.s
- Support for kernel drivers
- Strong memory manager with liballoc and sbrk support for kernel
- Preemptive multitasking
- ELF file support
- Port of Newlib


# Building

### BUILD REQUIREMENTS
**YOU WILL NEED A CROSS COMPILER (i686 / x86_64) TO BUILD REDUCEOS**

reduceOS requires `i686-elf-gcc`, `make`, `grub-mkrescue`, `xorriso`\
You can emulate reduceOS with `qemu-system`

Run `make targets` for a list of targets. To compile reduceOS fully, run `make all`.

To run reduceOS, you can use the command `make qemu`.\
To use a drive with reduceOS, run `make make_drive` to create a drive and `make qemu_drive` to run QEMU with the drive.


# System Filesystem Structure
On build, the kernel and its libc will place their headers in `source/sysroot`. For the kernel, it will be under `sysroot/usr/include/kernel`, and for libk it will be under `sysroot/usr/include/libk_reduced`.

Newlib will place its header files under `sysroot/usr/include/`. **Under no circumstances should this folder be used in modules**

PSF files are placed under `sysroot/usr/fonts/`.

The compiled `libk_reduced.a` is placed under `sysroot/usr/lib`, and this path is included in the compiler options for the kernel.

The initial ramdisk and the kernel are placed under `sysroot/boot/`.

Running `qemu make_drive` will create an ext2 drive with `sysroot` as its filesystem.   

# Real Hardware
Scary! I wouldn't do this, but it's possible. Here are some issues I've encountered:

**Modules such as the prototype AHCI driver will fail to load.** Use the boot argument `--skip_modules=<filename>` to skip these modules. This should work.

**Tasking (last stage) crashes the system** The boot argument `--no_tasking` should help here. Be warned the system will be unstable!

**There is no video driver, but I have a COM port!** Use the boot argument `--headless` to put reduceOS into serial-only mode.

If you manage to get reduceOS running, then tell me in our Discord server or in GitHub issues!

# Kernel Drivers
reduceOS operates on a semi-modular design, which allows for drivers/modules to be easily made and loaded.

I've written an excellent README for the `source/kmods` folder, which can be found [here](https://github.com/sasdallas/reduceOS/blob/main/source/kmods/README.txt) - there is also an example module available.

reduceOS will automatically load and handle your modules without needing to modify the parent directory Makefile.

# Known Bugs
*These are going to be moved to GitHub issues.*
- **Highest priority:** New memory subsystem not working with tasking (this is entirely tasking's fault)
- **High priority:** `valloc` sucks
- **High priority:** ATAPI drives cannot be read
- **Requires further debugging:** Loading bitmaps cause more memory leaks than the Titanic had, and if you try to free buffers they crash. 
- **TODO:** Merge USB driver handling into kernel
- **TODO:** device/ directory is not present when running `ls`
- **TODO:** DMA in IDE/ATA drivers
- **Likely will never be fixed:** A bitmap that is too large will crash the system


# Credits
Michael Petch - Patched my terrible interrupts and helped me fix my VMM, even though he didn't have to. Link to this awesome person [here](https://stackoverflow.com/users/3857942/michael-petch)

OSDev Wiki - Great resource for anyone looking into OS development. Helped with a ton of the principles and code. Link [here](https://wiki.osdev.org/)

BrokenThorn Entertainment - Incredible tutorials on kernel design, very useful. Link [here](http://www.brokenthorn.com/Resources/OSDevIndex.html)

ToaruOS - The Misaka kernel and it's modules provide a great example of easy-to-understand, yet robust code. Link [here](https://github.com/klange/ToaruOS)

# Licensing
reduceOS is licensed under the BSD 3-clause license. This license falls under kernel code, kernel modules, libk, polyaniline, and any other source-code elements specific to reduceOS.\
For more information on reduceOS licensing, see the LICENSE file.

Other components will usually have a licensing file included, also called LICENSE, in their directories.

If a copyright issue arises, start a GitHub issue or contact me directly for assistance. Thank you!