Android
=======

This port should support Android 3.1 (Honeycomb) and above.


Dependencies
============

This port depends on having CMake, the Android SDK, and the Android NDK
version r5b or better, JDK, and Apache ant. Python is required to build
the examples.

We assume you are building on Linux or otherwise a Unix-like system,
including MSYS.


Install the SDK
===============


Start by extracting the SDK. You then need to add the SDK to your PATH
environment variable. If you installed the SDK in $HOME/android-sdk, add
$HOME/android-sdk/tools to your PATH.

Once extracted you must run "android" (or android.bat on Windows) and install
a version of the SDK. Allegro needs version 13 (3.1) of the SDK or higher to
build. It can run on version 9 and up still though, if you include the
correct uses-sdk fields in your manifest. See:
https://developer.android.com/guide/topics/manifest/uses-sdk-element.html

Also make sure the JAVA_HOME environment variable is set to your JDK
directory (not always the case.)


Install the NDK
===============


Extract the NDK and add it to your PATH. If you installed the NDK in
$HOME/android-ndk, add $HOME/android-ndk to your PATH.


Make NDK standalone toolchain
=============================

Next you need to setup a standalone NDK toolchain. Set an environment
variable to point to the desired location of the Android toolchain:

    export ANDROID_NDK_TOOLCHAIN_ROOT=$HOME/android-toolchain

Assuming the NDK was extracted into $HOME/android-ndk run the following
command:

    $HOME/android-ndk/build/tools/make-standalone-toolchain.sh \
        --platform=android-9 --install-dir=$ANDROID_NDK_TOOLCHAIN_ROOT

You can use any platform 9 or higher. This command was last tested on ndk10d.
You may need to add --arch=arm if the auto-configuration fails.

Add $ANDROID_NDK_TOOLCHAIN_ROOT/bin to your PATH.


Build dependencies for Allegro
==============================

Now you should build the dependencies for the Allegro addons that you want
(you can skip this if just want to try out some simple examples).  Most of
the libraries use the standard GNU build system, and follow the same pattern.
For example, to build libpng:

    tar zxf libpng-1.6.6.tar.xz
    cd libpng-1.6.6
    ./configure --host=arm-linux-androideabi \
	--prefix=$HOME/allegro/build/deps
    make
    make install

If you get an error during configure about the system being unrecognised then
update the `config.guess` and `config.sub` files.  The files in libpng-1.6.6
are known to work.

The above commands will usually install both static and shared libraries into
the `deps` directory where it can be found by CMake, next.  You could install
into the toolchain directory instead.  If you want only static or shared
libraries, you can usually pass `--disable-static` or `--disable-shared` to
configure.

The static libraries should be easier to use (though I often had problems with
unresolved symbols when it came to run the programs, to investigate later).

If you wish to use shared libraries, be aware that shared objects must be
named like "libFOO.so", despite standard practice.  Most libraries you build
will have version suffixes by default, e.g. libpng16.so.1.6.  Renaming the
file after it is produced will not work as the versions are embedded as the
soname.  For libraries built using libtool, you can avoid the version suffixes
as follows:

    make LDFLAGS=-avoid-version
    make LDFLAGS=-avoid-version install

though you may want to edit the Makefiles instead to avoid overriding
important flags in LDFLAGS.

You need to ensure that CMake finds the the library file that matches the
soname.  Either delete the symlinks (e.g. libpng.so -> libpng16.so)
or modify the paths in CMake variables manually.


Building Allegro
================

The following steps will build Allegro for Android. Note that you still
need ANDROID_NDK_TOOLCHAIN_ROOT (see above) in your environment.

    mkdir build
    cd build
    cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-android.cmake
        -DCMAKE_BUILD_TYPE=Debug -DANDROID_TARGET=android-12 \
        # -G"MSYS Makefiles"
    make

Where you can change ANDROID_TARGET to be something else if you want. This
produces the normal Allegro native libraries (liballegro-*.so) as well as
Allegro5.jar.  You do not need to, but you may run `make install` to install
headers and libraries into the toolchain directory.

You may want to add -DWANT_MONOLITH=ON if you prefer a single Allegro library
instead of one for each addon.

NOTE: On OS X, add -DCMAKE_INSTALL_NAME_TOOL=/usr/bin/install_name_tool to
the cmake command line.


Running examples
================

You need the adb tool (the Android Debug Bridge) set up, and USB debugging
enabled on your device.  This can be quite involved, so please refer to the
Android tool documentation.

There are makefile targets named "run_FOO", so you can install and run
examples easily by typing, e.g.

    make run_speed

Many demos and examples do work, minimally, but most do not support touch
input or react to orientation changes, etc. Good examples to try are
ex_draw_bitmap and ex_touch_input.

If you want to build just the .apk for an example, there are targets
for that as well:

    make ex_draw_bitmap_apk
    adb install -r examples/ex_draw_bitmap.project/bin/ex_draw_bitmap-debug.apk
    adb -d shell 'am start -n org.liballeg.examples.ex_draw_bitmap/.Activity'


How startup works on Android
============================

The startup process begins with your application's main Activity class.
In the static initialiser for the Activity, you must manually load the
shared libraries that you require, i.e. Allegro and its addons,
with dependencies loaded first.  For a C++ program, you may need to load
the shared library of your chosen STL implementation.

After, the onCreate method of the AllegroActivity will be executed, which
does some Allegro initialisation.  Allegro will then load your application
from another shared library.  The library name can be specified by overriding
the constructor in your Activity class, otherwise the default is "libapp.so".
After loading, the `main` function in the library is finally called.

(The dynamic loader was finally fixed in Android 4.3 so that it loads
transitive dependencies.  If you don't care to support earlier versions
you could just load the Allegro core library and have it load your
application library and its linked dependencies automatically.)

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
    the name of the activity ("org.liballeg.example.ExampleActivity").

 *  In `jni/Android.mk` the name of the main module, the list of source
    files, and the list of Allegro libraries that it links to.

 *  In `ExampleActivity.java' the list of shared libraries to load, and
    the name of your application's shared library ("libexample.so").

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
adb should be set up and USB debugging enabled on your device.

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

Android on x86_64
=================

This is very useful to run in the emulator as it will run at native
speed in that case. The procedure is the same as for (32 bit) x86
above.

TODO
====

* accelerometer support
* mouse emulation (is this even really needed?)
* joystick emulation (at least till there is a more generic input api)
* properly detecting screen sizes and modes
* filesystem access including SD card, and the app's own package/data folder.
* camera access
* tweak build scripts to handle debug/release versions better
* support static linking allegro if at all possible.
* potential multi display support
* provide another template which uses gradle as ant is deprecated for
  Android development
* provide a gradle plugin so Allegro can be used from Android Studio
  without the need to compile it yourself

