Android
=======

This port should support Android 1.6 (Donut) and above.
Confirmed working on Android 2.1 and 2.2 devices at least.

Dependencies
============

This port depends on having CMake, the Android SDK, and the Android NDK
version r5b or better.

FIXME: NDK versions r8 and r8b don't work - CMake will be unable to determine
    endianness.

Building on Linux
=================

After extracting the Android SDK and NDK to known locations, you need to
setup a standalone NDK toolchain.  Set an environment variable to point to the
desired location of the Android toolchain:

    export TC=$HOME/android-toolchain

Assuming the NDK was extracted into $HOME/android-ndk run the following
command:

    $HOME/android-ndk/build/tools/make-standalone-toolchain.sh \
        --platform=android-9 --install-dir=$TC

You can use any platform 9 or higher. This command was last tested on ndk7.

The following steps will build Allegro and install it to the Android toolchain
directory.

    mkdir build
    cd build
    cmake .. -DANDROID_NDK_TOOLCHAIN_ROOT=$TC -DWANT_ANDROID=on \
        -DWANT_EXAMPLES=OFF -DWANT_DEMO=OFF \
        -DCMAKE_BUILD_TYPE=Debug
    make && make install

Stick with the Debug build configuration for now.

You may want to add -DWANT_MONOLITH=ON if you prefer a single Allegro library
instead of one for each addon.

NOTE: On OS X, add -DCMAKE_INSTALL_NAME_TOOL=/usr/bin/install_name_tool to
the cmake command line.

BIG FAT WARNING: building the examples and demos is not currently supported.

Using Allegro in an Android project
===================================

An Android project has a specific structure which you will need to replicate.
Start by copying the `android-project` folder to a location of your choice.
This will become your project folder.  Several files will need to be modified.

First, change the name of the package.

 *  In `src/org/liballeg/app/AllegroActivity.java` replace the line
    "package org.liballeg.app;" with your own package name.  Usually the
    package name is based on your domain name, in reverse.  You can make
    something up if you do not have a domain name, so long as it doesn't clash
    with an Android or Java package.

    Near the top under the comment /* load allegro */ adjust which addons you
    want to load. FIXME: It should just use all addons which were compiled by
    default.

 *  In `AndroidManifest.xml` replace the value of the "package" property
    in the "manifest" tag, at the top of the file.

 *  Though not recommended, you can also change the name of the AllegroActivity
    and AllegroSurface classes.  Then you must also change the "android:name"
    property in the "activity" tag in `AndroidManifest.xml`.

Next, change the name of your app's executable (library).

 *  In `AndroidManifest.xml` change the value of the
    "meta-data" tag from "allegro-example" to something else.

 *  In `jni/Android.mk` change the LOCAL_MODULE variable to the same
    value as the meta-data tag above.

 *  In `jni/Application.mk`, in the APP_MODULES line, change allegro-example
    to the same value you used above.

 *  In `jni/Android.mk` notice the LOCAL_SRC_FILES variable.
    That is where you tell the Android ndk-build script which files to build
    as part of your project.
 
Next, change the name Android will show for your app:

 *  In `res/values/strings.xml` change the text inside the
    string tag named "app_name".

 *  In `build.xml` change the "name" property of the "project" tag,
    at the top of the file.

Now to build your Android project.

 *  Copy or link the Allegro libraries you require to the `jni/armeabi-v7a`
    folder of your project.

 *  Add the following line to the top of Android.mk:
 
    ANDROID_NDK_TOOLCHAIN_ROOT := <put here the path in $TC>

 *  Adjust Android.mk and Application.mk to only contain the addons you are
    using.

 *  Run the ndk-build script from the NDK while in the root directory of your
    project.

        ANDROID_NDK_TOOLCHAIN_ROOT=$TC /home/username/android-ndk/ndk-build

 *  Run "android update project -p .".

 *  Run `ant debug`.  This should completely build your project
    resulting in an installable `.apk` file.

Finally, install and run your project on your Android device.
We assume you have the adb tool (the Android Debug Bridge) set up, and USB
debugging enabled on your device.  This can be quite involved, so please refer
to the Android tool documentation.

 *  If your app has already been installed once already, you need to uninstall
    it first. That can either be done from the device itself, or using this
    command:

        adb -d uninstall your.package.name

    where "your.package.name" is obviously the package name you previously gave
    to your project. (The -d option is to direct the command to a connected USB
    device.)

 *  To install, use the command:

        adb -d install bin/your-project-name.apk

    where "your-project-name" is the project name you gave in the build.xml
    file.

 *  To reinstall, you can add the `-r` option to the adb install command.

 *  To start your app without touching the device, you can run:

        adb -d shell 'am start -a android.intent.action.MAIN -n your.package.name/.AllegroActivity'

    If you changed it earlier, you should replace "AllegroActivity" with the
    name of your Activity class.

 *  To view the device log, use the command:

        adb logcat

MISCELLANEOUS
=============

You probably want to rename your program something other than org.liballeg.app,
so to help with that, you can pass CMake -DANDROID_APP_NAME=... where you give
it a application/domain name like com.mydomain.myapp.

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
