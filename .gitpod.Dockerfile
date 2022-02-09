FROM gitpod/workspace-full-vnc
RUN sudo apt-get update && \
    sudo apt-get install -y nasm grub-pc xorriso qemu-system-i386