# Python wrapper

This wrapper is not a proper Python-Allegro, but a simple 1:1 export
of all Allegro functions and constants using ctypes.

## Building

Enable WANT_PYTHON_WRAPPER in cmake and you should end up with a
single file called allegro.py in the python folder of your build
directory. You also need to build the shared version so you end up
with either:

Windows: liballegro*.dll
OSX: liballegro*.dylib
Unix: liballegro*.so

For simplicity we will simply call those files "DLL files".

## Using it

Distribute the allegro.py as well as the required DLL files along with
your project. If you want you can modify it to directly point to the
DLL files so you can be sure they are found. By default, it will try
several system specific locations.

E.g. in linux it will find .so files as long as they are in
LD_LIBRARY_PATH or in ldconfig paths. For distribution build of your
game, this usually means that the .so files are found within your
distributions /usr/lib directory. For a standalone distribution, you
may provide a shell script which modifies LD_LIBRARY_PATH to point
to your game's library folder then runs the python executable.

To run the included Python example something like this will work under
Linux, starting within the Allegro source distribution folder:

    mkdir build
    cd build
    cmake -D WANT_PYTHON_WRAPPER=1 ..
    make
    export PYTHONPATH=python/
    export LD_LIBRARY_PATH=lib/
    python ../python/ex_draw_bitmap.py

We use PYTHONPATH to specify the location of the allegro.py module and
LD_LIBRARY_PATH to specify the location of the .so files.

For OSX, this should work:

    mkdir build
    cd build
    cmake -D WANT_PYTHON_WRAPPER=1 ..
    make
    export PYTHONPATH=python/
    export DYLD_LIBRARY_PATH=lib/
    python ../python/ex_draw_bitmap.py

For Windows:

Make sure to have WANT_PYTHON_WRAPPER enabled when building. It doesn't
matter whether you use mingw or MSVC, but Python must be installed on
your system.

Then run ex_draw_bitmap.py. The DLLs as well as allegro.py must be
found, easiest is to just have them all in the same directory. Or
you can also edit allegro.py and specify a directory from which to
load the DLLs - see the comments in there.

## Limitations

Right now this is only a proof-of concept implementation, some
important things still have to be fixed:

### Variable arguments not parsed properly yet

For example, if C code has:

al_draw_textf(font, x, y, flags, "%d, %d", x, y);

You have to do it like this in Python right now:

al_draw_textf(font, x, y, flags, "%s", "%d, %d" % (x, y))

### No reference counting or garbage collection

For example, if you call al_load_bitmap, a C pointer is returned. If
it goes out of scope you end up with a memory leak - very unpythonic.

Therefore you should do something like this:

    class Bitmap:

                def __init__(self, filename):
                        self.c_pointer = al_load_bitmap(filename)

                def __del__(self):
                        al_destroy_bitmap(self.c_pointer)

                def draw(self, x, y, flags):
                        al_draw_bitmap(self.c_pointer, x, y, flags);

In other words, make a proper Python wrapper.

### No docstrings

Even though the documentation system provides the description of
each functions it's not copied into allegro.py right now. You will
need to use the C documentation until this is fixed.

### Need more examples

Or a demo game. It should test that at least each addon can be used.
