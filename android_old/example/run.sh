#!/bin/bash -e

echo build
ndk-build
ant debug
echo install
adb -d install -r bin/example-debug.apk
echo start
adb -d shell 'am start -a android.intent.action.MAIN -n org.liballeg.example/.ExampleActivity'
