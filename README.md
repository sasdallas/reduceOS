# reduceOS
[![CodeFactor](https://www.codefactor.io/repository/github/sasdallas/reduceos/badge/rewrite)](https://www.codefactor.io/repository/github/sasdallas/reduceos/overview/rewrite)

Welcome to the in progress development of reduceOS.\
If you would like to learn more about the development of the kernel, scroll down.

![reduceOS image](reduceOSDemo.png)

#### Please read the credits!

# What's different from reduceOS alpha?
reduceOS has switched to a brand new kernel, with cleaner, commented code and more features. This kernel is also written entirely by me, without a base.\
We do still have to stick with multiboot for now, however. Still, the OS is coming along great.

# Compiling
### Windows builds are NOT supported!

### IMPORTANT NOTE: If you are not using a 32-bit (i686) toolchain to build, you MUST download the obj/assembly/crtbegin.o and obj/assembly/crtend.o. They are REQUIRED.
**To build reduceOS, you need these packages:** `gcc`, `nasm`, `make`, `grub`\
**To run reduceOS, you need these packages:** `qemu-system` (emulation), or `grub-common` and `xorriso` (for building an ISO - does not work in QEMU!)

The makefile of reduceOS has two main targets for building - `rel` and `dbg`.\
The target to actually build the OS is `rel`. If you need to do further debugging, use `dbg` (outputs debug symbols)

Run `make rel` to build the OS in a release configuration, or `make dbg` if you're trying to debug it.

Finally, you need to launch the OS. This can be done in a variety of different ways, but the Makefile uses QEMU.\
Run `make qemu` to launch QEMU and start the OS.

**For debug:** You can use `make qemu_dbg` to route serial output to stdio, or you can use `make qemu_dbg_gdb` to output serial to stdio and wait for a GDB connection.

**macOS users:** The default version of gcc and ld included WILL NOT work! You need to use a package manager such as [homebrew](https://brew.sh) to install a custom version of gcc and ld. See below.

You can build reduceOS with a custom version of gcc or ld by passing these as parameters to make, like so: `make CC=<gcc version path> LD=<ld version path>`. See the makefile for all program variables.



# Known Bugs
- **Severe:** `liballoc` is super buggy, but so is the current memory implementation. In other words: memory bad (upd: unsure if bug with liballoc/my wrappers, or with ext2?)
- **Severe:** System calls not working.
- **Unsure:** System crashes multiple times when trying to detect VBE modes, but eventually gets it? Unsure if bug with QEMU or code (temporary fix: auto uses 1024*768)
- Bitmaps can only be displayed at a max of 764 (you can load a 1024x768 image and it will work, but it will only display up to 764 lines).


# Credits
Michael Petch - Patched my terrible interrupts and helped me fix my VMM, even though he didn't have to. Link to this awesome person [here](https://stackoverflow.com/users/3857942/michael-petch)

OSDev Wiki - Great resource for anyone looking into OS development. Helped with a ton of the basic principles and code. Link [here](https://wiki.osdev.org/)

BrokenThorn Entertainment - Incredible tutorials on kernel design, very useful. Link [here](http://www.brokenthorn.com/Resources/OSDevIndex.html)

JamesM's kernel development tutorials - (no need for Internet Archive anymore) Really helped out with some of the basic concepts, especially paging and the initial ramdisk. Link [here](http://jamesmolloy.co.uk/tutorial_html/)

GCC - Basically the entire OS, but some code was copied for the internal libc. Link [here](https://github.com/gcc-mirror/gcc)

# To Developers
If you see your code in my repository and do not wish it to be there, please notify me to have it removed or to add credit! I am so sorry if you aren't in the credits list, I forget sometimes.
Thank you for all you do, I appreciate it greatly!
