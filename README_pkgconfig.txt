Using pkg-config with Allegro
=============================

On Unix-like operating systems, we use the pkg-config tool to simplify the
process of linking with Allegro.

To print the list of Allegro libraries installed, you can run:

    pkg-config --list-all | grep allegro

You may need to set the PKG_CONFIG_PATH environment variable appropriately if
you installed to a non-standard location.

To print the command line options required to link with the core Allegro
library and the image addon, you would run:

    pkg-config --libs allegro-5 allegro_image-5

which outputs something like:

    -L/usr/lib -lallegro_image -lallegro

This can be combined with shell command substitution:

    gcc mygame.c -o mygame $(pkg-config --libs allegro-5 allegro_image-5)

If Allegro is installed to a non-standard location, the compiler will need
command line options to find the header files.  The pkg-config `--cflags`
option provides that information.  You can combine it with `--libs` as well:

    pkg-config --cflags --libs allegro-5 allegro_image-5

Most build systems will allow you to call pkg-config in a similar way to the
shell.  For example, a very basic Makefile might look like this:

    ALLEGRO_LIBRARIES := allegro-5 allegro_image-5
    ALLEGRO_FLAGS := $(shell pkg-config --cflags --libs $(ALLEGRO_LIBRARIES))

    mygame: mygame.c
	    $(CC) -o $@ $^ $(ALLEGRO_FLAGS)


Historical note
---------------

Prior to Allegro 5.0.9 the .pc files were named allegro-5.0.pc and so on.
To ease transitioning to future versions of Allegro the minor version
number was dropped, leaving allegro-5.pc and so on.  The old names are
deprecated but will remain available in 5.0.x releases.
Configuration scripts should look for Allegro using the new pkg-config
names first, then the old names, for greater compatibility.

