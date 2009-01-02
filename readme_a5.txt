% Allegro 4.9.x readme

Thanks for trying this development release of Allegro. This is currently work
in progress and only intended for developers. Here are some short notes.



Windows
=======

Allegro5 implements two graphics drivers for Windows: one using DirectX 9 and
onther one using OpenGL. You can force using OpenGL by calling
al_set_new_display_flags(ALLEGRO_OPENGL) prior to creating a display, while
DirectX is the default driver.
You can also select the driver using the `allegro.cfg` file.

Here are some files you may need regarding DirectX:

<http://trent.gamblin.ca/dx/>

Some of those are originally from:

<http://www.g-productions.net/list.php?c=files_devpak>

This is an experimental driver and more work still needs to be done
on compatibility. It works for many people, and doesn't work for others.
If you have problems, send and email to the alleg-developers list or
directly to Trent Gamblin @ trent2@gamblin.ca, and we may be able to
debug it further.



Linux / X Windows
=================

The only graphics driver implemented for Linux so far uses OpenGL on the X
Window System.  It has been tested with GLX 1.2 and above.  This driver isn't
Linux-specific so may work with other Unix-like systems but hasn't been
tested as such.



Mac OS X
========

We have an OpenGL graphics driver on Mac OS X as well.
The only sound driver we have on OS X at the moment uses OpenAL, so this is
required to get any sound at all on OS X. This should not normally be a
problem since OpenAL should be installed by default.



Compilation
===========

We provide two build systems, *CMake* and *scons*.

You will need CMake 2.6 or later.



CMake & Unix or MinGW or Mac OS X
---------------------------------

CMake works by first creating a makefile, which then can be used to compile.

To use it:

First create a directory for the build to take place in.
(This is not strictly necessary but keeps things cleaner.)

	$ mkdir Build
	$ cd Build

Run `cmake` with whatever options you want.  See below for examples.
If you're not sure about any option, just leave it at the default.

*Note:* MinGW users: if you have `sh.exe` in your PATH you should call CMake
with the options `-G "MSYS Makefiles"`.  Otherwise, use `-G "MinGW Makefiles"`.
If you do have `sh` but it isn't from MSYS (e.g. it's from Unixutils) then
you might need to modify your PATH so `sh` isn't in it and use "MinGW
Makefiles" instead.

The option `CMAKE_BUILD_TYPE` selects release, debug or profiling
configurations.  Valid values are: Release, Debug, RelWithDebInfo, MinSizeRel,
Profile.

The option `SHARED` controls whether libraries are built as shared libraries
or static libraries.  Shared libraries are built by default.

*Note:* For MinGW with gcc < 4, you cannot build a static library because
TLS (thread local storage, using __thread) support was not introduced
until version 4.

Examples: (you only need one)

	$ cmake ..

	$ cmake .. -G "MinGW Makefiles" -DSTATIC=off

	$ cmake .. -DCMAKE_BUILD_TYPE=Debug

Alternatively, you can use `ccmake` (Unix) or `cmake-gui` (Windows) to bring up
an interactive option selector. e.g. `ccmake ..` or `cmake-gui ..`.

*Note:* At least one user has had no luck with `cmake-gui` on Windows. If you
run into problems, please try again with `cmake`.

Now run `make` (or `mingw32-make`) and, optionally, `make install`.
(For MinGW users, you may have to set `MINGDIR` to point to your MinGW
directory before you do `mingw32-make install`):

	$ make
	$ make install



CMake & MSVC
------------

First open up a console and make sure that cmake.exe is in your %PATH%. You
ensure than by typing "SET PATH=C:\cmake\bin\;%PATH%" or similar. Typing
"cmake" should display the help message. It is also a good idea to have MSVC
compiler cl.exe in your path, to make it easier for CMake to find it, especially
if you have mutiple MSVC versions installed. You can do that by running
vcvars32.bat script found in the directory where MSVC is installed.

CMake works by first creating a project solution, which can then be opened
with MSVC IDE and built.

You may have to set your path to your DirectX SDK's Include and Lib directories,
	set INCLUDE=c:\program files\microsoft directx x.xx\include;%INCLUDE%
	set LIB=c:\program files\microsoft directx x.xx\lib\x86;%LIB%

You can use the same options from the CMake section above.

Examples:

	$ cmake -G "Visual Studio 8 2005" -DCMAKE_BUILD_TYPE=Debug

	$ cmake -G "Visual Studio 9 2008"

Now that the project solution has been generated, open it with the MSVC IDE
and start the building process.

*Note:*
The demo is currently excluded from the build if MSVC8 is detected, due to
some problems with IntelliSense.
Please give it a try and report problems.



Hints on setting up Visual C++ 2005 Express Edition
---------------------------------------------------

After installing Visual C++, you need to install the Platform SDK (or Windows
SDK), otherwise CMake will fail at a linking step.  You can do a web install to
avoid downloading a massive file.  For me, installation seemed to take an
inordinately long (half an hour or more), but eventually it completed.
Don't be too quick to hit Cancel.

You also need to install the DirectX SDK.  This is a big download, which I
don't know how to avoid.

Next, you'll need to tell VC++ about the Platform SDK.  Start the IDE.  Under
"Tools > Options > Projects and Solutions > VC++ Directories", add the Platform
SDK executable (bin), include and lib directories to the respective lists.
The DirectX SDK seems to add itself when it's installed.

Now you can open a command prompt with the correct environment by going to
"Tools > Visual Studio 2005 command prompt".  Change to the Allegro directory
and run CMake as before.

For debugging, use the DirectX control panel applet to switch to the debugging
runtime.  It's really useful.



Scons
-----

Scons uses more sophisticated dependency tracking than make, for example it
does not use timestamps, and in general will always know what to rebuild even
if files are added/removed/renamed, options are changed, or even external
dependencies are changed.

You should use scons 1.0.0 or above.

See the Allegro wiki for more info on the scons build system:
<http://wiki.allegro.cc/Scons>

To compile Allegro, simply type this inside the allegro directory:

	$ scons

To install, run (as root):

	$ scons install

To build different types of libraries, use different targets

Example:

	$ scons all # normal shared, non-debug
	$ scons all-static
	$ scons all-debug
	$ scons all-static-debug
	$ scons all-profile

To build all the versions at once
	
	$ scons everything

To install as a non-root user, you can do:

	$ scons install install=/home/myuser/mydirectory

To install all versions at once

	$ scons install-everything



Optional dependencies
=====================

Many of the addons make use of additional libraries.  They are not required to
build Allegro, but some functionality may be disabled if they are not present.
In particular the demo game currently *requires* Ogg Vorbis support.

The libraries are:

- libpng/zlib for PNG support
- FreeType, for TrueType font support
- Ogg Vorbis
- FLAC
- libsndfile, for loading .wav and other sample formats
- OpenAL (only really needed on Mac OS X due to missing native driver)
- ImageMagick (not really used)

On Windows, OpenGL is also optional.



Running the examples
====================

Remember that some examples look for data files in the current directory, so if
you used an external build directory you will need to change into the examples
directory and specify the path to the example in the build directory, e.g.

	$ cd examples
	$ ../Build/examples/ex_bitmap

Or symlink the data directory into the build directory:

	$ cd Build/examples
	$ ln -s ../../examples/data .
	$ ./ex_bitmap



API documentation
=================

The documentation is all linked to from the wiki:
<http://wiki.allegro.cc/NewAPI>

If you have NaturalDocs installed you can build the documentation by
running `make` in the `docs/naturaldocs` directory. You need at least
version 1.4 of NaturalDocs.

There is also the `docs/src/refman` directory, which doesn't contain
much yet.

