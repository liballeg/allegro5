#!/bin/bash
# Recreate the automatically generated source files in the tree.

set -e
set -x

misc/make_scanline_drawers.py | \
   indent -kr -i3 -l0 > src/scanline_drawers.inc

misc/make_converters.py

misc/gl_mkalias.sh

misc/make_mixer_helpers.py | \
   indent -kr -i3 -l0 > addons/audio/kcm_mixer_helpers.inc

# vim: set sts=3 sw=3 et:
