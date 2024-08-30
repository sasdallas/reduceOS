Structure of the source/ directory:
- source/fonts (contains PSF fonts that are linked directly into the kernel and copied to /usr/fonts)
- source/kernel (the main kernel source code and headers)
- source/initial_ramdisk (initial ramdisk generator)
- source/sysroot (this is the directory where kernel & libc headers are installed, and the filesystem is created)
- source/boot (prototype Polyaniline)
- source/libk_reduced (reduceOS version of libc, incomplete)
- source/kmods (kernel modules/drivers folder)
- source/newlib (newlib C library)