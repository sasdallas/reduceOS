#!/bin/bash

# clean.sh - Runs clean script on all projects in $PROJECTS

set -e


BUILDSCRIPTS_ROOT=${BUILDSCRIPTS_ROOT:-"$(cd `dirname $0` && pwd)"}
PROJECT_ROOT=${PROJECT_ROOT:-"$BUILDSCRIPTS_ROOT/.."}

. $BUILDSCRIPTS_ROOT/config.sh

# Take first argument as projects or default to $PROJECTS

PROJECTS=${*-$PROJECTS}

for PROJECT in $PROJECTS; do
        echo
    echo "-- Cleaning ${PROJECT}"
    (cd $BUILDSCRIPTS_ROOT/../source/$PROJECT && $MAKE clean)
done

rm -rf $PROJECT_ROOT/source/sysroot
