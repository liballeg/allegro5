#! /bin/sh

# Generate dependencies and rules for multifile binaries like the
# makedoc program. Usage is 'script binary_target sourcefiles'

first=$1
depend=
missing=
if test -n "$first"; then
   for file in $*; do
      if test $first != $file; then
	 if test -f $file; then
	    dir=`echo $file | sed 's,/[^/]*$,,'`
	    name=`echo $file | sed 's,^.*/,,' | sed 's,\.[^.]*$,,'`
	    ext=`echo $file | sed 's,^.*\.,,'`
	    echo "\$(OBJDIR)/$name\$(OBJ): $file"
	    echo "	\$(COMPILE_PROGRAM) -c $file -o \$(OBJDIR)/$name\$(OBJ)"
	    echo ""
	    depend="$depend \$(OBJDIR)/$name\$(OBJ)"
	 else
	    missing="$missing $file"
	 fi
      fi
   done

   if test -n "$missing"; then
      echo "# These files don't exist: $missing"
   fi
   
   if test -n "$depend"; then
      echo "$first: $depend"
      if test "$first" = "demo/demo"; then
         echo "	\$(LINK) -o $first $depend \$(LINK_LIBALLEG) \$(LIBS)"
      else
         echo "	\$(LINK) -o $first $depend"
      fi
   else
      echo "# Missing object files for $first, couldn't generate target"
   fi
else
   echo "# depmexe.sh script run without parameters!"
fi

