#!/bin/sh
set -e

BUILDSCRIPTS_ROOT=${BUILDSCRIPTS_ROOT:-"$(cd `dirname $0` && pwd)"}
PROJECT_ROOT=${PROJECT_ROOT:-"${BUILDSCRIPTS_ROOT}/.."}


. $(BUILDSCRIPTS_ROOT)/install_headers.sh


echo $PROJECT_ROOT
for PROJECT in $PROJECTS; do
    $(cd $PROJECT && DESTDIR="$SYSROOT" $MAKE)
done