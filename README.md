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


# Known Bugs
- **Unsure:** initrd doesn't work on QEMU but does on GRUB??
- **TODO:** ext2 driver needs deletion functions
- A bitmap that is too large will crash the system
- **Unsure:** System crashes multiple times when trying to detect VBE modes, but eventually gets it? Unsure if bug with QEMU or code (temporary fix: auto uses 1024*768)



# Credits
Michael Petch - Patched my terrible interrupts and helped me fix my VMM, even though he didn't have to. Link to this awesome person [here](https://stackoverflow.com/users/3857942/michael-petch)

OSDev Wiki - Great resource for anyone looking into OS development. Helped with a ton of the principles and code. Link [here](https://wiki.osdev.org/)

BrokenThorn Entertainment - Incredible tutorials on kernel design, very useful. Link [here](http://www.brokenthorn.com/Resources/OSDevIndex.html)

ToaruOS - The Misaka kernel and it's modules provide a great example of easy-to-understand, yet robust code. Link [here](https://github.com/klange/ToaruOS)

# To Developers
If I used your code in your repo without properly crediting it, please create a GitHub issue and I will resolve it right away!