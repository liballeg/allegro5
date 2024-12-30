Welcome to Allegro!
===================

Allegro is a cross-platform library mainly aimed at video game and
multimedia programming. It handles common, low-level tasks such as
creating windows, accepting user input, loading data, drawing images,
playing sounds, etc. and generally abstracting away the underlying
platform. However, Allegro is not a game engine: you are free to design
and structure your program as you like.

Allegro 5 has the following additional features:

- Supported on Windows, Linux, Mac OSX, iPhone and Android
- User-friendly, intuitive C API usable from C++ and many other programming languages
- Hardware accelerated bitmap and graphical primitive drawing support (via OpenGL or Direct3D)
- Audio recording support
- Font loading and drawing
- Video playback
- Abstractions over shaders and low-level polygon drawing
- And more!

This readme contains general information which applies to all platforms
that Allegro builds on.

README_cmake.txt discusses some build options for cmake.

README_msvc.txt discusses compilation on Windows with Microsoft Visual C/C++.

README_make.txt discusses compilation with GNU make.  This applies to Unix-like
operating systems such as Linux, MacOS X and MinGW on Windows.

README_macosx.txt has a few additional notes for MacOS X.

README_iphone.txt discusses iPhone operating systems.



Requirements
============

We assume you have C and C++ compilers installed and functioning.
We support gcc, clang and MSVC.

Allegro also requires CMake 3.0 or later to build.
You may download it from <http://www.cmake.org/>



Library dependencies
====================

Allegro is divided into a core library and a number of addon libraries.
The core library depends on certain libraries to function.  If you don't have
those, nothing will work.  These are required for the core library:

- DirectX SDK (Windows only)

  You can get this for MSVC from the Microsoft web site (large download).

  Alternatively, smaller downloads for MSVC and MinGW are available
  here: <http://liballeg.org/download.html#miscellaneous-files>. Some
  MinGW distributions come with sufficient DirectX SDK to support
  compiling Allegro.

- X11 development libraries (Linux/Unix only)
  The libraries will be part of your Linux distribution, but you may have to
  install them explicitly.

- OpenGL development libraries (optional only on Windows)

The addons, too, may require additional libraries.  Since the addons are
strictly optional, they are not required to build Allegro, but a lot of
functionality may be disabled if they are not present.

Windows users may find some precompiled binaries for the additional libraries
from <http://gnuwin32.sourceforge.net/>.  You need to get the `bin` and `lib`
packages.  The `bin` packages contain DLLs, and the `lib` packages contain the
headers and import libraries.

Mac users may find some dependencies in Homebrew, Fink or MacPorts.
<http://brew.sh/>, <http://www.finkproject.org/> and
<http://www.macports.org/>

Linux users likely have all the dependencies already, except PhysicsFS
and DUMB. If your distribution uses separate development packages, they
will need to be installed.  The packages are probably named *-dev or *-devel.

These are the dependencies required for the addons:

- libpng and zlib, for PNG image support (Unix and older MinGW only)
  Home page: <http://www.libpng.org/pub/png/>
  Windows binaries: <http://gnuwin32.sourceforge.net/packages/libpng.htm>

  On Windows/Mac OS X/iPhone/Android, PNG image support is available by
  using the native facilities on the respective operating systems, so
  libpng is not required.

- libjpeg, for JPEG image support (Unix and older MinGW only)
  Home page: <http://www.ijg.org/>
  Windows binaries: <http://gnuwin32.sourceforge.net/packages/jpeg.htm>

  On Windows/Mac OS X/iPhone/Android, JPEG image support is available
  by using the native facilities on the respective operating systems,
  so libjpeg is not required.

- libwebp, for WebP image support
  Home page: <https://developers.google.com/speed/webp/>

  On Android, WebP image support is available by using the native
  facilities of the operating system, so libwebp is not required.

- FreeType, for TrueType font support.
  Home page: <http://freetype.sourceforge.net/>
  Windows binaries: <http://gnuwin32.sourceforge.net/packages/freetype.htm>

- Ogg Vorbis, a free lossy audio format. (libogg, libvorbis, libvorbisfile)
  Home page: <http://www.vorbis.com/>

- Opus, a free lossy audio codec. (libogg, libopus, libopusfile)
  Home page: <http://www.opus-codec.org/>

- FLAC, a free lossless audio codec. (libFLAC, libogg)
  Home page: <http://flac.sourceforge.net/>

- DUMB, an IT, XM, S3M and MOD player library. (libdumb)
  Home page: <http://dumb.sourceforge.net/>

- OpenAL, a 3D audio API.
  The audio addon can use OpenAL, although the 3D capabilities aren't used.
  <http://kcat.strangesoft.net/openal.html>

  On Mac OS X, OpenAL is *required* but should come with the OS anyway.

  On Linux and Windows, OpenAL will only be used if you request it, hence there
  is no reason to install it specifically.

- PhysicsFS, provides access to archives, e.g. .zip files.
  Home page: <http://icculus.org/physfs/>

On Windows it may be a pain to place all these libraries such that they can be
found.  Please see the README_cmake.txt section on the "deps subdirectory"
when the time comes.



API documentation
=================

To build the documentation you will need Pandoc.
Pandoc's home page is <http://johnmacfarlane.net/pandoc/>

Installing Pandoc from source can be challenging, but you can build Allegro
without building the documentation.

Online documentation is available on the Allegro web site:
<http://docs.liballeg.org/>



Building with CMake
===================

Building with CMake is a two step process.  During the _configuration_ step,
cmake will detect your compiler setup and find the libraries which are
installed on your system.  At the same time, you may select options to
customise your build.  If you are unsure of what you are doing, leave all the
options at the defaults.

You must configure Allegro with a separate build directory. For example,

    mkdir build
    cd build
    cmake ..

If you configure Allegro to build in the source directory (i.e. `cmake .`)
you will get an error message. Delete `CMakeCache.txt` and the `CMakeFiles`
directory and re-configure as described above.

Once the configuration step is successful, you will invoke another tool to
build Allegro.  The tool depends on your compiler, but is usually either
`make`, or your IDE.

To avoid problems, unpack Allegro into a directory *without spaces or other
"weird" characters in the path*.  This is a known problem.

Now read README_msvc.txt, README_make.txt or README_macosx.txt.

