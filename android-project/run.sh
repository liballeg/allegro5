#!/bin/bash

export ANDROID_NDK_TOOLCHAIN_ROOT=~/code/android-toolchain
if [ "x$1" = "xCLEAN" ] ; then
	V=1 ~/code/android-ndk-r7b/ndk-build clean
fi
V=1 ~/code/android-ndk-r7b/ndk-build

ant clean
ant debug

if [ "x$1" = "xNORUN" ] ; then
	exit 0
fi

#echo uninstall
#/Users/trent/code/android-sdk-macosx/platform-tools/adb -d uninstall org.liballeg.app
echo install
/Users/trent/code/android-sdk-macosx/platform-tools/adb -d install -r bin/alandroid-project-debug.apk
echo start
/Users/trent/code/android-sdk-macosx/platform-tools/adb -d shell 'am start -a android.intent.action.MAIN -n org.liballeg.app/.AllegroActivity'

