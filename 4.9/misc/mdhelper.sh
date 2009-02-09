#!/bin/sh
# Used by makefile.all's create-install-dirs target.

for dir in $*; do
   if [ ! -d "$dir" ]; then
      mkdir $dir
      if [ $? != 0 ]; then
         exit 1
      fi
   fi
done
