#! /bin/sh

# Usage: depmod.sh module_name libraries sources ...

# Generate dependencies and rules for building dynamically loaded modules
# under Unices.


modname=$1
modlibs=$2
shift; shift

sources=$*
objects=`echo $sources | sed 's,[^	 ]*/,,g;s,\.[^.	 ]*,,g'`

MODNAME=`echo $modname | tr a-z A-Z`

module="alleg-${modname}.so"
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
if test "$modlibs" = "--"; then
   echo "	\$(CC) -shared \$(ALLEGRO_SHAREDLIB_CFLAGS) -o \$@ \$(${objlist}) \$(LDFLAGS)"
else
   echo "	\$(CC) -shared \$(ALLEGRO_SHAREDLIB_CFLAGS) -o \$@ \$(${objlist}) \$(LDFLAGS) $modlibs"
fi
echo ""

# explicit object rules (pass -DALLEGRO_MODULE)
for file in .. $sources; do
  if test "$file" != ..; then
    name=`echo $file | sed 's,^.*/,,;s,\.[^.]*$,,'`
    ext=`echo $file | sed 's,^.*\.,,'`
    echo "\$(OBJDIR)/module/$name\$(OBJ): \$(srcdir)/$file \$(obj_unix_asmdef_inc)"
    if test "$ext" = "c"; then
      echo "	\$(COMPILE_NORMAL) \$(ALLEGRO_SHAREDLIB_CFLAGS) -DALLEGRO_MODULE -c \$(srcdir)/$file -o \$(OBJDIR)/module/$name\$(OBJ)"
    else
      echo "	\$(COMPILE_S_NORMAL) \$(ALLEGRO_SHAREDLIB_CFLAGS) -DALLEGRO_MODULE -c \$(srcdir)/$file -o \$(OBJDIR)/module/$name\$(OBJ)"
    fi
  fi
done
echo ""
echo ""
