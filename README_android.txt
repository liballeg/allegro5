Android
=======

This port should support Android 1.6 (Donut) and above.

Dependencies
============

This port depends on having cmake, the android SDK, and the android ndk
version r5b or better.

Building on Linux
=================

After extracting the android sdk and ndk to known locations, you need to
setup a standalone ndk toolchain.

Assuming the ndk was extracted into $HOME/android-ndk run the following command:

    $ $HOME/android-ndk/build/tools/make-standalone-toolchain.sh \
        --platform=android-9 --install-dir=$HOME/android-toolchain

You can use any platform 9 or higher. This command was last tested on ndk7.

Then to build Allegro run the following from the Allegro's root source directory:

    $ mkdir build
    $ cd build
    $ cmake .. -DANDROID_NDK_TOOLCHAIN_ROOT=$HOME/android-toolchain
        -DWANT_ANDROID=on \
        -DCMAKE_INSTALL_PREFIX=$HOME/android-toolchain/user/armeabi-v7a \
        -DWANT_EXAMPLES=OFF -DWANT_DEMO=OFF
    $ make && make install

BIG FAT WARNING: building the examples and demos is not currently supported.

Using Allegro on Android
========================

Copy the android-project folder to a convenient location.

Several files in the android-project folder will need to be modified.

The first thing that you will want to do is change the name of the package.
To do that, open up the src/org/liballeg/app/AllegroActivity.java and locate
the line containing "package org.liballeg.app;" and change the "org.liballeg.app"
text to something else, usually it is based on your domain name, but reversed.
If you do not have a domain name of your own, you can make something up, so long
as it doesn't clash with a package in android or java. Also you need to open up
the AndroidManifest.xml file, and replace the value of the package property
in the manifest tag.

Optionally you can also change the name of the AllegroActivity and
AllegroSurface classes, however that is not recommended.
If you do change the name of the AllegroActivity class, you must also change
the android:name property in the activity tag in the AndroidManifest.xml file.

Next you will want to change the name of your app's executable (library),
to do that change the value of the meta-data tag in the AndroidManifest.xml
file from allegro-example to whatever you want, you will also need to open up
the jni/Android.mk file and change the LOCAL_MODULE variable to the same value
you gave to the meta-data tag above. While in the jni/Android.mk file,
notice the LOCAL_SRC_FILES variable, that is where you tell the android ndk-build
script which files to build as part of your project.

Now you will also probably want to change the name Android will show for your
app. Open up the res/values/strings.xml file and change the text inside the
string tag named "app_name".

Lastly, you need to change the name property of the project tag in the
build.xml file.

After all that is done, you need to try and build your shiny new android
project.

To build your project, first copy the allegro libraries you are going to use
to the jni/armeabi-v7a folder of your project, then run the ndk-build script
from the ndk from the root directory of your project folder. Say you installed
the ndk into `/home/username/build/android-ndk` you will make sure the current
directory is the root of your project and run:
ANDROID_NDK_TOOLCHAIN_ROOT=pathtotoolchain /home/username/build/android-ndk/ndk-build
where `pathtotoolchain` is the path you installed the standalone toolchain to.
Then run 'ant debug'. that should have completely built your project.

Last but not least, time to install your project on your android device.
If your app has already been installed once already, you need to uninstall it
first. That can either be done from inside android itself, or using the
following command: `adb -d uninstall package.name.here` where
"package.name.here" is the package name you previously gave to your project.
To install, run `adb -d install bin/project-name-here.apk` where
project-name-here is the same value you gave the name property of the project
tag in the build.xml file above.

If you wish to start your app without touching the device, you can run
`adb -d shell 'am start -a android.intent.action.MAIN -n package.name.here/.ActivityNameHere'`
where package.name.here is the package name you gave to your project, and
ActivityNameHere here is the name of the Activity class in the java source
file. Which should normally be AllegroActivity.

TODO
====

* accelerometer support
* screen rotation
* support more than just armv7a
* sound of any kind
* mouse emulation (is this even really needed?)
* joystick emulation (at least till there is a more generic input api)
* properly detecting screen sizes and modes
* filesystem access including SD card, and the app's own package/data folder.
* camera access
* tweak build scripts to handle debug/release versions better
* maybe make some kind of script that makes a customized version of the
  android-project directory from a template, so fewer things need to be edited.
* support static linking allegro if at all possible.
* potential multi display support
