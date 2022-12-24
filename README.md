# reduceOS - the rewrite
[![CodeFactor](https://www.codefactor.io/repository/github/sasdallas/reduceos/badge/rewrite)](https://www.codefactor.io/repository/github/sasdallas/reduceos/overview/rewrite)

Welcome to the in progress development of the new reduceOS.\
If you would like to learn more about the development of the kernel, scroll down.

![reduceOS image](reduceOSDemo.png)

#### Please read the credits!

# What's different?
I've switched off multiboot in favor of a custom assembly bootloader and kernel. More features are coming in the kernel rewrite along with a ton of flaws and bugs fixed.\
More info coming soon.

# Why does it look so messy?
The assembly code shouldn't look that messy. If it is, start a pull request/contact me and I will update it.

# What's the current stage?
Improving physical memory handling, adding virtual memory handling (or paging handling)

# Compiling
### Again, even though we were having trouble with Linux builds before, Windows builds are NOT supported. Use WSL, mingw-32, or MSys to build.

**To build reduceOS, you need these packages:** `gcc`, `nasm`, `make`, `qemu-tools` (specifically `qemu-img`), and `qemu-system` (for running)

The makefile of reduceOS has two main targets for building - `all` and `dbg`.\
The target to actually build the OS is `all`. If you need to do further debugging, use `dbg` **as well as** `all` (dbg only outputs some debugging symbols)

Run `make` to build the OS, or `make all dbg` if you're trying to debug it.

Now that you have built the OS, you need to write to the image file.\
Run `make img` to create an image and write to it.

Finally, you need to launch the OS. This can be done in a variety of different ways, but the Makefile uses QEMU.\
Run `make qemu` to launch QEMU and start the OS.


# Known Bugs
- Command parser can't handle backspaces. Likely an error with the keyboard driver.
- **Annoying:** Keyboard driver has a hard time keeping up.
- **Will be fixed later:** heap.c has no integration with paging.c - working on it.
- A little bit of disgusting code in `keyboardGetChar()` (unsure how to fix)
- Terminal scrolling can scroll the bottom bar of the screen sometimes (no way to fix easily, but not critical)
- No stack-smashing protector for printf (and a few other functions).

# Credits
OSDev Wiki - Great resource for anyone looking into OS development. Helped with a ton of the basic principles and code. Link [here](https://wiki.osdev.org/)

BrokenThorn Entertainment - Incredible tutorials on kernel design, very useful. Link [here](http://www.brokenthorn.com/Resources/OSDevIndex.html)

StackOverflow - Could never have gotten my custom bootloader without them. Question link [here](https://stackoverflow.com/questions/74172118/how-to-read-sector-into-memory-and-jump-to-it-for-os?noredirect=1#comment131168294_74172118)

JamesM's kernel development tutorials - (no need for Internet Archive anymore) Really helped out with some of the basic concepts. Link [here](http://jamesmolloy.co.uk/tutorial_html/)