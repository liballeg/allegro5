#!/bin/sh
#
# Create `languages.dat' and `keyboard.dat' from `resource' directory.
#

test -z "$DAT" && DAT=dat

# Make sure that dat actually is available.
if ! ($DAT ; true) | fgrep -q 'Allegro'
then
    echo "$0: dat tool not available, aborted"
    exit 1
fi

echo "Creating keyboard.dat..."
(cd resource/keyboard; LD_PRELOAD=$MKDATA_PRELOAD $DAT -a -c1 ../../keyboard.dat *.cfg)

echo "Creating languages.dat..."
(cd resource/language; LD_PRELOAD=$MKDATA_PRELOAD $DAT -a -c1 ../../language.dat *.cfg)

chmod 664 keyboard.dat language.dat
