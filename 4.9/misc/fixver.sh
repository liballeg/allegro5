#! /bin/sh -e
#
#  Shell script to adjust the version numbers and dates in files.
#
#  Note: if you pass "datestamp" as the only argument, then the version
#  digits will remain unchanged and the comment will be set to the date.
#  This is in particular useful for making SVN snapshots.


if [ $# -lt 3 -o $# -gt 4 ]; then
   if [ $# -eq 1 -a $1 == "datestamp" ]; then
      ver=`grep "version=[0-9]" misc/allegro5-config.in`
      major_num=`echo $ver | sed -e "s/version=\([0-9]\).*/\1/"`
      sub_num=`echo $ver | sed -e "s/version=[0-9]\.\([0-9]\).*/\1/"`
      wip_num=`echo $ver | sed -e "s/version=[0-9]\.[0-9]\.\([0-9]\+\).*/\1/"`
      $0 $major_num $sub_num $wip_num `date '+%Y%m%d'`
      exit 0
   else
      echo "Usage: fixver major_num sub_num wip_num [comment]" 1>&2
      echo "   or: fixver datestamp" 1>&2
      echo "Example: fixver 3 9 1 WIP" 1>&2
      exit 1
   fi
fi

# get the version and date strings in a nice format
if [ $# -eq 3 ]; then
   verstr="$1.$2.$3"
else
   verstr="$1.$2.$3 ($4)"
fi

year=$(date +%Y)
month=$(date +%m)
day=$(date +%d)
datestr="$(date +%b) $day, $year"

# patch allegro/base.h
echo "s/\#define ALLEGRO_VERSION .*/\#define ALLEGRO_VERSION          $1/" > fixver.sed
echo "s/\#define ALLEGRO_SUB_VERSION .*/\#define ALLEGRO_SUB_VERSION      $2/" >> fixver.sed
echo "s/\#define ALLEGRO_WIP_VERSION .*/\#define ALLEGRO_WIP_VERSION      $3/" >> fixver.sed
echo "s/\#define ALLEGRO_VERSION_STR .*/\#define ALLEGRO_VERSION_STR      \"$verstr\"/" >> fixver.sed
echo "s/\#define ALLEGRO_DATE_STR .*/\#define ALLEGRO_DATE_STR         \"$year\"/" >> fixver.sed
echo "s/\#define ALLEGRO_DATE .*/\#define ALLEGRO_DATE             $year$month$day    \/\* yyyymmdd \*\//" >> fixver.sed

echo "Patching include/allegro5/base.h..."
cp include/allegro5/base.h fixver.tmp
sed -f fixver.sed fixver.tmp > include/allegro5/base.h

# patch the OSX package readme
echo "s/\\_\/__\/     Version .*/\\_\/__\/     Version $verstr/" > fixver.sed
echo "s/By Shawn Hargreaves, .*\./By Shawn Hargreaves, $datestr\./" >> fixver.sed

echo "Patching misc/pkgreadme._tx..."
cp misc/pkgreadme._tx fixver.tmp
sed -f fixver.sed fixver.tmp > misc/pkgreadme._tx

# patch allegro5-config.in, allegro-config.qnx
echo "s/version=[0-9].*/version=$1.$2.$3/" >> fixver.sed

echo "Patching misc/allegro5-config.in..."
cp misc/allegro5-config.in fixver.tmp
sed -f fixver.sed fixver.tmp > misc/allegro5-config.in

echo "Patching misc/allegro-config-qnx.sh..."
cp misc/allegro-config-qnx.sh fixver.tmp
sed -f fixver.sed fixver.tmp > misc/allegro-config-qnx.sh

# patch the spec file
echo "Patching misc/allegro.spec..."
cp misc/allegro.spec fixver.tmp
sed -e "s/^Version: .*/Version: $1.$2.$3/" fixver.tmp > misc/allegro.spec

# patch CMakeLists.txt
echo "Patching CMakeLists.txt..."
cp CMakeLists.txt fixver.tmp
sed -e "s/set(ALLEGRO_VERSION [^)]*)/set(ALLEGRO_VERSION $1.$2.$3)/" fixver.tmp > CMakeLists.txt

# clean up after ourselves
rm fixver.sed fixver.tmp

echo "Done!"
