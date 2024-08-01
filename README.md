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
**To run reduceOS, you need these packages:** `qemu-system` (emulation), or `grub-common` and `xorriso` (for building an ISO - does not work in QEMU!)

The makefile of reduceOS has two main targets for building - `rel` and `dbg`.\
The target to actually build the OS is `rel`. If you need to do further debugging, use `dbg` (outputs debug symbols)

Run `make rel` to build the OS in a release configuration, or `make dbg` if you're trying to debug it.

Finally, you need to launch the OS. This can be done in a variety of different ways, but the Makefile uses QEMU.\
Run `make qemu` to launch QEMU and start the OS.

**For debug:** You can use `make qemu_dbg` to route serial output to stdio, or you can use `make qemu_dbg_gdb` to output serial to stdio and wait for a GDB connection.

**macOS users:** The default version of gcc and ld included WILL NOT work! You need to use a package manager such as [homebrew](https://brew.sh) to install a custom version of gcc and ld. See below.

You can build reduceOS with a custom version of gcc or ld by passing them as parameters to make, like so: `make CC=<gcc version path> LD=<ld version path>`. See the makefile for all program variables.\


# Known Bugs
- **SEVERE:** Integer overflow bugs render VFS abstraction system in need of repair.
- printf() long handler is disgusting and does not work for signed longs
- ACPI no longer detects table signatures in the RSDT
- initrd is broken, address cannot be found
- **TODO:** ext2 driver needs deletion functions
- A bitmap that is too large will crash the system
- **Severe:** System calls not working.
- **Unsure:** System crashes multiple times when trying to detect VBE modes, but eventually gets it? Unsure if bug with QEMU or code (temporary fix: auto uses 1024*768)



# Credits
Michael Petch - Patched my terrible interrupts and helped me fix my VMM, even though he didn't have to. Link to this awesome person [here](https://stackoverflow.com/users/3857942/michael-petch)

OSDev Wiki - Great resource for anyone looking into OS development. Helped with a ton of the principles and code. Link [here](https://wiki.osdev.org/)

BrokenThorn Entertainment - Incredible tutorials on kernel design, very useful. Link [here](http://www.brokenthorn.com/Resources/OSDevIndex.html)

ToaruOS - The Misaka kernel and it's modules provide a great example of easy-to-understand, yet robust code. Link [here](https://github.com/klange/ToaruOS)

# To Developers
If I used your code in your repo without properly crediting it, please create a GitHub issue and I will resolve it right away!