#!/bin/sh

if [ "x$1" = "x" ]; then
	echo "Usage: utod filename"
	exit -1
fi

if (test -x "$1"); then
	AGL_EXE="1"
fi

# Delete all carriage returns, then insert one before each line feed;
# touch to update the timestamp,
# then move on top of the old file
tr -d \\\r < "$1" | sed -e `echo -e 's/\\(.*\\)/\\\\1\\\\\\015/'` > .utod.tmp &&
touch -r "$1" .utod.tmp &&
mv -f .utod.tmp "$1"

if (test -n "$AGL_EXE"); then
	chmod +x $1
fi
