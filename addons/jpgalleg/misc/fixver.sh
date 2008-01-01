#!/bin/sh
#
#  Shell script to adjust the version numbers and dates wherever they appear
#  on the JPGalleg sources and text files.

if [ $# -ne 2 ]; then
   echo "Usage: fixver major minor" 1>&2
   exit 1
fi

ver_str="$1.$2"
year_str="$(date +%Y)"

echo "s/Version [0-9]\.[0-9], by Angelo Mottola, .*/Version $ver_str, by Angelo Mottola, 2000-$year_str/" > fixver.sed
echo "s/JPGalleg [0-9]\.[0-9], by Angelo Mottola, .*/JPGalleg $ver_str, by Angelo Mottola, 2000-$year_str/" >> fixver.sed
echo "s/\"JPGalleg [0-9]\.[0-9], by Angelo Mottola, .*/\"JPGalleg $ver_str, by Angelo Mottola, 2000-$year_str\"/" >> fixver.sed
echo "s/JPGALLEG_VERSION .*/JPGALLEG_VERSION 0x0$1\0$2/" >> fixver.sed

files_list=`find . -type f "(" -name "*.c" -o -name "*.s" -o -name "*.h" -o -name "makefile*" ")"`

for file in $files_list; do
   echo "Patching $file..."
   cp $file fixver.tmp
   sed -f fixver.sed fixver.tmp > $file
done

rm fixver.sed fixver.tmp

echo "Done!"
