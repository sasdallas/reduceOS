Structure of the source/ directory:
- source/fonts (contains PSF fonts that are linked directly into the kernel)
- source/images (contains test images that are either copied into the kernel itself or added to ext2 images)
- source/kernel (the main kernel source code and headers)
- source/libc (the source of our custom C standard library - KERNEL ONLY! WE LINK WITH LIBGCC!)
- source/sysroot (this is the directory where kernel & libc headers are installed)
