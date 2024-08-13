Structure of the source/ directory:
- source/fonts (contains PSF fonts that are linked directly into the kernel and copied to /usr/fonts)
- source/images (contains test images that are either copied into the kernel itself or added to ext2 images)
- source/kernel (the main kernel source code and headers)
- source/initial_ramdisk (initial ramdisk generator)
- source/sysroot (this is the directory where kernel & libc headers are installed, and the filesystem is created)
- source/boot (old, retired boot loader for reduceOS. will not build)
- source/libc_reduced (reduceOS version of libc, incomplete)
- source/kmods (kernel modules/drivers folder)