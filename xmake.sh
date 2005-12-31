#!/bin/sh
#
# This is a helper script for using a cross compiler to build and install
# the Allegro library. It is currently set up to use the MinGW
# cross-compiler out of the box but you can edit XC_PATH and INSTALL_BASE
# to use it with for example a djgpp cross-compiler.

# 1. Put here the path on which the cross compiler and other tools
# for the target will be found with standard names.

XC_PATH=/usr/local/cross-tools/i386-mingw32msvc/bin:/usr/local/cross-tools/bin
XPREFIX=i386-mingw32msvc-

# 2. Put here the path for where things are to be installed.
# You should have created the lib, info and include directories
# in this directory.

INSTALL_BASE=/usr/local/cross-tools/i386-mingw32msvc

# Set up some environment variables and export them to GNU make.

CROSSCOMPILE=1
MINGDIR=$INSTALL_BASE
DJDIR=$INSTALL_BASE
NATIVEPATH=$PATH
PATH=$XC_PATH:$NATIVEPATH

export CROSSCOMPILE MINGDIR DJDIR NATIVEPATH PATH XPREFIX

# Then run make and pass through all command line parameters to it.

make $*
