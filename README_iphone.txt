IPhone
======

Can use either OpenGL ES 1 or 2 for graphics, by default OpenGL ES 1 is
used.

The accelerometer axes are reported as joystick axes.

Dependencies
------------

For most things no additional dependencies besides the IPhone SDK
are needed.

The download section on liballeg.org has some pre-compiled IPhone
versions of Freetype (.ttf support), Tremor (.ogg support) and
Physfs (.zip) support.

Building
--------

In theory, you build like this:

    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-iphone.cmake -G Xcode ..
    xcodebuild # but does not work without manual changes, see below

However, many things cannot be set up by cmake, so you have to do them
by hand. (An Xcode project named ALLEGRO should be in your build folder
which you can open by double clicking.) Here's a list of things to
adjust:

- Set per-project XCode options (like ipad/iphone target). Some things
  are right now set per-target from cmake so to change those you need to
  edit the CMakeLists.txt.

  Here are some settings you want to change in the Build Settings for
  your project:
  
  * Base SDK
  * Architectures
  * Code Signing Identity
  * Targeted Device Family

- Find the correct libraries (cmake always thinks it is building for
  OSX instead of IPhone). They are all hacked in with linker flags right
  now. The deps folder (see README_cmake.txt) can however make things a
  lot easier. For example if you have freetype for iphone you could do
  this:
  
    mkdir deps
    cp -r my_iphone_freetype/{libs,include} deps

  And cmake should pick it up if they use the standard names.

- Attach resources to a bundle. For the demos and examples you need
  to add the data folder by hand to get them transferred to the
  device.
