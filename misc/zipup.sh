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
#  for making CVS snapshots.


if [ $# -lt 1 -o $# -gt 2 ]; then
   echo "Usage: zipup archive_name [previous_archive]" 1>&2
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
for file in makefile.*; do
   mv $file _tmpfile
   tr -d \\\r < _tmpfile > $file
   rm _tmpfile
done


# delete all generated files
echo "Cleaning the Allegro tree..."

sed -n -e "/CLEAN_FILES/,/^$/p; /^ALLEGRO_.*_EXES/,/^$/p" makefile.lst | \
   sed -e "/CLEAN_FILES/d; /ALLEGRO_.*_EXES/d; s/\\\\//g" | \
   xargs -n 1 echo | \
   sed -e "s/\(.*\)/-c \"rm -f \1\"/" | \
   xargs -l sh

find . -name '*~' -exec rm -f {} \;


# emulation of the djgpp utod utility program (Unix to DOS text format)
utod()
{
   for file in $*; do
      if echo $file | grep -q "^\.\.\./"; then
	 # files like .../*.c recurse into directories (emulating djgpp libc)
	 spec=$(echo $file | sed -e "s/^\.\.\.\///")
	 find . -type f -name "$spec" -exec perl -p -i -e 's/([^\r]|^)\n/\1\r\n/' {} \;
      else
	 perl -p -i -e "s/([^\r]|^)\n/\1\r\n/" $file
      fi
   done
}

export UNIX_TOOLS=1

# generate dependencies for djgpp
echo "Generating djgpp dependencies..."

./fix.sh djgpp --quick

make depend


# generate dependencies for Watcom
echo "Generating Watcom dependencies..."

./fix.sh watcom --quick

make depend


# generate dependencies for MSVC
echo "Generating MSVC dependencies..."

./fix.sh msvc --quick

make depend


# generate dependencies for MinGW32
echo "Generating Mingw32 dependencies..."

./fix.sh mingw32 --quick

make depend


# generate dependencies for Borland C++
echo "Generating BCC32 dependencies..."

./fix.sh bcc32 --quick

make depend


# generate dependencies for BeOS
echo "Generating BeOS dependencies..."

./fix.sh beos --quick

make depend


# generate dependencies for QNX
echo "Generating QNX dependencies..."

./fix.sh qnx --quick

make depend

unset UNIX_TOOLS


# generate the DLL export definition files for Windows compilers
misc/fixdll.sh

# running autoconf
echo "Running autoconf to generate configure script..."
autoconf

# touch stamp-h.in so the user doesn't need autoheader to compile
touch stamp-h.in

# convert documentation from the ._tx source files
echo "Converting documentation..."

tr -d \\\r < docs/makedoc.c > _tmpdoc.c
gcc _tmpdoc.c -o _makedoc.exe

./_makedoc.exe -ascii CHANGES docs/src/changes._tx
./_makedoc.exe -part -ascii AUTHORS docs/src/thanks._tx
./_makedoc.exe -part -ascii THANKS docs/src/thanks._tx
for base in abi ahack allegro const faq help; do
   ./_makedoc.exe -ascii docs/txt/$base.txt docs/src/$base._tx
done

rm _tmpdoc.c _makedoc.exe


# create language.dat and keyboard.dat files
misc/mkdata.sh


# convert files to djgpp format for distribution
./fix.sh djgpp --utod


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
cd ..
if [ -f $name.zip ]; then rm $name.zip; fi
find allegro -iname "CVS" -prune -o -iname ".*" -prune -o -iname "*.rej" -prune -o -iname "*.orig" -prune -o -print | zip -9 $name.zip -@


# generate the manifest file
echo "Generating allegro.mft..."
unzip -Z1 $name.zip | sort > allegro/allegro.mft
echo "allegro/allegro.mft" >> allegro/allegro.mft
utod allegro/allegro.mft
zip -9 $name.zip allegro/allegro.mft


# if we are building diffs as well, do those
if [ $# -eq 2 ]; then

   echo "Inflating current version ($name.zip)..."
   mkdir current
   unzip -q $name.zip -d current

   echo "Inflating previous version ($2)..."
   mkdir previous
   unzip -q "$2" -d previous

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


echo "Done!"

