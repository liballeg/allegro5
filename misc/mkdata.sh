#!/bin/sh
#
# Create `languages.dat' and `keyboard.dat' from `resource' directory.
#

DAT=dat

echo "Creating keyboard.dat..."
(cd resource/keyboard; $DAT -a -c1 ../../keyboard.dat *.cfg)

echo "Creating languages.dat..."
(cd resource/language; $DAT -a -c1 ../../language.dat *.cfg)
