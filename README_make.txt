Building Allegro with make
==========================

This document discusses building Allegro using CMake and GNU make from a
terminal window.  This document applies to Unix-like operating systems such as
Linux, and also Mac OS X and MinGW.

1. Unpack Allegro.

2. Create a build directory under the Allegro directory and go there.

        cd /path/to/allegro
        mkdir Build
        cd Build

3. Run `cmake` with whatever options you want.  See README_cmake.txt
for details about options you can set.

        cmake ..

Here ".." is the path to the Allegro directory.

Alternatively, you can use `ccmake` (Unix) or `cmake-gui` (Windows) to bring up
an interactive option selector. e.g. `ccmake ..` or `cmake-gui ..`.

You may need to tell CMake which "generator" to use; cmake -h will tell you
which generators are available.  We recommend using the Makefile generator
(default everywhere except Windows).  On MinGW you will have a choice between
"MinGW Makefiles" or "MSYS Makefiles".  If `sh.exe` is on your PATH then you
must use "MSYS Makefiles", otherwise use "MinGW Makefiles".

More examples:

        cmake .. -G "MinGW Makefiles"

        cmake .. -G "MSYS Makefiles"

4. Now, if that step was successful you can run `make` to build Allegro.
On MinGW your make might actually be called `mingw32-make`.

        make

Since multicore processors are common now, you might wish to speed that up by
passing a "-j<n>" option, where <n> is the number of parallel jobs to spawn.


5. You may optionally install Allegro into your system path with the install
target.

        make install

MinGW users might need to set the MINGDIR environment variable first.

The DESTDIR variable is supported for staged installs.

        make install DESTDIR=/tmp/allegro-package

