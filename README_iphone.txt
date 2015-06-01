iPhone
======

Can use either OpenGL ES 1 or 2 for graphics, by default OpenGL ES 1 is
used.

The accelerometer axes are reported as joystick axes.

Dependencies
------------

The download section on liballeg.org has some pre-compiled iPhone
versions of Freetype (.ttf support), Tremor (.ogg support) and
Physfs (.zip) support.

Make a directory in the root allegro5 directory called deps. Off of that
make an include directory and put your headers there. The structure should
look like:

allegro5
    deps
        include
            allegro5
                <copy this from misc/Allegro 5 iOS>
            freetype2
                <ft2build.h etc here>
            physfs.h

Building using CMake
--------------------

To build for the iOS simulator, run these commands from the build directory:

    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-iphone.cmake -G Xcode \
    -DIOS_PLATFORM="iphonesimulator" ..
    xcodebuild

This will generate the static libraries inside the lib/RelWithDebInfo directory
by default. Note that cmake will occasionally pick up the OSX dependencies,
rather than the iPhone dependencies. If that happens, you may want to disable
the system you don't want via cmake (e.g. -DWANT_PHYSFS=no).

You can also open the XCode project (named ALLEGRO.xcodeproj) and run the
examples.

To build for the iOS device, the procedure is similar. It is easiest to disable
the building of demos, examples and tests as those currently require manual
modification of the generated XCode project (to enable code signing).

    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-iphone.cmake -G Xcode \
    -DIOS_PLATFORM="iphoneos" -DWANT_EXAMPLES=off -DWANT_DEMO=off -DWANT_TESTS=off ..
    xcodebuild

This should generate some ARM libraries in the same location. You might need to
manually modify the generated XCode project to enable more architectures.

Building using a pre-built XCode project
----------------------------------------

If the above doesn't work for you, you can try the pre-built XCode project.

Use the Xcode project in misc/Allegro 5 iOS. By default it will build for the
simulator. You will have to change the target to build for an iOS device. The
project builds armv7/armv7s/arm64 fat binaries. The project is compiled into
a single static library (monolith).

You can find the resulting library in ~/Library/Developer/Xcode/DerivedData.

The Xcode project currently does not build the audio, acodec or video
addons. To build them, add the source files, add addons/audio etc to
your header search path and put the dependency includes in
deps/include.

To install you'll have to copy the library (in DerivedData/*) and the headers
manually. The headers you need are the ones you copied to deps, plus the
ones in allegro5/include plus <addon>/allegro5 for each of the addons.
