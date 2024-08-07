#!/bin/sh

# config.sh is called from Makefiles, do not call it yourself
# It must be called with parameter 1 being the root directory of the project (hacky)
# You cannot call config.sh from anywhere else


SYSTEM_HEADER_PROJECTS = "kernel"
PROJECTS = "kernel"

export ROOT=$1


export MAKE=${MAKE:-make}
export HOST=${HOST:-$($ROOT/buildscripts/default_host.sh)}

echo "Configuring target $HOST..."

export AR=${HOST}-ar
export AS=${HOST}-as
export CC=${HOST}-gcc
export LD=${HOST}-ld
export NASM=${NASM:-nasm}

export PREFIX=/usr
export EXEC_PREFIX=$PREFIX
export BOOTDIR=/boot
export LIBDIR=$EXEC_PREFIX/lib
export INCLUDEDIR=$PREFIX/include

export CFLAGS='-O2 -g'
export CPPFLAGS=''

# Configure the cross-compiler to use the desired sysroot
export SYSROOT="$ROOT/source/sysroot"

