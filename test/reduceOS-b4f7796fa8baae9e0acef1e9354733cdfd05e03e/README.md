# reduceOS - the rewrite
[![CodeFactor](https://www.codefactor.io/repository/github/sasdallas/reduceos/badge/rewrite)](https://www.codefactor.io/repository/github/sasdallas/reduceos/overview/rewrite)

Welcome to the in progress development of the new reduceOS.\
If you would like to learn more about the development of the kernel, scroll down.

![reduceOS image](reduceOSDemo.png)

#### Please read the credits!
# What's different?
reduceOS has switched to a brand new kernel, with cleaner, commented code and more features. This kernel is also written entirely by me, without a base.\
We do still have to stick with multiboot for now, however. Still, the OS is coming along great.

# Why does it look so messy?
The assembly code shouldn't look that messy. If it is, start a pull request/contact me and I will update it.

# What's the current stage?
Adding multitasking and perfecting anniversary (January 20th)

# Compiling
### Again, even though we were having trouble with Linux builds before, Windows builds are NOT supported. Use WSL, mingw-32, or MSys to build.

**To build reduceOS, you need these packages:** `gcc`, `nasm`, `make`, `grub`\
**To run reduceOS, you need these packages:** `qemu-system` (emulation), or `grub-common` and `xorriso` (for building an ISO - does not work in QEMU!)


The makefile of reduceOS has two main targets for building - `all` and `dbg`.\
The target to actually build the OS is `all`. If you need to do further debugging, use `dbg` **as well as** `all` (dbg only outputs some debugging symbols)

Run `make` to build the OS, or `make all dbg` if you're trying to debug it.

Finally, you need to launch the OS. This can be done in a variety of different ways, but the Makefile uses QEMU.\
Run `make qemu` to launch QEMU and start the OS.


# Known Bugs
- ACPI does not initialize, gets stuck in a loop on handling APIC.
- TSS does not initialize properly (it's not needed for now)
- bios32 not working properly, crashes on call.
- **Severe:** Multitasking never calls the handlers - working on a solution.
- When printing BUILD_DATE to the serial console, it is not printed properly. Possibly a bug with printf_putchar's `%u` handler.
- For some reason, `printf()` can't handle a `%x` when paging values are passed. I added some code I found to fix it, it works fine in the function calling it but not in printf. It's not bad, the `%x` operator works fine otherwise. In the meantime, `printf_hex()` has been added to mitigate this.
- A little bit of disgusting code in `keyboardGetChar()`, `commandHandler()`, and a few other functions (unknown how to fix)

# Credits
OSDev Wiki - Great resource for anyone looking into OS development. Helped with a ton of the basic principles and code. Link [here](https://wiki.osdev.org/)

BrokenThorn Entertainment - Incredible tutorials on kernel design, very useful. Link [here](http://www.brokenthorn.com/Resources/OSDevIndex.html)

JamesM's kernel development tutorials - (no need for Internet Archive anymore) Really helped out with some of the basic concepts, especially paging and the initial ramdisk. Link [here](http://jamesmolloy.co.uk/tutorial_html/)

eduOS by RWTH-OS - Helped with multitasking a ton, great resource if you need examples to build off. Link [here](https://github.com/RWTH-OS/eduOS)