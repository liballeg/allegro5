#! /bin/sh
#
#  Shell script to adjust the version numbers and dates in allegro.h,
#  readme.txt, allegro._tx, and makefile.all.


if [ $# -lt 3 -o $# -gt 4 ]; then
   echo "Usage: fixver major_num sub_num wip_num [comment]" 1>&2
   echo "Example: fixver 3 9 1 WIP" 1>&2
   exit 1
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

# patch allegro.h
echo "s/\#define ALLEGRO_VERSION .*/\#define ALLEGRO_VERSION          $1/" > fixver.sed
echo "s/\#define ALLEGRO_SUB_VERSION .*/\#define ALLEGRO_SUB_VERSION      $2/" >> fixver.sed
echo "s/\#define ALLEGRO_WIP_VERSION .*/\#define ALLEGRO_WIP_VERSION      $3/" >> fixver.sed
echo "s/\#define ALLEGRO_VERSION_STR .*/\#define ALLEGRO_VERSION_STR      \"$verstr\"/" >> fixver.sed
echo "s/\#define ALLEGRO_DATE_STR .*/\#define ALLEGRO_DATE_STR         \"$year\"/" >> fixver.sed
echo "s/\#define ALLEGRO_DATE .*/\#define ALLEGRO_DATE             $year$month$day    \/\* yyyymmdd \*\//" >> fixver.sed

echo "Patching include/allegro.h..."
cp include/allegro.h fixver.tmp
sed -f fixver.sed fixver.tmp > include/allegro.h

#echo patch dllver.rc
echo "s/VALUE \"InternalName\", .*/VALUE \"InternalName\", \"ALL$1$2$3\\\\000\"/" > fixver.sed
echo "s/VALUE \"OriginalFilename\", .*/VALUE \"OriginalFilename\", \"ALL$1$2$3\\\\.DLL\\\\000\"/" >> fixver.sed

echo "Patching src/win/dllver.rc..."
cp src/win/dllver.rc fixver.tmp
sed -f fixver.sed fixver.tmp > src/win/dllver.rc

# patch readme.txt
echo "s/\\_\/__\/     Version .*/\\_\/__\/     Version $verstr/" > fixver.sed
echo "s/By Shawn Hargreaves, .*\./By Shawn Hargreaves, $datestr\./" >> fixver.sed

echo "Patching readme.txt..."
cp readme.txt fixver.tmp
sed -f fixver.sed fixver.tmp > readme.txt

# patch allegro._tx
echo "s/@manh=\"version [^\"]*\"/@manh=\"version $verstr\"/" >> fixver.sed

echo "Patching docs/allegro._tx..."
cp docs/allegro._tx fixver.tmp
sed -f fixver.sed fixver.tmp > docs/allegro._tx

# patch makefile.ver
prever=`sed -n -e "s/shared_major = \(.*\)/\1/p" makefile.ver`

echo "s/LIBRARY_VERSION = .*/LIBRARY_VERSION = $1$2$3/" > fixver.sed
echo "s/shared_version = .*/shared_version = $1.$2.$3/" >> fixver.sed
echo "s/shared_major = .*/shared_major = `expr $prever + 1`/" >> fixver.sed

echo "Patching makefile.ver..."
cp makefile.ver fixver.tmp
sed -f fixver.sed fixver.tmp > makefile.ver

# patch allegro-config.in, allegro-config.be and allegro-config.qnx
echo "s/version=.*/version=$1.$2.$3/" >> fixver.sed

echo "Patching misc/allegro-config.in..."
cp misc/allegro-config.in fixver.tmp
sed -f fixver.sed fixver.tmp > misc/allegro-config.in

echo "Patching misc/allegro-config.be..."
cp misc/allegro-config.be fixver.tmp
sed -f fixver.sed fixver.tmp > misc/allegro-config.be

echo "Patching misc/allegro-config.qnx..."
cp misc/allegro-config.qnx fixver.tmp
sed -f fixver.sed fixver.tmp > misc/allegro-config.qnx

# patch modules.in
echo "Patching misc/modules.in.."
cp misc/modules.in fixver.tmp
sed -e "s/[0-9][0-9.]*[0-9]/$1.$2.$3/g" fixver.tmp > misc/modules.in

# patch the spec files
echo "s/\\_\/__\/     Version .*/\\_\/__\/     Version $verstr/" > fixver.sed
echo "s/Version:        .*/Version:        $1.$2.$3/" >> fixver.sed
echo "s/allegro-[0-9][0-9.]*[0-9]/allegro-$1.$2.$3/g" >> fixver.sed

echo "Patching misc/allegro.spec..."
cp misc/allegro.spec fixver.tmp
sed -f fixver.sed fixver.tmp > misc/allegro.spec

echo "Patching misc/allegro-enduser.spec..."
cp misc/allegro-enduser.spec fixver.tmp
sed -f fixver.sed fixver.tmp > misc/allegro-enduser.spec

# clean up after ourselves
rm fixver.sed fixver.tmp

echo "Done!"
