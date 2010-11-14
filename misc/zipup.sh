#! /bin/sh
#
#  Shell script to create a distribution zip, generating the manifest
#  file and optionally building diffs against a previous version. This
#  should be run from the allegro directory, but will leave the zip 
#  files in the parent directory.
#
#  This script tries to generate dependencies for all the supported
#  compilers, which involves considerable skulduggery to make sure it
#  will work on any Unix-like platform, even if the compilers are not
#  available themselves.
#
#  Note: if you write datestamp in the archive_name field, then the
#  resulting archive will be datestamped. This is in particular useful
#  for making SVN snapshots.
#
#  This script does not try to clean up your workspace so when making
#  a release, do so from a freshly checked out workspace.


if [ $# -lt 1 -o $# -gt 2 ]; then
   echo "Usage: zipup archive_name [previous_archive]" 1>&2
   exit 1
fi

if [ -n "$2" ] && [ ! -r ../"$2" ]; then
   echo "Previous archive $2 not in parent directory, aborting"
   exit 1
fi



# strip off the path and extension from our arguments
name=$(echo "$1" | sed -e 's/.*[\\\/]//; s/\.zip//')
prev=$(echo "$2" | sed -e 's/.*[\\\/]//; s/\.zip//')

# make a datestamped archive if specified
if test $name = datestamp; then
  date=`date '+%Y%m%d'`
  name=allegro_$date
fi

# make sure all the makefiles are in Unix text format
# for file in makefile.*; do
#    mv $file _tmpfile
#    tr -d \\\r < _tmpfile > $file
#    touch -r _tmpfile $file
#    rm _tmpfile
# done


# delete all generated files
# echo "Cleaning the Allegro tree..."
# 
# sed -n -e "/CLEAN_FILES/,/^$/p; /^ALLEGRO_.*_EXES/,/^$/p" makefile.lst | \
#    sed -e "/CLEAN_FILES/d; /ALLEGRO_.*_EXES/d; s/\\\\//g" | \
#    xargs -n 1 echo | \
#    sed -e "s/\(.*\)/-c \"rm -f \1\"/" | \
#    xargs -l sh
# 
# find . -name '*~' -exec rm -f {} \;


# emulation of the djgpp utod utility program (Unix to DOS text format)
utod()
{
   for file in $*; do
      if echo $file | grep "^\.\.\./" >/dev/null; then
	 # files like .../*.c recurse into directories (emulating djgpp libc)
	 spec=$(echo $file | sed -e "s/^\.\.\.\///")
	 find . -type f -name "$spec" -exec perl -p -i -e 's/([^\r]|^)\n/\1\r\n/' {} \;
      else
	 perl -p -e "s/([^\r]|^)\n/\1\r\n/" $file > _tmpfile
	 touch -r $file _tmpfile
	 mv _tmpfile $file
      fi
   done
}


# generate dependencies for DJGPP
# echo "Generating DJGPP dependencies..."
# 
# ./fix.sh djgpp --quick
# 
# make depend UNIX_TOOLS=1 CC=gcc


# generate dependencies for Watcom
# echo "Generating Watcom dependencies..."
# 
# ./fix.sh watcom --quick
# 
# make depend UNIX_TOOLS=1 CC=gcc


# generate dependencies for MSVC
# echo "Generating MSVC dependencies..."
# 
# ./fix.sh msvc --quick
# 
# make depend UNIX_TOOLS=1 CC=gcc


# generate dependencies for MinGW
# echo "Generating MinGW dependencies..."
# 
# ./fix.sh mingw --quick
# 
# make depend UNIX_TOOLS=1 CC=gcc


# generate dependencies for Borland C++
# echo "Generating BCC32 dependencies..."
# 
# ./fix.sh bcc32 --quick
# 
# make depend UNIX_TOOLS=1 CC=gcc


# generate dependencies for BeOS
# echo "Generating BeOS dependencies..."
# 
# ./fix.sh beos --quick
# 
# make depend UNIX_TOOLS=1 CC=gcc


# generate dependencies for QNX
# echo "Generating QNX dependencies..."
# 
# ./fix.sh qnx --quick
# 
# make depend UNIX_TOOLS=1 CC=gcc


# generate dependencies for MacOS X
# echo "Generating MacOS X dependencies..."
# 
# ./fix.sh macosx --quick
# 
# make depend UNIX_TOOLS=1 CC=gcc


# generate the DLL export definition files for Windows compilers
# misc/fixdll.sh

# running autoconf
# echo "Running autoconf to generate configure script..."
# autoconf || exit 1

# touch stamp-h.in so the user doesn't need autoheader to compile
# touch stamp-h.in


# convert documentation from the ._tx source files
# echo "Converting documentation..."
#
# gcc -o _makedoc.exe docs/src/makedoc/*.c
#
# ./_makedoc.exe -ascii readme.txt docs/src/readme._tx
# ./_makedoc.exe -ascii CHANGES docs/src/changes._tx
# ./_makedoc.exe -part -ascii AUTHORS docs/src/thanks._tx
# ./_makedoc.exe -part -ascii THANKS docs/src/thanks._tx
# for base in abi ahack allegro const faq help mistakes; do
#    ./_makedoc.exe -ascii docs/txt/$base.txt docs/src/$base._tx
# done
# for base in bcc32 beos darwin djgpp linux macosx mingw32 msvc qnx unix watcom; do
#    ./_makedoc.exe -ascii docs/build/$base.txt docs/src/build/$base._tx
# done
#
#
# rm _makedoc.exe


# convert documentation from pandoc-format source files
if which cmake >/dev/null && which pandoc >/dev/null
then
   echo "Generating documentation from Pandoc source files..."
   builddir=,,zipup_builddir.$$
   trap 'rm -rf $builddir' 0 1 2 3 13 15
   mkdir $builddir
   ( cd $builddir
       cmake ..
       make -j4 docs html man
       mv docs/txt/changes-5.0.txt ../CHANGES-5.0.txt

       test -d ../docs/html || mkdir -p ../docs/html
       rm -rf ../docs/html/refman
       mv docs/html/refman ../docs/html/refman

       rm -rf ../docs/man
       mv docs/man ../docs/man
   ) || exit 1
   rm -rf $builddir
else
   echo "WARNING: CMake or Pandoc not found, skipping step" 1>&2
fi


# generate NaturalDocs documentation
# if which NaturalDocs >/dev/null
# then
#    echo "Generating NaturalDocs..."
#    ( cd docs/naturaldocs
#       make clean
#       make public
#    ) || exit 1
# else
#    echo "WARNING: NaturalDocs not found, skipping step" 1>&2
# fi


# create language.dat and keyboard.dat files
# misc/mkdata.sh || exit 1


# convert files to djgpp format for distribution
# ./fix.sh djgpp --utod


# convert line endings to CR/LF
./misc/convert_line_endings.sh --utod || exit 1


# recursive helper to fill any empty directories with a tmpfile.txt
scan_for_empties()
{
   if [ -f $1/tmpfile.txt ]; then rm $1/tmpfile.txt; fi

   files=`echo $1/*`
   if [ "$files" = "$1/CVS" -o "$files" = "$1/cvs" -o "$files" = "$1/*" ]; then
      echo "This file is needed because some unzip programs skip empty directories." > $1/tmpfile.txt
   else
      for file in $files; do
	 if [ -d $file ]; then
	    scan_for_empties $file
	 fi
      done
   fi
}


#echo "Filling empty directories with tmpfile.txt..."
scan_for_empties "."


# build the main zip archive
echo "Creating $name.zip..."
if [ -f .dist/$name.zip ]; then rm .dist/$name.zip; fi
rm -rf ./autom4te*
mkdir -p .dist/allegro
# Note: we use -print0 and xargs -0 to handle file names with spaces.
find . -type f "(" -path "*/.*" -prune -o -iname "*.rej" \
    -prune -o -iname "*.orig" -prune -o -print0 ")" | \
    xargs -0 cp -a --parents -t .dist/allegro

# from now on, the scripts runs inside .dist
cd .dist


# If 7za is available, use that to produce .7z files.
# I used to produce .zip files with it as well but I noticed upon extraction
# files with timestamps 10 hours into the future (I'm at UTC+10).  .7z files
# seem to be unaffected.  This was with p7zip 4.65.
if 7za > /dev/null ; then
   rm -f $name.7z
   if [ -n "$FAST_ZIPUP" ]; then
      7za a -mx0 $name.7z allegro
   else
      7za a -mx9 -ms=on $name.7z allegro
   fi
fi

rm -f $name.zip
if [ -n "$FAST_ZIPUP" ]; then
   zip -0 -r $name.zip allegro
else
   zip -9 -r $name.zip allegro
fi


# generate the manifest file
echo "Generating allegro.mft..."
unzip -Z1 $name.zip | sort > allegro/allegro.mft
echo "allegro/allegro.mft" >> allegro/allegro.mft
utod allegro/allegro.mft
zip -9 $name.zip allegro/allegro.mft

if [ -r $name.7z ]; then
   7za a $name.7z allegro/allegro.mft
fi


# if we are building diffs as well, do those
if [ $# -eq 2 ]; then

   echo "Inflating current version ($name.zip)..."
   mkdir current
   unzip -q $name.zip -d current

   echo "Inflating previous version ($2)..."
   mkdir previous
   unzip -q ../../"$2" -d previous || exit 1

   echo "Generating diffs..."
   diff -U 3 -N --recursive previous/ current/ > $name.diff

   echo "Deleting temp files..."
   rm -r previous
   rm -r current


   # generate the .txt file for the diff archive
   echo "This is a set of diffs to upgrade from $prev to $name." > $name.txt
   echo >> $name.txt
   echo "Date: $(date '+%A %B %d, %Y, %H:%M')" >> $name.txt
   echo >> $name.txt
   echo "To apply this patch, copy $name.diff into the same directory where you" >> $name.txt
   echo "installed Allegro (this should be one level up from the allegro/ dir, eg." >> $name.txt
   echo "if your Allegro installation lives in c:/djgpp/allegro/, you should be in" >> $name.txt
   echo "c:/djgpp/). Then type the command:" >> $name.txt
   echo >> $name.txt
   echo "   patch -p1 < $name.diff" >> $name.txt
   echo >> $name.txt
   echo "Change into the allegro directory, run make, and enjoy!" >> $name.txt

   utod $name.txt


   # zip up the diff archive
   echo "Creating ${name}_diff.zip..."
   if [ -f ${name}_diff.zip ]; then rm ${name}_diff.zip; fi
   zip -9 ${name}_diff.zip $name.diff $name.txt


   # find if we need to add any binary files as well
   bin=$(sed -n -e "s/Binary files previous\/\(.*\) and current.* differ/\1/p" $name.diff)

   if [ "$bin" != "" ]; then
      echo "Adding binary diffs..."
      zip -9 ${name}_diff.zip $bin
   fi

   rm $name.diff $name.txt

fi


# convert line endings to back to LF
cd ..
./misc/convert_line_endings.sh --dtou || exit 1
