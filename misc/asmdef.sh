#!/bin/sh

# This script, by Neil Townsend, is for cross-compilers to parse the `asmdef.s'
# file and generate `asmdef.inc'.

# 1. Get the comments
grep "/\*" $1 | cut -f 2 -d '"' | cut -f 1 -d \\ > $2
echo >> $2

# 2. Get the real stuff
list=`grep ".long" $1 | sed -e s/.*.long//`

while [ 1 = 1 ]; do
    c=`echo $list | cut -f 1 -d " "`:
    if [ $c = "0:" ]; then break; fi
    n=`awk /$c/,/ascii/ $1 | grep "ascii" | cut -f 2 -d '"' | cut -f 1 -d '\'`
    f=`echo $n | cut -b 1`
    if [ $f = "#" ]; then
      f2=`echo $n | cut -b 2`
      if [ $f2 = "#" ]; then
        n=`echo $n | cut -b 3-`
	echo "#ifndef $n" >> $2
	echo "#define $n" >> $2
	echo "#endif" >> $2
	echo >> $2
      else
        echo $n >> $2
      fi
    else
      v=`echo $list | cut -f 2 -d " "`
      if [ $n = "NEWLINE" ]; then echo >> $2;
      else
        echo "#define $n $v" >> $2
      fi
    fi
    list=`echo $list | cut -f 3- -d " "`
done

echo >> $2
