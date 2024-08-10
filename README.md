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
**To run reduceOS, you need these packages:** `qemu-system` (emulation), or `grub-common` and `xorriso`

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


# Known Bugs
- **Severe:** Systems with >1G break the allocator.
- initrd should be replaced with a known-good impl. instead of something I wrote a few years ago
- FDC not working correctly
- **TODO:** dev/ directory is not present when running `ls`
- ATAPI drives cannot be read
- **TODO:** DMA in IDE/ATA driver
- **TODO:** System calls need a better interface(?)
- Kernel security is not being upheld with usermode as kernel pages are RW. Not good! (needs to be fixed in allocatePage)
- **TODO:** ext2 driver deletion functions suck
- A bitmap that is too large will crash the system



# Credits
Michael Petch - Patched my terrible interrupts and helped me fix my VMM, even though he didn't have to. Link to this awesome person [here](https://stackoverflow.com/users/3857942/michael-petch)

OSDev Wiki - Great resource for anyone looking into OS development. Helped with a ton of the principles and code. Link [here](https://wiki.osdev.org/)

BrokenThorn Entertainment - Incredible tutorials on kernel design, very useful. Link [here](http://www.brokenthorn.com/Resources/OSDevIndex.html)

ToaruOS - The Misaka kernel and it's modules provide a great example of easy-to-understand, yet robust code. Link [here](https://github.com/klange/ToaruOS)

# To Developers
If I used your code in your repo without properly crediting it, please create a GitHub issue and I will resolve it right away!