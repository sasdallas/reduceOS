# This folder is obsolete - reduceOS has switched to multiboot..
### My apologies in advance. If you're curious about what happened, read below:

# What happened:
The reason the paging driver was breaking was because the kernel was loaded in at 0x1100. Unfortunately, when I tried to load it in at 1 MB (0x100000), it failed. I later learned real mode just can't do that. Unfortunately, I wasn't sure exactly how to even proceed on this, so, sadly, we had to switch to multiboot.
