#! /bin/sh

# Generate X11 icon
# Usage: xfixicon iconfile

if test -z "$1"; then
   echo "Usage:"
   echo "   xfixicon iconfile [-o outputfile]"
   echo "this will generate a C file that can be linked with your application"
   echo "to set the X11 icon automatically."
   echo ""
   echo "Options:"
   echo "  -o  Set the name of the output file. Default name is allegro_icon.c"
   exit
fi

outfile="allegro_icon.c"

while !(test -z "$1"); do
   if (test "$1" = "-o"); then
      outfile=$2
      shift
   else
      file=$1
   fi
   shift
done

if !(test -e "$file"); then
   echo "File not found: $file"
   exit 1
fi

if !(convert -transparent "magenta" "$file" "/tmp/allegico_xpm.xpm"); then
   echo "Conversion failed"
   exit 1
fi

echo "#include <allegro.h>" > $outfile
cat /tmp/allegico_xpm.xpm | sed -e 's,static char,static const char,' >> $outfile
echo "#if defined ALLEGRO_WITH_XWINDOWS && defined ALLEGRO_USE_CONSTRUCTOR" >> $outfile
echo "extern void *allegro_icon;" >>  $outfile
echo "CONSTRUCTOR_FUNCTION(static void _set_allegro_icon(void));" >> $outfile
echo "static void _set_allegro_icon(void)" >> $outfile
echo "{" >> $outfile
echo "    allegro_icon = allegico_xpm;" >> $outfile
echo "}" >> $outfile
echo "#endif" >> $outfile

rm /tmp/allegico_xpm.xpm
