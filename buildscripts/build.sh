#!/bin/sh
set -e

BUILDSCRIPTS_ROOT=${BUILDSCRIPTS_ROOT:-"$(cd `dirname $0` && pwd)"}
PROJECT_ROOT=${PROJECT_ROOT:-"${BUILDSCRIPTS_ROOT}/.."}


. $BUILDSCRIPTS_ROOT/install_headers.sh


PROJECTS=${1-$PROJECTS}

for PROJECT in $PROJECTS; do
    (cd $PROJECT_ROOT/source/$PROJECT && DESTDIR="$SYSROOT" $MAKE install)
done