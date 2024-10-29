# config.sh - Configure the kernel. You shouldn't touch any of this unless you know what you're doing.

HEADER_PROJECTS="libpolyhedron hexahedron"      # Projects to install headers for. This is always done first
PROJECTS="libpolyhedron hexahedron"             # Projects to enter and run make install.

export MAKE=${MAKE:-make}
export HOST=${HOST:-$($BUILDSCRIPTS_ROOT/build-arch.sh)}

export BUILD_ARCH=${BUILD_ARCH:-$($BUILDSCRIPTS_ROOT/target-triplet-to-arch.sh $HOST)}
export BUILD_ARCH_UPPER=$(echo $BUILD_ARCH | tr '[a-z]' '[A-Z]')

# Compile variables
export AR=${HOST}-ar
export AS=${HOST}-as
export CC=${HOST}-gcc
export OBJCOPY=${HOST}-objcopy
export LD=${HOST}-ld

# Python
export PYTHON=echo
command -v python3 > /dev/null && PYTHON=$(command -v python3) || PYTHON=$(command -v python)

# NASM. This shouldn't be used but is here just in case.
export NASM=${NASM:-nasm}

# Sysroot prefixes
export PREFIX=/usr                          # /usr/
export EXEC_PREFIX=$PREFIX                  # /usr/
export BOOT_DIRECTORY=/boot                 # /boot/
export LIB_DIRECTORY=$EXEC_PREFIX/lib       # /usr/lib
export INCLUDE_DIRECTORY=$PREFIX/include    # /usr/include/


# Handle a weird case where PROJECT_ROOT is bad but BUILDSCRIPTS_ROOT is ok
if [ -z "$PROJECT_ROOT" ];
then
    export PROJECT_ROOT="$BUILDSCRIPTS_ROOT/.."
fi

# Configure build directory
export BUILD_OUTPUT_DIRECTORY="$PROJECT_ROOT/build-output"
export OBJ_OUTPUT_DIRECTORY="$BUILD_OUTPUT_DIRECTORY/obj"
export ISO_OUTPUT_DIRECTORY="$BUILD_OUTPUT_DIRECTORY/"
export FILE_OUTPUT_DIRECTORY="$BUILD_OUTPUT_DIRECTORY/output"

# Configure system root
export SYSROOT="$BUILD_OUTPUT_DIRECTORY/sysroot"

# Create the directories just in case they don't exist
mkdir -p "$BUILD_OUTPUT_DIRECTORY/"
mkdir -p "$OBJ_OUTPUT_DIRECTORY/"
mkdir -p "$ISO_OUTPUT_DIRECTORY/"
mkdir -p "$FILE_OUTPUT_DIRECTORY/"
mkdir -p "$SYSROOT/"



# Configure CFLAGS
export CFLAGS="-D__HEXAHEDRON__ -D__REDUCEOS__ -DARCH=$BUILD_ARCH -D__ARCH_${BUILD_ARCH_UPPER}__"
export CFLAGS="$CFLAGS -MD -MP --sysroot=$SYSROOT"

# Work around that -elf targets don't have a sysroot include directory
if echo "$HOST" | grep -Eq -- '-elf($|-)'; then
    export CFLAGS="$CFLAGS -isystem=$INCLUDE_DIRECTORY"
fi

