#!/bin/sh
#
# This file is only used now to convert file line endings by misc/zipup.sh.
# We used to do this in fix.sh/fix.bat but those files have been removed
# in the 4.9.x series as running them could confuse the CMake and Scons builds,
# plus the old build system is not being maintained.
#

proc_filelist()
{
   # common files. This must not include any shell scripts or batch files.
   AL_FILELIST=`find . -type f "(" ! -path "*/.*" ")" -a "(" \
      -name "*.c" -o -name "*.cfg" -o -name "*.cpp" -o -name "*.def" -o \
      -name "*.h" -o -name "*.hin" -o -name "*.in" -o -name "*.inc" -o \
      -name "*.m" -o -name "*.m4" -o -name "*.mft" -o -name "*.s" -o \
      -name "*.rc" -o -name "*.rh" -o -name "*.spec" -o -name "*.pl" -o \
      -name "*.txt" -o -name "*._tx" -o -name "makefile*" -o \
      -name "*.inl" -o -name "CHANGES" -o \
      -name "AUTHORS" -o -name "THANKS" ")" \
      -a ! -path "*.dist*" \
   `

   # touch unix shell scripts?
   if [ "$1" != "omit_sh" ]; then
      AL_FILELIST="$AL_FILELIST `find . -type f -name '*.sh' -o \
         -name 'configure' -a ! -path "*addons*"`"
   fi

   # touch DOS batch files?
   if [ "$1" != "omit_bat" ]; then
      AL_FILELIST="$AL_FILELIST `find . -type f -name '*.bat' -a \
         ! -path "*addons*"`"
   fi
}

proc_utod()
{
   echo "Converting files from Unix to DOS/Win32 ..."
   proc_filelist "omit_sh"
   /bin/sh misc/utod.sh $AL_FILELIST
}

proc_dtou()
{
   echo "Converting files from DOS/Win32 to Unix ..."
   proc_filelist "omit_bat"
   /bin/sh misc/dtou.sh $AL_FILELIST
}

trap 'exit $?' 1 2 3 13 15

case "$1" in
  "--utod"  ) proc_utod ;;
  "--dtou"  ) proc_dtou ;;
esac

echo "Done!"
