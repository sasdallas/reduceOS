FROM gitpod/workspace-full
# Start install of required tools.
# Deprecated: GitPod now allows apt-get in the cmd line.
# REPLACED WITH tools.sh

RUN echo 'Installing build tools...'
RUN sudo apt-get update && sudo apt-get install -y gcc binutils nasm python python3 perl sed diffutils qemu
RUN echo "Cleaning up..."
RUN sudo rm -rf /var/lib/apt/lists

