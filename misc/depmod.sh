#! /bin/sh

# Usage: depmod.sh module_name libraries sources ...

# Generate dependencies and rules for building dynamically loaded modules
# under Unices.


modname=$1
modlibs=$2
shift; shift

sources=$*
objects=`echo $sources | sed 's,[^	 ]*/,,g' | sed 's,\.[^.	 ]*,,g'`

MODNAME=`echo $modname | tr [a-z] [A-Z]`

module="alleg-${modname}-\$(shared_version).so"
objlist="MODULE_${MODNAME}_OBJECTS"


# object list
prev="$objlist ="
for file in .. $objects; do
  if test "$file" != ..; then
    echo "$prev \\"
    prev="  \$(OBJDIR)/module/$file\$(OBJ)"
  fi
done
echo "$prev"
echo ""

# module rule
echo "\$(LIBDIR)/$module: \$(${objlist})"
echo "	gcc -shared -o \$@ \$(${objlist}) \$(LDFLAGS) $modlibs"
echo ""

# explicit object rules (pass -DALLEGRO_MODULE)
for file in .. $sources; do
  if test "$file" != ..; then
    name=`echo $file | sed 's,^.*/,,;s,\.[^.]*$,,'`
    ext=`echo $file | sed 's,^.*\.,,'`
    echo "\$(OBJDIR)/module/$name\$(OBJ): \$(srcdir)/$file"
    if test "$ext" = "c"; then
      echo "	\$(COMPILE_NORMAL) -DALLEGRO_MODULE -c \$(srcdir)/$file -o \$(OBJDIR)/module/$name\$(OBJ)"
    else
      echo "	\$(COMPILE_S_NORMAL) -DALLEGRO_MODULE -c \$(srcdir)/$file -o \$(OBJDIR)/module/$name\$(OBJ)"
    fi
  fi
done
echo ""
echo ""
