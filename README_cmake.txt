CMake options
-------------

Our build system supports many options.  Here are some of them:

The option `CMAKE_BUILD_TYPE` selects release, debug or profiling
configurations.  Valid values are: Release, Debug, RelWithDebInfo, MinSizeRel,
Profile.

The option `SHARED` controls whether libraries are built as shared libraries
or static libraries.  Shared libraries are built by default.

*Note:* For MinGW with gcc < 4, you cannot build a static library because
TLS (thread local storage, using __thread) support was not introduced
until version 4.

There are many options named WANT_*.  Unselecting these will prevent the
associated addon or feature from being built.

HTML and man page documentation will be built by default, but Info and PDF
(through pdfLaTeX) can also be selected from the CMake options.



deps subdirectory
-----------------

As a convenience, you may create a subdirectory called "deps" in the main
Allegro directory, or in the build directory.  Inside you can place header and
library files for dependencies that Allegro will search for.  Allegro will
search in these locations:

        deps/include
        deps/lib
        deps/<anything>/include
        deps/<anything>/lib

