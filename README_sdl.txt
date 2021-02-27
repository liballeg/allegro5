SDL
===

This is an experimental port which uses SDL2 to create the Allegro
window and OpenGL context.


Dependencies
------------

SDL2 is required.


Building
--------

Pass -D ALLEGRO_SDL=on to cmake to enable building for SDL2.


Limitations
-----------

SDL2 requires calling SDL_PumpEvents() from the main thread regularly to
generate events. Currently Allegro5 makes those calls from the timer
subsystem - so as long as you have any timers running things will work
fine. If your Allegro 5 program uses no timers it may get stuck because
no SDL2 events can be received.


Emscripten
----------

One reason the SDL2 port is useful is that it allows running Allegro on
platforms not supported directly by Allegro, such as Emscripten. This is
even more experimental but here are some example steps that will compile
most of the examples for running in a web browser:

1. Make sure to set up the emscripten environment by running the

emsdk_env.sh

or equivalent as provided by emscripten.

2. The "--preload-file data" option below includes a folder named "data"
with each binary, so we make sure each example and demo has such a
folder:

( cd examples; ln -s {path-to-allegro-source-folder}/examples/data )
mkdir demos/speed/data
touch demos/speed/data/nothing.txt

3. Create a build folder and configure CMake, using emcmake

mkdir build_emscripten
cd build_emscripten

USE_FLAGS=(
    -s USE_FREETYPE=1
    -s USE_VORBIS=1
    -s USE_OGG=1
    -s USE_LIBJPEG=1
    -s USE_SDL=2
    -s USE_LIBPNG=1
    -s FULL_ES2=1
    -s ASYNCIFY
    -s TOTAL_MEMORY=2147418112
    -O3
    )

emcmake cmake {path-to-allegro-source-folder}
    -D CMAKE_BUILD_TYPE=Release
    -D ALLEGRO_SDL=ON
    -D SHARED=OFF
    -D WANT_MONOLITH=ON
    -D WANT_ALLOW_SSE=OFF
    -D WANT_DOCS=OFF
    -D WANT_TESTS=OFF
    -D WANT_OPENAL=OFF
    -D ALLEGRO_WAIT_EVENT_SLEEP=ON
     # not sure why these are not found automatically with emcmake, so
     # we use a  hack - we manually specify the cache path, so cmake
     # will fail on first run in the compile test but populate the cache
     # (from the -s options) and on second run it all works
    -D SDL2_INCLUDE_DIR=$EM_CACHE/include
    -D PNG_PNG_INCLUDE_DIR=$EM_CACHE/include
    -D PNG_LIBRARY=$EM_CACHE/wasm/libpng.a
    -D JPEG_INCLUDE_DIR=$EM_CACHE/include
    -D JPEG_LIBRARY=$EM_CACHE/wasm/libjpeg.a
    -D FREETYPE_INCLUDE_DIRS=$EM_CACHE/include
    -D FREETYPE_LIBRARY=$EM_CACHE/wasm/libfreetype.a
    -D VORBIS_INCLUDE_DIR=$EM_CACHE/include/vorbis
    -D VORBIS_LIBRARY=$EM_CACHE/wasm/libvorbis.a
    -D VORBISFILE_LIBRARY=$EM_CACHE/wasm/libvorbis.a
    -D OGG_INCLUDE_DIR=$EM_CACHE/include
    -D OGG_LIBRARY=$EM_CACHE/wasm/libogg.a
    -D CMAKE_C_FLAGS="${USE_FLAGS[@]}"
    -D CMAKE_CXX_FLAGS="${USE_FLAGS[@]}"
    -D CMAKE_EXE_LINKER_FLAGS="${USE_FLAGS[@]} --preload-file data"
    -D CMAKE_EXECUTABLE_SUFFIX=".html"

To compile your own game adjust as necessary. You can use the
lib/liballegro_monolith-static.a library.
