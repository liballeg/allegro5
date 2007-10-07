#!/bin/sh

if [ "x$1" = "x" ]; then
	echo "Usage: dtou filename"
	exit -1
fi

if (test -x "$1"); then
	AGL_EXE="1"
fi

# Delete all carriage returns
# touch to update the timestamp,
# then move on top of the old file
tr -d \\\r < "$1" > .dtou.tmp &&
touch -r "$1" .dtou.tmp &&
mv -f .dtou.tmp "$1"

if (test -n "$AGL_EXE"); then
	chmod +x $1
fi
