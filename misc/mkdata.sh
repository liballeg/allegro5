#!/bin/sh
#
# Create `languages.dat' and `keyboard.dat' from `resource' directory.
#

test -z "$DAT" && DAT=dat

echo "Creating keyboard.dat..."
(cd resource/keyboard; LD_PRELOAD=$MKDATA_PRELOAD $DAT -a -c1 ../../keyboard.dat *.cfg)

echo "Creating languages.dat..."
(cd resource/language; LD_PRELOAD=$MKDATA_PRELOAD $DAT -a -c1 ../../language.dat *.cfg)

chmod 664 keyboard.dat language.dat
