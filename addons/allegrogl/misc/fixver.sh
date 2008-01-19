#! /bin/sh
#
#  Shell script to adjust the version numbers in various AllegroGL files.
#
#  Note: if you pass "datestamp" as the only argument, then the version
#  digits will remain unchanged and the comment will be set to the date.
#  This is in particular useful for making SVN snapshots.

set -e


if [ $# -lt 3 -o $# -gt 4 ]; then
   if [ $# -eq 1 -a $1 == "datestamp" ]; then
      ver=`grep "^AllegroGL - [0-9]" readme.txt | awk '{ print $3 }'`
      split_ver=`echo $ver | tr '.' ' '`
      datestamp=`date '+%Y%m%d'`
      exec $0 $split_ver $datestamp
   else
      echo "Usage: fixver major_num sub_num wip_num [comment]" 1>&2
      echo "   or: fixver datestamp" 1>&2
      echo "Example: fixver 0 4 4 WIP" 1>&2
      exit 1
   fi
fi

# get the version and date strings in a nice format
if [ $# -eq 3 ]; then
   verstr="$1.$2.$3"
else
   verstr="$1.$2.$3 ($4)"
fi


# Patch routine.
# Write to $sed_script then call dopatch <file1> <file2> ...

sed_script=fixver.sed
tmp_file=fixver.tmp
errcode=0

function dopatch() {
   for x in $* ; do
      if [ -f $x ]; then
         echo "Patching $x..."
         cp $x $tmp_file
         sed -f $sed_script $tmp_file > $x
      else
         echo "WARNING: $x does not exist, skipping"
         errcode=1
      fi
   done
}


# patch include/alleggl.h
cat > $sed_script << END_OF_SED
s#\(define AGL_VERSION.*\)[0-9]#\1$1#
s#\(define AGL_SUB_VERSION.*\)[0-9]#\1$2#
s#\(define AGL_WIP_VERSION.*\)[0-9]#\1$3#
s#\(define AGL_VERSION_STR.*"\)[^"]\+"#\1$verstr"#
END_OF_SED

dopatch include/alleggl.h


# patch readme.txt
cat > $sed_script << END_OF_SED
s#^\(AllegroGL - \).*#\1$verstr#
END_OF_SED

dopatch readme.txt



# patch make/makefile.ver
cat > $sed_script << END_OF_SED
s#^\(shared_version =\).*#\1 $1.$2.$3#
s#^\(shared_major_minor =\).*#\1 $1.$2#
s#^\(shared_major =\).*#\1 $1#
END_OF_SED

dopatch make/makefile.ver


# patch projects/msvc2005/allegrogl/allegrogl.vcproj
# There are two instances of Version= in the file.  We rely on the fact that
# the version we _don't_ want to change contains a comma, and that the
# AllegroGL version number doesn't.
cat > $sed_script << END_OF_SED
s#\(Version="\)[^",]\+"#\1$verstr"#
END_OF_SED

dopatch projects/msvc2005/allegrogl/allegrogl.vcproj


# patch projects/msvc6/allegrogldll/allegrogldll.dsp
cat > $sed_script << END_OF_SED
s#\(version:\)[0-9][.][0-9]\+#\1$1.$2$3#
END_OF_SED

dopatch projects/msvc6/allegrogldll/allegrogldll.dsp


# patch docs/Doxyfile
cat > $sed_script << END_OF_SED
s#^\(PROJECT_NUMBER.*=\).*#\1 $verstr#
END_OF_SED

dopatch docs/Doxyfile


# clean up after ourselves
rm -f $sed_script $tmp_file

echo "Done!"
exit $errcode

# vim: sts=3 sw=3 et
