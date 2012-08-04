#!/bin/sh -ex
# Recreate the automatically generated source files in the tree.

python misc/make_scanline_drawers.py | \
   indent -kr -i3 -l0 > src/scanline_drawers.inc

python misc/make_converters.py

misc/gl_mkalias.sh

# vim: set sts=3 sw=3 et:
