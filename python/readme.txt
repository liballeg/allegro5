# Python wrapper

This wrapper is not a proper Python-Allegro, but a simple 1:1 export
of all Allegro functions and constants using ctypes.

## Building

Enable WANT_PYTHON_WRAPPER in cmake and you should end up with a
single file called allegro.py in the python folder of your build
directory.

## Using it
  
Distribute the allegro.py along with your project. If you want you can
modify it to directly point to your allegro.dll so you can be sure
the file is found. By default, it will try several system specific
locations. E.g. in linux it will find .so files as long as they are
in LD_LIBRARY_PATH or in ldconfig paths.

## Limitations

Right now this is only a proof-of concept implementation, some
important things still have to be fixed:

### Running main in a different thread under OSX
  
The problem in OSX is that al_init() should not be called from the
main thread. For now you'll have to look into src/osx/main.m and see how
you can do the same from Python. This should be fixed soon though.

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
