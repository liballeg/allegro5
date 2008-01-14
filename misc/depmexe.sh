#! /bin/sh

# Generate dependencies and rules for multifile binaries like the
# makedoc program and demo games.
# Usage is 'script binary_target sourcefiles'

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
            if test "$first" = "demos/skater/skater"; then
               echo "\$(OBJDIR)/skater/$name\$(OBJ): $file"
               echo "	\$(COMPILE_PROGRAM) -c $file -o \$(OBJDIR)/skater/$name\$(OBJ)"
               depend="$depend \$(OBJDIR)/skater/$name\$(OBJ)"
            elif test "$first" = "demos/skater/skater_agl"; then
               echo "\$(OBJDIR)/skater_agl/$name\$(OBJ): $file"
               echo "	\$(COMPILE_PROGRAM) -Iaddons/allegrogl/include \
-DDEMO_WITH_ALLEGRO_GL -c $file -o \$(OBJDIR)/skater_agl/$name\$(OBJ)"
               depend="$depend \$(OBJDIR)/skater_agl/$name\$(OBJ)"
            elif test "$first" = "demos/shooter/shooter"; then
               echo "\$(OBJDIR)/shooter/$name\$(OBJ): $file"
               echo "	\$(COMPILE_PROGRAM) -c $file -o \$(OBJDIR)/shooter/$name\$(OBJ)"
               depend="$depend \$(OBJDIR)/shooter/$name\$(OBJ)"
            else
               echo "\$(OBJDIR)/$name\$(OBJ): $file"
	       echo "	\$(COMPILE_PROGRAM) -c $file -o \$(OBJDIR)/$name\$(OBJ)"
               depend="$depend \$(OBJDIR)/$name\$(OBJ)"
	    fi
	    echo ""
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
      if test "$first" = "demos/shooter/shooter"; then
         echo "	\$(LINK) -o $first $depend \$(LINK_LIBALLEG) \$(LIBS)"
      elif test "$first" = "demos/skater/skater"; then
         echo "	\$(LINK) -o $first $depend \$(LINK_LIBALLEG) \$(LIBS)"
      elif test "$first" = "demos/skater/skater_agl"; then
         echo "	\$(LINK) -o $first $depend \$(ALLEGRO_GL_LFLAGS) \$(LINK_LIBALLEG) \$(LIBS)"
      else
         echo "	\$(LINK) -o $first $depend"
      fi
   else
      echo "# Missing object files for $first, couldn't generate target"
   fi
else
   echo "# depmexe.sh script run without parameters!"
fi

