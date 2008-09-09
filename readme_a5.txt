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

XXX fill this in



Compilation
===========

We provide two build systems, *CMake* and *scons*.

You will need CMake 2.6 or later.



CMake & Unix or MinGW
---------------------

CMake works by first creating a makefile, which then can be used to compile.

To use it:

First create a directory for the build to take place in.
(This is not strictly necessary but keeps things cleaner.)

	$ mkdir Build
	$ cd Build

*Note:* For MinGW with gcc < 4, you cannot build a static library because
TLS (thread local storage, using __thread) support was not introduced
until version 4.

Run `cmake` with whatever options you want. Some common options are,
with defaults in brackets:

- GRADE_STANDARD -- Build release library (on)
- GRADE_DEBUG -- Build debug library (off)
- GRADE_PROFILE -- Build profiling library (off)
- SHARED -- Build shared libraries (on)
- STATIC -- Build static libraries (on)

Examples:

	$ cmake .. -G "MinGW Makefiles" -DSTATIC=off

	$ cmake .. -G "Unix Makefiles"

Alternatively, you can use `ccmake` to bring up an interactive option
selector. e.g. `ccmake ..`

Now run `make` (or `mingw32-make`) and, optionally, `make install`.
(For MinGW users, you should have `MINGDIR` pointing to your MinGW directory
before you do `mingw32-make install`):

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

You can use the same options from the CMake section above.

Examples:

	$ cmake -G "Visual Studio 8 2005" -DGRADE_DEBUG=on -DGRADE_STANDARD=off

	$ cmake -G "Visual Studio 9 2008"

Now that the project solution has been generated, open it with the MSVC IDE
and start the building process.

*Note:*
The demo is currently excluded from the build if MSVC8 is detected, due to
some problems with IntelliSense.
Please give it a try and report problems.



CMake & Mac OS X
----------------

This should be the same as on a traditional Unix, except that currently the
shared library won't link properly so you'll have to disable it.

Example:

	$ mkdir Build
	$ cmake .. -DSHARED=off
	$ make



Scons
-----

Scons uses more sophisticated dependency tracking than make, for example it
does not use timestamps, and in general will always know what to rebuild even
if files are added/removed/renamed, options are changed, or even external
dependencies are changed.

See the Allegro wiki for more info on the scons build system:
<http://wiki.allegro.cc/Scons>

To compile Allegro, simply type this inside the allegro directory:

	$ scons

To install, run (as root):

	$ scons install

There are also some options you can use with the scons command:

- static (default: 1)
- debug

Example:

	$ scons static=1 debug=1
	$ scons static=0 debug=1

etc..

To install as a non-root user, you can do:

	$ scons install install=/home/myuser/mydirectory



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
running `make` in the `docs/naturaldocs` directory.

There is also the `docs/src/refman` directory, which doesn't contain
much yet.

