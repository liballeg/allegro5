#!/bin/bash

echo build
ndk-build
ant debug
echo uninstall
adb -d uninstall org.liballeg.app
echo install
adb -d install bin/alandroid-project-debug.apk
echo start
adb -d shell 'am start -a android.intent.action.MAIN -n org.liballeg.example/.ExampleActivity'
