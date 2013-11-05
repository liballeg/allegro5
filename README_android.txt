Android
=======

This port should support Android 1.6 (Donut) and above.
Confirmed working on Android 2.1 and 2.2 devices at least.


Dependencies
============

This port depends on having CMake, the Android SDK, and the Android NDK
version r5b or better.


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

The following steps will build Allegro for Android:

    mkdir build
    cd build
    cmake .. -DANDROID_NDK_TOOLCHAIN_ROOT=$TC -DWANT_ANDROID=on \
        -DCMAKE_BUILD_TYPE=Debug
    make

Stick with the Debug build configuration for now.

This produces the normal Allegro native libraries (liballegro-*.so) as
well as Allegro5.jar.  You do not need to, but you may run `make install`
to install headers and libraries into the toolchain directory.

You may want to add -DWANT_MONOLITH=ON if you prefer a single Allegro library
instead of one for each addon.

NOTE: On OS X, add -DCMAKE_INSTALL_NAME_TOOL=/usr/bin/install_name_tool to
the cmake command line.

WARNING: The demos and examples mostly do not run correct yet.


How startup works on Android
============================

The startup process begins with your application's main Activity class.
In the static initialiser for the Activity, you must manually load the
shared libraries that you require, i.e. Allegro and its addons,
with dependencies loaded first.  For a C++ program, you may need to load
the shared library of your chosen STL implementation.

After, the onCreate method of the AllegroActivity will be executed, which
does some Allegro initialisation.  Allegro will now load your application,
which has been compiled to another shared library.  The name of the
library is taken from the AndroidManifest.xml file.  After loading, the
`main` function in the library is finally called.

Poking around in the android/example directory may help.


Using Allegro in an Android project
===================================

An Android project has a specific structure which you will need to
replicate.  You can start by copying android/example and using it as a
base.

This example uses the standard Android build script `ndk-build` to build
the C code, then `ant` to bundle it into an apk file.  You will probably
not avoid ant, but you are not obliged to use ndk-build to compile your
application.  You just need to get your application into a shared library
using the tools from the standalone toolchain that was created earlier,
and fit it into the startup process described above.  The Allegro demos
are worth a look here, as is the script `misc/make_android_project.py`.

Here is what you want to change in the example project:

 *  In `AndroidManifest.xml` the package name ("org.liballeg.example"),
    the name of the activity ("org.liballeg.example.ExampleActivity")
    and the name of your application's shared library ("example").

 *  In `jni/Android.mk` the name of the main module, the list of source
    files, and the list of Allegro libraries that it links to.

 *  In `ExampleActivity.java' the list of shared libraries to load.

 *  In `build.xml` the project name at the top of the file ("example").

 *  In `res/values/strings.xml` change the text inside the
    string tag named "app_name" is the name that Android will show for
    your application.

To build the example project:

 *  Run "android update project -p . --target android-10".
    You may replace "10" with a higher Android SDK number if desired.

 *  Run `ndk-build`.  This builds the main module and copies necessary
    shared libraries into the `libs` directory.

 *  Run `ant debug`.  This bundles the files together into
    an installable `.apk` file.

Now you can install and run your project on your Android device.
We assume you have the adb tool (the Android Debug Bridge) set up, and USB
debugging enabled on your device.  This can be quite involved, so please
refer to the Android tool documentation.

 *  To install, use the command:

        adb -d install -r bin/example.apk

    where "example" is the project name in the build.xml.
    The `-d` option is to direct the command to a connected USB device.
    The `-r` option allows it to reinstall again if necessary.

 *  To start your app without touching the device, you can run:

        adb -d shell 'am start -a android.intent.action.MAIN -n org.liballeg.example/.ExampleActivity'

    Replace the last part as necessary.

 *  You may uninstall with:

        adb -d uninstall org.liballeg.example

 *  To view the device log, use the command:

        adb logcat


Android on x86
==============

It is possible to build Allegro for Android on x86.  Only slightly tested with
NDK r8b and AndroVM (<http://androvm.org/>).  You must get hardware OpenGL
acceleration working in AndroVM or else the Allegro program will crash.

When running creating the toolchain directory, run make-standalone-toolchain.sh
with `--arch=x86`.

When configuring Allegro, run the cmake command with -DARM_TARGETS=x86
XXX Fix the option name.

When building the native libraries, run `ndk-build TARGET_ARCH_ABI=x86`
or change the TARGET_ARCH_ABI=armeabi lines in Android.mk and Application.mk.


TODO
====

* accelerometer support
* support more than just armv7a
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
