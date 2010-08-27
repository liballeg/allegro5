IPhone
======

This uses OpenGL ES for graphics. The touch screen is implemented as a
mouse, with multi-touch triggering different mouse buttons.

The accelerometer axes are reported as joystick axes.


Building
--------

In theory, byou uild like this:

mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-iphone.cmake -G Xcode ..
xcodebuild

However, many things cannot be set up by cmake, so you have to do them
by hand:

- Set per-project XCode options (like ipad/iphone target). Some things
  are right now set per-target from cmake so to change those you need to
  edit the CMakeLists.txt.
  
  Here are some things you want to change. To do so, right-click
  the ALLEGRO on top of the list to the left, then select "Get Info"
  and go to the "Build" tab. Then set them to whatever values you
  have there in your own IPhone projects.
  
  * Base SDK
  * Architectures
  * Code Signing Identity
  * Targeted Device Family

- Find the correct libraries (cmake always thinks it is building for
  OSX instead of IPhone). They are all hacked in with linker flags right
  now. The deps folder (see README_iphone) can however make things a lot
  easier. For example if you have freetype for iphone you could do this:
  
  mkdir deps
  cp -r my_iphone_freetype/{libs,include} deps
  
  And cmake should pick it up if they use the standard names.

- Attach resources to a bundle. So for the demos and examples you need
  to add the data folder by hand to get them transferred to the
  device. The easiest way to do that is to add whatever data folder
  is expected to the project as a folder reference (so it appears
  blue instead of yellow).
  
  Then right click -> Get Info -> Targets. And check the targets which
  need it.
