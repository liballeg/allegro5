IPhone
======

This uses OpenGL ES for graphics. The touch screen is implemented as a
mouse, with multi-touch triggering different mouse buttons.

The accelerometer axes are reported as joystick axes.

Build like this:

mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-iphone.cmake -G XCode ..
xcodebuild

Many things cannot be set up by cmake though, so you have to do them
by hand:

- Set per-project XCode options (like ipad/iphone target). They are
  right now set per-target so to change it you need to edit the
  CMakeLists.txt.

- Find the correct libraries (cmake always things it is building for
  OSX instead of IPhone). They are all hacked in with linker flags right
  now.

- Attach resources to a bundle. So for the demos and examples you need
  to add the data folder by hand to get them transferred to the
  device.
