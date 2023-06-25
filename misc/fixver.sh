#! /bin/sh -e
#
#  Shell script to adjust the version numbers and dates in files.
#
#  Note: if you pass "datestamp" as the only argument, then the version
#  digits will remain unchanged and the comment will be set to the date.
#  This is in particular useful for making SVN snapshots.


if [ $# -eq 1 -a $1 == "datestamp" ]; then
   # Uses GNU grep -o.
   ver=$( grep -o "ALLEGRO_VERSION [0-9.]*" CMakeLists.txt | cut -d' ' -f2 )
   major_num=$( echo $ver | cut -d. -f1 )
   sub_num=$( echo $ver | cut -d. -f2 )
   wip_num=$( echo $ver | cut -d. -f3 )
   $0 $major_num $sub_num $wip_num `date '+%Y%m%d'`
   exit 0
fi

case $# in
   3|4|5) ;;
   *)
      echo "Usage: fixver major_num sub_num wip_num [n] [comment]" 1>&2
      echo "   or: fixver datestamp" 1>&2
      echo "Example: fixver 4 9 14 SVN" 1>&2
      echo "         fixver 4 9 14 WIP" 1>&2
      echo "         fixver 4 9 14 1 WIP" 1>&2
      echo "         fixver 5 0 0" 1>&2
      exit 1
      ;;
esac

# get the version and date strings in a nice format
case $# in
   3)
      # Proper release.
      fourth=1
      comment=
      ;;
   4)
      case $4 in
         WIP)
            # Proper release
            fourth=1
            comment=WIP
            ;;
         [0-9])
            # Proper release
            fourth=$(( $4 + 1 ))
            comment=
            ;;
         *)
            fourth=0
            comment=$4
            ;;
      esac
      ;;
   5)
      fourth=$(( $4 + 1 ))
      comment=$5
      ;;
esac

if [ -z "$comment" ]; then
   verstr="$1.$2.$3"
else
   verstr="$1.$2.$3 ($comment)"
fi

year=$(date +%Y)
month=$(date +%m)
day=$(date +%d)
datestr="$(date +%b) $day, $year"

# patch allegro/base.h
echo "s/\#define ALLEGRO_VERSION .*/\#define ALLEGRO_VERSION          $1/" > fixver.sed
echo "s/\#define ALLEGRO_SUB_VERSION .*/\#define ALLEGRO_SUB_VERSION      $2/" >> fixver.sed
echo "s/\#define ALLEGRO_WIP_VERSION .*/\#define ALLEGRO_WIP_VERSION      $3/" >> fixver.sed
echo "s/\#define ALLEGRO_RELEASE_NUMBER .*/\#define ALLEGRO_RELEASE_NUMBER   $fourth/" >> fixver.sed
echo "s/\#define ALLEGRO_VERSION_STR .*/\#define ALLEGRO_VERSION_STR      \"$verstr\"/" >> fixver.sed
echo "s/\#define ALLEGRO_DATE_STR .*/\#define ALLEGRO_DATE_STR         \"$year\"/" >> fixver.sed
echo "s/\#define ALLEGRO_DATE .*/\#define ALLEGRO_DATE             $year$month$day    \/\* yyyymmdd \*\//" >> fixver.sed

echo "Patching include/allegro5/base.h..."
cp include/allegro5/base.h fixver.tmp
sed -f fixver.sed fixver.tmp > include/allegro5/base.h

# patch CMakeLists.txt
echo "Patching CMakeLists.txt..."
cp CMakeLists.txt fixver.tmp
sed -e "s/set(ALLEGRO_VERSION [^)]*)/set(ALLEGRO_VERSION $1.$2.$3)/" fixver.tmp > CMakeLists.txt

# clean up after ourselves
rm fixver.sed fixver.tmp

echo "Done!"
