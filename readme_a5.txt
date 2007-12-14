Thanks for trying this development release of Allegro. This is currently work
in progress and only intended for developers. Here are some short notes:



== Windows ==

The only graphics driver implemented so far requires DirectX 9. Here are some
files you may need:

http://trent.gamblin.ca/dx/

Some of those are originally from:

http://www.g-productions.net/list.php?c=files_devpak

This is an experimental driver and more work still needs to be done
on compatibility. It works for many people, and doesn't work for others.
If you have problems, send and email to the alleg-developers list or
directly to Trent Gamblin @ trent2@gamblin.ca, and we may be able to
debug it further.



== Linux ==

The only graphics driver implemented so far requires a fairly recent GLX, it
was only tested with GLX 1.4 so far.



== OSX ==

Sorry, nobody did yet write any drivers for OSX.



== Compilation ==

We provide two build systems, cmake and scons. cmake works on Windows
and scons works on Linux.



=== Cmake ===

Cmake works by first creating a makefile, which then can be used to compile.

To use it:

First create a directory for the build to take place in. E.g.:

mkdir tmp
cd tmp

Note: For MinGW with gcc < 4, you cannot build a static library because
TLS (thread local storage, using __thread) support was not introduced
until version 4.

Run cmake with whatever options you want. Some common options are,
with defaults in braces:
	- GRADE_STANDARD -- Build release library (on)
	- GRADE_DEBUG -- Build debug library (off)
	- GRADE_PROFILE -- Build profiling library (off)
	- SHARED -- Build shared libraries (on)
	- STATIC -- Build static libraries (on)
	- WANT_FONT -- Build the font addon, needed by demo (on)
	- WANT_D3D -- Build the D3D driver (on)

Example:

cmake -G "MinGW Makefiles" \
	-DSTATIC=off -DGRADE_STANDARD=off -DGRADE_DEBUG=on ..

Now run make (or mingw32-make) and make install
(Note: You should have MINGDIR pointing to your MinGW directory
before you do mingw32-make install):

mingw32-make
mingw32-make install

Compiling with MSVC is untested.



=== Scons ===

Scons uses more sophisticated dependency tracking than make, for example it
does not use timestamps, and in general will always know what to rebuild even
if files are added/removed/renamed, options are changed, or even external
dependencies are changed.

To compile Allegro, simply type this inside the allegro directory:

scons

To install, run (as root):

scons install

TODO: Options, how to run as non-root



== Running the examples ==

Currently, only a few examples work using the new API.
Some of the older 4.2.x examples work with the new drivers,
but are not accelerated.

- The demo has been replaced with an A5 demo (C++ right now)
- exnewapi - A messy example showing off a lot of A5 features
- exnew_bitmap - Simply draws a bitmap on screen
- exnew_fs_resize - Demonstrates fullscreen display resizing
- enxew_lockbitmap - Shows how to lock a bitmap to write directly to it
- exnew_lockscreen - Like exnew_lockbitmap, but operates directly on the screen
- exnew_mouse - Uses mouse polling to show a cursor
- exnew_mouse_events - Uses the new event system to show a mouse cursor
- exnew_resize - Demonstrates windows resized with code

