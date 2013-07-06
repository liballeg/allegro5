#!/bin/bash

rsync --progress --exclude='*.svn*' -r -t ~/projects/allegro-5.1/android-project/ ~/projects/alandroid-project/

cp ~/build/android-toolchain/user/armeabi-v7a/lib/liballegro-debug.so jni/armeabi-v7a/
cp ~/build/android-toolchain/user/armeabi-v7a/lib/liballegro_primitives-debug.so jni/armeabi-v7a/
cp ~/build/android-toolchain/user/armeabi-v7a/lib/liballegro_image-debug.so jni/armeabi-v7a/

export ANDROID_NDK_TOOLCHAIN_ROOT=~/build/android-toolchain
V=1 ~/build/android-ndk/ndk-build clean
V=1 ~/build/android-ndk/ndk-build

ant clean
ant debug

echo uninstall
adb -d uninstall org.liballeg.app
echo install
adb -d install bin/alandroid-project-debug.apk
echo start
adb -d shell 'am start -a android.intent.action.MAIN -n org.liballeg.app/.AllegroActivity'

