Android
=======

This port should support Android 3.1 (Honeycomb, API level 13) and above.


Dependencies
============

This port depends on having CMake, the Android SDK, the Android NDK and
a Java JDK.

We assume you are building on Linux or otherwise a Unix-like system,
including MSYS.


Install the SDK
===============


The most simple way is to install Android Studio which by default will
place a copy of the SDK into ~/Android/Sdk. 

Alternatively you can also download the command-line SDK tools. In that
case you will have to accept the licenses, for example like this:

    ~/Android/Sdk/tools/bin/sdkmanager --licenses


Install the NDK
===============


The most simple way is again to use Android Studio. Create a new project
with C++ support and it will ask you if you want to install the NDK and
will then proceed to place it into ~/Android/Sdk/ndk-bundle.

Alternatively you can download the NDK and place anywhere you like.


Java
====


Android Studio comes with a java runtime environment. To use it for
building Android libraries set the evironment variable JAVA_HOME like
this:

export JAVA_HOME=~/android-studio/jre



Build dependencies for Allegro
==============================

Now you should build the dependencies for the Allegro addons that you want
(you can skip this if just want to try out some simple examples). Most of
the libraries use the standard GNU build system, and follow the same pattern.
For example, to build libpng:

    tar zxf libpng-1.6.37.tar.xz
    cd libpng-1.6.37
    # see https://developer.android.com/ndk/guides/other_build_systems
    export ABI=armeabi-v7a
    export HOST=arm-linux-androideabi
    export CHOST=armv7a-linux-androideabi
    export SDK=21
    export HOST_TAG=linux-x86_64
    export PREFIX=$HOME/allegro/build/deps
    export NDK=$HOME/Android/Sdk/ndk-bundle
    export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/$HOST_TAG
    export AR=$TOOLCHAIN/bin/$HOST-ar
    export AS=$TOOLCHAIN/bin/$HOST-as
    export CC=$TOOLCHAIN/bin/$CHOST$SDK-clang
    export CXX=$TOOLCHAIN/bin/$CHOST$SDK-clang++
    export LD=$TOOLCHAIN/bin/$HOST-ld
    export RANLIB=$TOOLCHAIN/bin/$HOST-ranlib
    export STRIP=$TOOLCHAIN/bin/$HOST-strip
    ./configure --host $HOST --prefix $PREFIX
    make
    make install

For HOST_TAG you will want:
    linux-x86_64 if you are using Linux
    darwin-x86_64 in OSX
    windows in 32-bit Windows
    windows-x86_64 in 64-bit Windows

For ABI and HOST you will generally use the following (use a separate build folder for each):

    if ABI == "x86" then HOST = "i686-linux-android"
    if ABI == "x86_64" then HOST = "x86_64-linux-android"
    if ABI == "armeabi-v7" then HOST = "arm-linux-androideabi"
    if ABI == "arm64-v8a" then HOST = "aarch64-linux-android"

CHOST is HOST, except if ABI is armeabi-v7 then CHOST = "armv7a-linux-android".

The above commands will usually install both static and shared libraries into
the `deps` directory where it can be found by CMake, next.  If you want only
static or shared libraries, you can usually pass `--disable-static` or
`--disable-shared` to configure.

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

The following steps will build Allegro for Android. It uses the cmake
toolchain provided by the Android NDK. (Adjust the path if yours is not
under ~/Android/Sdk/ndk-bundle.)

    mkdir build_android_armeabi-v7a
    cd build_android_armeabi-v7a
    cmake .. -DCMAKE_TOOLCHAIN_FILE=~/Android/Sdk/ndk-bundle/build/cmake/android.toolchain.cmake
        -DANDROID_ABI=armeabi-v7a
        -DCMAKE_BUILD_TYPE=RelWithDebInfo
        -DWANT_EXAMPLES=ON
        -DCMAKE_INSTALL_PREFIX=~/allegro/build/deps

You can also use all the normal cmake options supported by Allegro or
run cmake (or cmake-gui) to modify them.

    Finally run:

    make
    make install

Change ANDROID_ABI to whichever architecture you are building for.
The recognized architectures are:

    -DANDROID_ABI="armeabi"
    -DANDROID_ABI="armeabi-v7a"
    -DANDROID_ABI="armeabi-v7a with NEON"
    -DANDROID_ABI="arm64-v8a"
    -DANDROID_ABI="x86"
    -DANDROID_ABI="x86_64"

See here for more information: https://developer.android.com/ndk/guides/abis.html

This produces the normal Allegro native libraries (liballegro-*.so) as
well as allegro-release.aar.

You may want to add -DWANT_MONOLITH=ON if you prefer a single Allegro library
instead of one for each addon.


Running examples
================

You need the adb tool (the Android Debug Bridge) set up, and USB debugging
enabled on your device or emulator. This can be quite involved, so please
refer to the Android tool documentation.

There are makefile targets named "run_FOO", so you can install and run
examples easily by typing, e.g.

    make run_speed

Many demos and examples do work, minimally, but most do not support touch
input or react to orientation changes, etc. Good examples to try are
ex_draw_bitmap and ex_touch_input.

If you want to build just the .apk for an example, there are targets
for that as well:

    make ex_draw_bitmap_apk
    adb -d install -r ./examples/ex_draw_bitmap.project/app/build/outputs/apk/debug/app-debug.apk
    adb -d shell 'am start -n org.liballeg.ex_draw_bitmap/org.liballeg.app.MainActivity'


How startup works on Android
============================

The startup process begins with your application's MainActivity class.
In the static initialiser for the Activity, you must manually load the
shared libraries that you require, i.e. Allegro and its addons,
with dependencies loaded first.  For a C++ program, you may need to load
the shared library of your chosen STL implementation.

After that the onCreate method of the AllegroActivity will be executed, which
does some Allegro initialisation.  Allegro will then load your application
from another shared library.  The library name can be specified by overriding
the constructor in your Activity class, otherwise the default is "libapp.so".
After loading, the `main` function in the library is finally called.

Poking around in the android/example directory may help.


Using Allegro in your game
==========================

If you build with examples or demos, look into your build folder for
any of them, for example

build/demos/speed

It will have a folder called speed.project which is a gradle project
ready to compile for Android. You can use it as a template for your
own game code. (Either by opening it in Android Studio or by using
commandline gradle to compile.)

Read the next section if you would like to create an Android project
using Allegro from scratch.


Using Allegro in a new project
==============================

Start Android Studio.

Note: Android Studio is not strictly required, you can edit the files mentioned
below with any text editor instead of in Android Studio and run ./gradlew instead
of rebuilding from within the IDE.
Android Studio just is usually more convenient to use when porting a game to Android.

On the welcome dialog, select "Start a new Android Studio project".

On the "Create Android Project" screen, make sure to check the
"Include C++ support" checkbox and click Next.

On the "Target Android Devices" screen leave everything at the defaults
and click Next.

On the "Add an Activity to Mobile" screen pick "Empty Activity".

On the "Configure Activity" screen leave the defaults and click Next.

On the "Customize C++ Support" screen leave everything at defaults
and click Finish.

You should be able to click the green arrow at the top and run your
application. If not make sure to fix any problems in your Android
Studio setup - usually it will prompt you to download any missing
components like the NDK or (the special Android) CMake. After that you
should be able to run your new Android app, either in an emulator or on
a real device.

The program we now have already shows how to mix native code and Java
code, it just does not use Allegro yet.

Find MainActivity.java and adapt it to look like this (do not
modify your package line at the top though):

import org.liballeg.android.AllegroActivity;
public class MainActivity extends AllegroActivity {
    static {
        System.loadLibrary("allegro");
        System.loadLibrary("allegro_primitives");
        System.loadLibrary("allegro_image");
        System.loadLibrary("allegro_font");
        System.loadLibrary("allegro_ttf");
        System.loadLibrary("allegro_audio");
        System.loadLibrary("allegro_acodec");
        System.loadLibrary("allegro_color");
    }
    public MainActivity() {
        super("libnative-lib.so");
    }
}

If you used the monolith library, you only need a single
System.loadLibrary for that.

The "import org.liballeg.android.AllegroActivity" line will be red.
Let's fix that. Find the allegro-release.aar from build/lib, where
build is the build folder you used when building Allegro. Open your
Project-level build.gradle and make your "allprojects" section look
like this:

allprojects {
    repositories {
        google()
        jcenter()
        flatDir { dirs 'libs' }
    }
}

Then copy allegro-release.aar into the app/libs folder of your Android
Studio project. For example I did the following:

cp ~/allegro-build/lib/allegro-release.aar ~/AndroidStudioProjects/MyApplication/app/libs/

Now open your app-level build.gradle and add this line inside of the
dependencies section:

implementation "org.liballeg.android:allegro-release:1.0@aar"

On older versions of Android studio use this instead:

compile "org.liballeg.android:allegro-release:1.0@aar"

Next hit the "Sync Now" link that should have appeared at the top of
Android Studio. If you look back at MainActivity.java, nothing should
be red any longer.

Now run your app again.

It will open but crash right away. That is because we are still using
the sample C++ code. Let's instead use some Allegro code. Find the
native-lib.cpp and replace its code with this:

#include <allegro5/allegro5.h>

 int main(int argc, char **argv) {
     al_init();
     auto display = al_create_display(0, 0);
     auto queue = al_create_event_queue();
     auto timer = al_create_timer(1 / 60.0);
     auto redraw = true;
     al_register_event_source(queue, al_get_display_event_source(display));
     al_register_event_source(queue, al_get_timer_event_source(timer));
     al_start_timer(timer);
     while (true) {
         if (redraw) {
             al_clear_to_color(al_map_rgb_f(1, al_get_time() - (int)(al_get_time()), 0));
             al_flip_display();
             redraw = false;
         }
         A5O_EVENT event;
         al_wait_for_event(queue, &event);
         if (event.type == A5O_EVENT_TIMER) {
             redraw = true;
         }
     }
     return 0;
 }

The #include <allegro5/allegro5.h> will be red. Oh no. Again, let's fix
it. Find CMakeLists.txt under External Build Files and add a line like
this:

include_directories(${ANDROID_NDK_TOOLCHAIN_ROOT}/user/{ARCH}/include)

Where ${ANDROID_NDK_TOOLCHAIN_ROOT}/user/{ARCH} should be the path where
the Allegro headers were installed during Allegro's "make install", for
example:

include_directories($HOME/android-toolchain-arm/user/arm/include)

Then add a line like this:

target_link_libraries(native-lib ${ANDROID_NDK_TOOLCHAIN_ROOT}/user/{ARCH}/lib/liballegro.so)

For example:

target_link_libraries(native-lib $HOME/android-toolchain-arm/user/arm/lib/liballegro.so)

Finally, create these folders in your project:

app/src/main/jniLibs/armeabi-v7a
app/src/main/jniLibs/arm64-v8a
app/src/main/jniLibs/x86
app/src/main/jniLibs/x86_64
app/src/main/jniLibs/mips
app/src/main/jniLibs/mips64

And copy the .so files in the corresponding folder for its architecture.

You may have to use "Build->Refresh Linked C++ Projects" for Android
Studio to pick up the CMakeLists.txt changes.

Run the app again. If it worked, congratulations! You just ran your
first Allegro program on Android!

(The sample code will just flash your screen yellow and red with no way to quit, so you will have to force quit it.)
