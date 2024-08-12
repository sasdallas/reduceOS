SYSTEM_HEADER_PROJECTS="libc_reduced kernel"
PROJECTS="kmods fonts libc_reduced kernel initial_ramdisk"

export MAKE=${MAKE:-make}
export HOST=${HOST:-$($BUILDSCRIPTS_ROOT/default_host.sh)}

echo "-- Configuring target $HOST..."

export AR=${HOST}-ar
export AS=${HOST}-as
export CC=${HOST}-gcc
export LD=${HOST}-ld
export NM=${HOST}-nm
export NASM=${NASM:-nasm}

export PYTHON=echo
command -v python3 > /dev/null && PYTHON=$(command -v python3) || PYTHON=$(command -v python)

export PREFIX=/usr
export EXEC_PREFIX=$PREFIX
export BOOTDIR=/boot
export LIBDIR=$EXEC_PREFIX/lib
export INCLUDEDIR=$PREFIX/include

export CFLAGS=''
export CPPFLAGS=''

# Configure the cross-compiler to use the desired sysroot
export SYSROOT="$PROJECT_ROOT/source/sysroot"
export CC="$CC --sysroot=$SYSROOT"

# Work around that -elf gcc targets don't have a sysroot include directory
if echo "$HOST" | grep -Eq -- '-elf($|-)'; then
    export CC="$CC -isystem=$INCLUDEDIR"
fi