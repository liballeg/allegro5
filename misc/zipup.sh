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
#  available themselves, and for converting things like fixdll.bat and
#  fixdjgpp.bat into something that bash can understand.


if [ $# -lt 1 -o $# -gt 2 ]; then
   echo "Usage: zipup archive_name [previous_archive]" 1>&2
   exit 1
fi


# strip off the path and extension from our arguments
name=$(echo "$1" | sed -e 's/.*[\\\/]//; s/\.zip//')
prev=$(echo "$2" | sed -e 's/.*[\\\/]//; s/\.zip//')


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


# helper for turning DOS format batch files into something that Bash can run
run_batch_file()
{
   echo "Running $1..."

   # export utod() code into the generated script
   sed -n -e "/^utod()$/,/^}$/p" $0 > _tmpbat2

   # DOS to Unix line endings
   tr -d \\\r < $1 > _tmpbat1

   # batch to shell syntax
   sed -e "/^@echo/d" \
       -e "/^rem/d" \
       -e "s/^echo \([^>]*\)\(.*\)/echo \"\1\"\2/; s/ \">/\" >/" \
       -e "s/^copy/cp/; s/ > nul//" \
       -e "s/^\([^\"]*\)\\\\/\1\//g; s/^\([^\"]*\)\\\\/\1\//g; s/^\([^\"]*\)\\\\/\1\//g" \
       -e "s/\\\\\([^\"]*\)$/\/\1/g; s/\\\\\([^\"]*\)$/\/\1/g; s/\\\\\([^\"]*\)$/\/\1/g" \
       -e "s/\\\\/\\\\\\\\/g" \
       _tmpbat1 >> _tmpbat2

   sh _tmpbat2

   rm _tmpbat1 _tmpbat2
}


# generate the DLL linkage files for Windows compilers
run_batch_file fixdll.bat

utod lib/msvc/allegro.def
utod lib/mingw32/allegro.def
utod include/allegro/rsxdll.h


# generate dependencies for MSVC
echo "Generating MSVC dependencies..."

echo "MAKEFILE_INC = makefile.vc" > makefile
echo "include makefile.all" >> makefile

make depend

utod obj/msvc/*/makefile.dep


# generate dependencies for Watcom
echo "Generating Watcom dependencies..."

echo "MAKEFILE_INC = makefile.wat" > makefile
echo "include makefile.all" >> makefile

make depend

utod obj/watcom/*/makefile.dep


# generate dependencies for RSXNT
echo "Generating RSXNT dependencies..."

echo "MAKEFILE_INC = makefile.rsx" > makefile
echo "include makefile.all" >> makefile

make depend

utod obj/rsxnt/*/makefile.dep


# generate dependencies for Mingw32
echo "Generating Mingw32 dependencies..."

echo "MAKEFILE_INC = makefile.mgw" > makefile
echo "include makefile.all" >> makefile

make depend

utod obj/mingw32/*/makefile.dep


# generate dependencies for Borland
echo "Generating Borland dependencies..."

echo "MAKEFILE_INC = makefile.bcc" > makefile
echo "include makefile.all" >> makefile

make depend

utod obj/bcc32/*/makefile.dep


# generate dependencies for BeOS
echo "Generating BeOS dependencies..."

echo "MAKEFILE_INC = makefile.be" > makefile
echo "include makefile.all" >> makefile

make depend

utod obj/beos/*/makefile.dep


# generate dependencies for djgpp
echo "Generating djgpp dependencies..."

echo "MAKEFILE_INC = makefile.dj" > makefile
echo "include makefile.all" >> makefile

make depend

utod obj/djgpp/*/makefile.dep


# convert documentation from the ._tx source files
echo "Converting documentation..."

tr -d \\\r < docs/makedoc.c > _tmpdoc.c
gcc _tmpdoc.c -o _makedoc.exe

./_makedoc.exe -ascii CHANGES docs/changes._tx
./_makedoc.exe -part -ascii AUTHORS docs/thanks._tx
./_makedoc.exe -part -ascii THANKS docs/thanks._tx
./_makedoc.exe -ascii faq.txt docs/faq._tx

utod CHANGES AUTHORS THANKS

rm _tmpdoc.c _makedoc.exe


# convert files to djgpp format for distribution
run_batch_file fixdjgpp.bat

utod makefile


# recursive helper to fill any empty directories with a tmpfile.txt
scan_for_empties()
{
   if [ -f $1/tmpfile.txt ]; then rm $1/tmpfile.txt; fi

   for file in $1/*; do
      if [ -d $file ]; then
	 scan_for_empties $file
      elif [ $file = "$1/*" ]; then
	 echo "This file is needed because some unzip programs skip empty directories." > $1/tmpfile.txt
      fi
   done
}


#echo "Filling empty directories with tmpfile.txt..."
scan_for_empties "."


# build the main zip archive
echo "Creating $name.zip..."
cd ..
if [ -f $name.zip ]; then rm $name.zip; fi
zip -9 -r $name.zip allegro


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

