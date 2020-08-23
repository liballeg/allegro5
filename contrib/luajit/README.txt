Luajit
======

The contents of this directory provides support for using Allegro5 from [Luajit][lj].


Usage
-----

To create a Luajit FFI desciption we use the prototypes scraped using the checkdocs.py script.

    # output a file protos.txt
    ./python/checkdocs.py -p protos.txt -b build/

    # use the prototypes to create Luajit datatypes for Allegro5
    ./contrib/luajit/generate_luajit_ffi.py -p protos.txt -o al5_ffi.lua

See the Luajit docs on how to use this file.


[lj]: http://luajit.org
