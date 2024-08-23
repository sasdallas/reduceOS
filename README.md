# reduceOS
[![CodeFactor](https://www.codefactor.io/repository/github/sasdallas/reduceos/badge/rewrite)](https://www.codefactor.io/repository/github/sasdallas/reduceos/overview/rewrite)

Welcome to the in progress development of reduceOS.\
If you would like to learn more about the development of the kernel, scroll down.

![reduceOS image](reduceOSDemo.png)


# What's different from reduceOS alpha?
reduceOS has switched to a brand new kernel, with cleaner, commented code and more features. This kernel is also written entirely by me, without a base.

# Compiling

### IMPORTANT NOTE: YOU MUST HAVE A CROSS-COMPILER (i686-elf) READY FOR YOUR SYSTEM. 
**To build reduceOS, you need these packages:** `i686-elf-gcc`, `nasm`, `make`, `grub`\
**To run reduceOS, you need these packages:** `qemu-system` (emulation), `grub2-common`, and `xorriso`

There are **two targets** available to build reduceOS for - RELEASE and DEBUG.\
To change the target you want to build, edit the `make.config` file to have BUILD_TARGET set to either release or debug.

Then, you can run `make build iso` to build an ISO in `out/iso`.

Run `make targets` to get a full list of targets.

# System Filesystem Structure
On build, the kernel and its libc will place their headers in `source/sysroot`. For the kernel, it will be under `sysroot/usr/include/kernel`, and for libc it will be under `sysroot/usr/include`.

PSF files are placed under `sysroot/usr/fonts/`.

The compiled `libc_reduced.a` is placed under `sysroot/usr/lib`, and this path is included in the compiler options for the kernel.

The initial ramdisk and the kernel are placed under `sysroot/boot/`.

Running `qemu make_drive` will create an ext2 drive with `sysroot` as its filesystem.   


# Kernel Drivers
reduceOS operates on a semi-modular design, which allows for drivers/modules to be easily made and loaded.

I've written an excellent README for the `source/kmods` folder, which can be found [here](https://github.com/sasdallas/reduceOS/blob/main/source/kmods/README.txt) - there is also an example module available.

reduceOS will automatically load and handle your modules without needing to modify the parent directory Makefile.

# Known Bugs
- **Highest priority:** `waitpid()` is not working, process scheduling requires bugfixes
- **High priority:** ACPI not working anymore
- **High priority:** ATAPI drives cannot be read
- **Medium priority:** VMM needs a cleanup and removal of kspace shenanigans
- **Medium Priority:** FDC not working correctly
- **Medium Priority:** Memory dumping doesn't entirely work
- **TODO:** dev/ directory is not present when running `ls`
- **TODO:** DMA in IDE/ATA driver
- **TODO:** System calls need a better interface(?)
- **TODO:** ext2 driver deletion functions suck
- **Likely will never be fixed:** Kernel security is not being upheld with usermode as kernel pages are RW. Not good! (needs to be fixed in allocatePage)
- **Likely will never be fixed:** A bitmap that is too large will crash the system


# Credits
Michael Petch - Patched my terrible interrupts and helped me fix my VMM, even though he didn't have to. Link to this awesome person [here](https://stackoverflow.com/users/3857942/michael-petch)

OSDev Wiki - Great resource for anyone looking into OS development. Helped with a ton of the principles and code. Link [here](https://wiki.osdev.org/)

BrokenThorn Entertainment - Incredible tutorials on kernel design, very useful. Link [here](http://www.brokenthorn.com/Resources/OSDevIndex.html)

ToaruOS - The Misaka kernel and it's modules provide a great example of easy-to-understand, yet robust code. Link [here](https://github.com/klange/ToaruOS)

# To Developers
If I used your code in your repo without properly crediting it, please create a GitHub issue and I will resolve it right away!