# Python wrapper

This wrapper is not a proper Python-Allegro, but a simple 1:1 export
of all Allegro functions and constants using ctypes.

## Building

It's not currently wired into the cmake build process, so you should
build it like this:

- Run cmake to build the library. We really only need the generated
  headers for parsing in the next step but building the library is
  the easiest way to get them.

- Run the command below, specifying your cmake build dir (to find the
  headers mention above):

  docs/scripts/checkdocs.py -p protos -b <BUILD_DIR>

  You should now have a file called "protos" containing all the lines
  from Allegro's public headers which define functions, types and
  constants.

- Run the command below, specifying the protos file from before:

  python/generate_python_ctypes.py -p protos -o python/allegro.py
  
  This will generate the file allegro.py which has the code we want
  in it.

## Using it
  
Distribute the allegro.py along with your project. If you want you can
modify it to directly point to your allegro.dll so you can be sure
the file is found. By default, it will try several system specific
locations. E.g. in linux it will find .so files as long as they are
in LD_LIBRARY_PATH or in ldconfig paths.

## Limitations

Right now this is only a proof-of concept implementation, some
important things still have to be fixed:

### END_OF_MAIN broken under OSX
  
One problem on OSX is that al_init() should not be called from the
main thread. This will be solved once END_OF_MAIN has been removed,
for now you'll have to look into src/osx/main.m and see how you can
do the same from Python.

### Variable arguments not parsed properly yet

For example, if C code has:

al_draw_textf(font, x, y, flags, "%d, %d", x, y);

You have to do it like this in Python right now:

al_draw_textf(font, x, y, flags, "%s", "%d, %d" % (x, y))

### No reference counting or garbage collection

You probably should do something like this:

    class Bitmap:

		def __init__(self, filename):
			self.c_pointer = al_load_bitmap(filename)

		def __del__(self):
			al_destroy_bitmap(self.c_pointer)
	
		def draw(self, x, y, flags):
			al_draw_bitmap(self.c_pointer, x, y, flags);

In other words, a proper Python wrapper.

### No docstrings

Even though the documentation system provides the description of
each functions it's not copied into allegro.py right now. You will
need to use the C documentation until it is fixed.
