#!/bin/sh

set -e

BUILDSCRIPTS_ROOT=${BUILDSCRIPTS_ROOT:-"$(cd `dirname $0` && pwd)"}
PROJECT_ROOT=${PROJECT_ROOT:-"${BUILDSCRIPTS_ROOT}/.."}

. $BUILDSCRIPTS_ROOT/config.sh

cd $PROJECT_ROOT

# Create the sysroot directory.
mkdir -p "$SYSROOT"

for PROJECT in $HEADER_PROJECTS; do
    (cd $PROJECT_ROOT/$PROJECT && DESTDIR="$SYSROOT" $MAKE install-headers)
done