#! /bin/sh

# Generate dependencies and rules for building the datafile plugins library.

plugin_files="$* tools/plugins/*.c"
objects=`echo $plugin_files | sed 's,[^	 ]*/,,g;s,\.[^.	 ]*,,g'`

# Normal library.
prev="LIBALDAT_OBJECTS ="
for file in .. $objects; do
  if test "$file" != ..; then
    echo "$prev \\"
    prev="  \$(OBJDIR)/alleg/$file\$(OBJ)"
  fi
done
echo "$prev"
echo ""
echo "\$(LIBDIR)/libaldat.a: \$(LIBALDAT_OBJECTS)"
echo "	rm -f \$(LIBDIR)/libaldat.a"
echo "	\$(AR) \$(LIBDIR)/libaldat.a \$(LIBALDAT_OBJECTS)"
echo ""

# Debugging library.
prev="LIBADDAT_OBJECTS ="
for file in .. $objects; do
  if test "$file" != ..; then
    echo "$prev \\"
    prev="  \$(OBJDIR)/alld/$file\$(OBJ)"
  fi
done
echo "$prev"
echo ""
echo "\$(LIBDIR)/libaddat.a: \$(LIBADDAT_OBJECTS)"
echo "	rm -f \$(LIBDIR)/libaddat.a"
echo "	\$(AR) \$(LIBDIR)/libaddat.a \$(LIBADDAT_OBJECTS)"
echo ""

# Profiling library.
prev="LIBAPDAT_OBJECTS ="
for file in .. $objects; do
  if test "$file" != ..; then
    echo "$prev \\"
    prev="  \$(OBJDIR)/allp/$file\$(OBJ)"
  fi
done
echo "$prev"
echo ""
echo "\$(LIBDIR)/libapdat.a: \$(LIBAPDAT_OBJECTS)"
echo "	rm -f \$(LIBDIR)/libapdat.a"
echo "	\$(AR) \$(LIBDIR)/libapdat.a \$(LIBAPDAT_OBJECTS)"
echo ""

missing=
symbols=
for file in .. $plugin_files; do
  if test -f "$file"; then
    dir=`echo $file | sed 's,/[^/]*$,,'`
    name=`echo $file | sed 's,^.*/,,;s,\.[^.]*$,,'`
    ext=`echo $file | sed 's,^.*\.,,'`
    includes=
    deps="$file"
    while test -n "$deps"; do
      newdeps=
      for dep in $deps; do
	includes1=`grep '^[ 	]*#[ 	]*include[ 	]*[a-zA-Z0-9_][a-zA-Z0-9_]*' $dep | \
	  sed 's,^[ 	]*#[ 	]*include[ 	]*\([a-zA-Z0-9_]*\),\1,'`
	includes2=`grep '^[ 	]*#[ 	]*include[ 	]*".*"' $dep | \
	  sed 's,^[ 	]*#[ 	]*include[ 	]*"\(.*\)",\1,'`
	if test -n "$includes1"; then
	  for include in $includes1; do
	    includes="$includes \$($include)"
	    case "$symbols" in
	    *$include* )
	      ;;
	    * )
	      symbols="$symbols $include"
	      ;;
	    esac
	  done
	fi
	if test -n "$includes2"; then
	  for include in $includes2; do
	    if test -f "$dir/$include"; then
	      includes="$includes \$(srcdir)/$dir/$include"
	      newdeps="$newdeps $dir/$include"
	    else
	      include=`echo $include | sed 's,[-./:],_,g'`
	      includes="$includes \$($include)"
	      case "$symbols" in
	      *$include* )
		;;
	      * )
		symbols="$symbols $include"
		;;
	      esac
	    fi
	  done
	fi
      done
      deps="$newdeps"
    done

    # Normal library.
    echo "\$(OBJDIR)/alleg/$name\$(OBJ): \$(srcdir)/$file$includes"
    if test "$ext" = "c"; then
      echo "	\$(COMPILE_NORMAL) -c \$(srcdir)/$file -o \$(OBJDIR)/alleg/$name\$(OBJ)"
    else
      echo "	\$(COMPILE_S_NORMAL) -c \$(srcdir)/$file -o \$(OBJDIR)/alleg/$name\$(OBJ)"
    fi
    echo ""

    # Debugging library.
    echo "\$(OBJDIR)/alld/$name\$(OBJ): \$(srcdir)/$file$includes"
    if test "$ext" = "c"; then
      echo "	\$(COMPILE_DEBUG) -c \$(srcdir)/$file -o \$(OBJDIR)/alld/$name\$(OBJ)"
    else
      echo "	\$(COMPILE_S_DEBUG) -c \$(srcdir)/$file -o \$(OBJDIR)/alld/$name\$(OBJ)"
    fi
    echo ""

    # Profiling library.
    echo "\$(OBJDIR)/allp/$name\$(OBJ): \$(srcdir)/$file$includes"
    if test "$ext" = "c"; then
      echo "	\$(COMPILE_PROFILE) -c \$(srcdir)/$file -o \$(OBJDIR)/allp/$name\$(OBJ)"
    else
      echo "	\$(COMPILE_S_PROFILE) -c \$(srcdir)/$file -o \$(OBJDIR)/allp/$name\$(OBJ)"
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

# Special rule to create the plugins.h file
inc_files=`echo tools/plugins/*.inc | sed 's,tools/plugins/,$(srcdir)/tools/plugins/,g'`
echo "\$(OBJDIR)/plugins.h: $inc_files"
echo "	cat $inc_files > \$(OBJDIR)/plugins.h"
echo ""

