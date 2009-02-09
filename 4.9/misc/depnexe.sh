#! /bin/sh

# Generate dependencies and rules for building simple programs
# from one source file.

missing=
symbols=
for file in .. $*; do
  if test -f $file; then
    dir=`echo $file | sed 's,/[^/]*$,,'`
    name=`echo $file | sed 's,^.*/,,' | sed 's,\.[^.]*$,,'`
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
	      include=`echo $include | sed 's,[-./],_,g'`
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

    # Program.
    echo "$dir/$name\$(EXE): \$(OBJDIR)/$name\$(OBJ)"
    echo "	\$(LINK) -o $dir/$name\$(EXE) \$(OBJDIR)/$name\$(OBJ)"
    echo ""

    # Object file.
    echo "\$(OBJDIR)/$name\$(OBJ): \$(srcdir)/$file$includes"
    if test "$ext" = "c"; then
      echo "	\$(COMPILE_PROGRAM) -c \$(srcdir)/$file -o \$(OBJDIR)/$name\$(OBJ)"
    else
      echo "	\$(COMPILE_S_PROGRAM) -c \$(srcdir)/$file -o \$(OBJDIR)/$name\$(OBJ)"
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
