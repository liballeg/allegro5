#!/bin/sh -ex
# Recreate the automatically generated source files in the tree.

python2 misc/make_scanline_drawers.py | \
   indent -kr -i3 -l0 > src/scanline_drawers.inc

python2 misc/make_converters.py

misc/gl_mkalias.sh

python2 misc/make_mixer_helpers.py | \
   indent -kr -i3 -l0 > addons/audio/kcm_mixer_helpers.inc

# vim: set sts=3 sw=3 et:
