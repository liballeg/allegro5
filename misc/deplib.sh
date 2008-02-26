#! /bin/sh

# Generate dependencies and rules for building libraries.


# write_code:  Writes the Make rules to create a set of libraries.
#  Pass the library type (e.g. `alleg') and the object list variables' prefix
#  (e.g. 'LIBALLEG').  What a nasty mixture of shell and Make variables...
write_code() {
    staticlib="lib${1}.a"
    staticobj="\$(${2}_OBJECTS)"

    sharelib="lib${1}-\$(shared_version).so"
    shareobj="\$(${2}_SHARED_OBJECTS)"

    unsharelib="lib${1}_unsharable.a"
    unshareobj="\$(${2}_UNSHARED_OBJECTS)"
    
    echo "\$(LIBDIR)/${staticlib}: ${staticobj}"
    echo "	rm -f \$@"
    echo "	\$(AR) \$@ ${staticobj}"
    echo ""
    echo "\$(LIBDIR)/${sharelib}: ${shareobj}"
    echo "	rm -f \$@"
    # gf: This bit is obviously gcc-specific
    # eb: Yes, but the GNU C Compiler doesn't always spell 'gcc'
    echo "	\$(CC) -shared -o \$@ ${shareobj} ${3} \$(LDFLAGS) -Wl,-h,lib${1}.so.\$(shared_major_minor) \$(LIBS)"
    echo ""
    echo "\$(LIBDIR)/${unsharelib}: ${unshareobj}"
    echo "	rm -f \$@"
    echo "	\$(AR) \$@ ${unshareobj}"
}


# See src/unix/udummy.c for the rationale
sources=`echo $* | sed 's,src/,,g'`
sharable_sources=`echo $sources | sed 's,[^.	 ]*\.s,,g'`
sharable_sources=`echo $sharable_sources | sed 's,[	 ]*unix/udummy\.c,,g'`
unsharable_sources=`echo $sources | sed 's,[^.	 ]*\.[^s],,g'`
unsharable_sources=`echo $unsharable_sources unix/udummy.c`

objects=`echo $sources | sed 's,\.[^.	 ]*,,g'`
sharable_objects=`echo $sharable_sources | sed 's,\.[^.	 ]*,,g'`
unsharable_objects=`echo $unsharable_sources | sed 's,\.[^.	 ]*,,g'`


echo \# sources: $sources

# Normal library.
prev="LIBALLEG_OBJECTS ="
for file in .. $objects; do
  if test "$file" != ..; then
    echo "$prev \\"
    prev="  \$(OBJDIR)/alleg/$file\$(OBJ)"
  fi
done
echo "$prev"
prev="LIBALLEG_SHARED_OBJECTS ="
for file in .. $sharable_objects; do
  if test "$file" != ..; then
#    if test -n "$file"; then
	    echo "$prev \\"
	    prev="  \$(OBJDIR)/shared/alleg/$file\$(OBJ)"
#    fi
  fi
done
echo "$prev"
prev="LIBALLEG_UNSHARED_OBJECTS ="
for file in .. $unsharable_objects; do
  if test "$file" != ..; then
    echo "$prev \\"
    prev="  \$(OBJDIR)/shared/alleg/$file\$(OBJ)"
  fi
done
echo "$prev"
echo ""
write_code alleg LIBALLEG -s
echo ""
echo ""


# Debugging library.
prev="LIBALLD_OBJECTS ="
for file in .. $objects; do
  if test "$file" != ..; then
    echo "$prev \\"
    prev="  \$(OBJDIR)/alld/$file\$(OBJ)"
  fi
done
echo "$prev"
prev="LIBALLD_SHARED_OBJECTS ="
for file in .. $sharable_objects; do
  if test "$file" != ..; then
#    if test -n "$file"; then
      echo "$prev \\"
      prev="  \$(OBJDIR)/shared/alld/$file\$(OBJ)"
#    fi
  fi
done
echo "$prev"
prev="LIBALLD_UNSHARED_OBJECTS ="
for file in .. $unsharable_objects; do
  if test "$file" != ..; then
    echo "$prev \\"
    prev="  \$(OBJDIR)/shared/alld/$file\$(OBJ)"
  fi
done
echo "$prev"
echo ""
write_code alld LIBALLD -g
echo ""
echo ""


# Profiling library.
prev="LIBALLP_OBJECTS ="
for file in .. $objects; do
  if test "$file" != ..; then
    echo "$prev \\"
    prev="  \$(OBJDIR)/allp/$file\$(OBJ)"
  fi
done
echo "$prev"
prev="LIBALLP_SHARED_OBJECTS ="
for file in .. $sharable_objects; do
  if test "$file" != ..; then
#    if test -n "$file"; then
      echo "$prev \\"
      prev="  \$(OBJDIR)/shared/allp/$file\$(OBJ)"
#    fi
  fi
done
echo "$prev"
prev="LIBALLP_UNSHARED_OBJECTS ="
for file in .. $unsharable_objects; do
  if test "$file" != ..; then
    echo "$prev \\"
    prev="  \$(OBJDIR)/shared/allp/$file\$(OBJ)"
  fi
done
echo "$prev"
echo ""
write_code allp LIBALLP -pg
echo ""
echo ""


missing=
symbols=
for file in .. $*; do
  if test -f "$file"; then
    dir=`echo $file | sed 's,/[^/]*$,,;s,src,,'`
    name=`echo $file | sed 's,^.*/,/,;s,\.[^.]*$,,'`
    ext=`echo $file | sed 's,^.*\.,,'`
    includes=`./misc/grabdeps.pl $file`
    deps=

    # Normal library.
    echo "\$(OBJDIR)/alleg$dir$name\$(OBJ): \$(srcdir)/$file$includes"
    if test "$ext" = "c"; then
      echo "	\$(COMPILE_NORMAL) -c \$(srcdir)/$file -o \$(OBJDIR)/alleg$dir$name\$(OBJ)"
    else
      echo "	\$(COMPILE_S_NORMAL) -c \$(srcdir)/$file -o \$(OBJDIR)/alleg$dir$name\$(OBJ)"
    fi
    echo "\$(OBJDIR)/shared/alleg$dir$name\$(OBJ): \$(srcdir)/$file$includes"
    if test "$ext" = "c"; then
      echo "	\$(COMPILE_NORMAL) \$(ALLEGRO_SHAREDLIB_CFLAGS) -c \$(srcdir)/$file -o \$(OBJDIR)/shared/alleg$dir$name\$(OBJ)"
    else
      echo "	\$(COMPILE_S_NORMAL) -c \$(srcdir)/$file -o \$(OBJDIR)/shared/alleg$dir$name\$(OBJ)"
    fi
    echo ""

    # Debugging library.
    echo "\$(OBJDIR)/alld$dir$name\$(OBJ): \$(srcdir)/$file$includes"
    if test "$ext" = "c"; then
      echo "	\$(COMPILE_DEBUG) -c \$(srcdir)/$file -o \$(OBJDIR)/alld$dir$name\$(OBJ)"
    else
      echo "	\$(COMPILE_S_DEBUG) -c \$(srcdir)/$file -o \$(OBJDIR)/alld$dir$name\$(OBJ)"
    fi
    echo "\$(OBJDIR)/shared/alld$dir$name\$(OBJ): \$(srcdir)/$file$includes"
    if test "$ext" = "c"; then
      echo "	\$(COMPILE_DEBUG) \$(ALLEGRO_SHAREDLIB_CFLAGS) -c \$(srcdir)/$file -o \$(OBJDIR)/shared/alld$dir$name\$(OBJ)"
    else
      echo "	\$(COMPILE_S_DEBUG) -c \$(srcdir)/$file -o \$(OBJDIR)/shared/alld$dir$name\$(OBJ)"
    fi
    echo ""

    # Profiling library.
    echo "\$(OBJDIR)/allp$dir$name\$(OBJ): \$(srcdir)/$file$includes"
    if test "$ext" = "c"; then
      echo "	\$(COMPILE_PROFILE) -c \$(srcdir)/$file -o \$(OBJDIR)/allp$dir$name\$(OBJ)"
    else
      echo "	\$(COMPILE_S_PROFILE) -c \$(srcdir)/$file -o \$(OBJDIR)/allp$dir$name\$(OBJ)"
    fi
    echo "\$(OBJDIR)/shared/allp$dir$name\$(OBJ): \$(srcdir)/$file$includes"
    if test "$ext" = "c"; then
      echo "	\$(COMPILE_PROFILE) \$(ALLEGRO_SHAREDLIB_CFLAGS) -c \$(srcdir)/$file -o \$(OBJDIR)/shared/allp$dir$name\$(OBJ)"
    else
      echo "	\$(COMPILE_S_PROFILE) -c \$(srcdir)/$file -o \$(OBJDIR)/shared/allp$dir$name\$(OBJ)"
    fi
    echo ""
  elif test "$file" != ..; then
    missing="$missing $file"
  fi
done

if test -n "$symbols"; then
  echo "# Headers referred by symbols:"
  echo "#$symbols"
  echo ""
fi

if test -n "$missing"; then
  echo "# Missing source files:"
  echo "#$missing"
  echo ""
fi
