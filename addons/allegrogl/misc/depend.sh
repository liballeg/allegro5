#!/bin/sh

# Arg 1 : $(INCLUDE_DIR) directory
# Arg 2 : $(SRC_DIR) directory
# Arg 3 : $(EXAMPLE_DIR) directory

# Generates dependencies automatically

# 1. Dependencies of library sources
sources=`ls $2 | grep '\.c'`
sources1=`ls $2/amesa | grep '\.c'`
for file in $sources1; do
	sources="$sources amesa/$file"
done
for file in $sources; do
        src="$2/$file"
    	name=`echo $file | sed 's,^.*/,,;s,\.[^.]*$,,'`
        includes1=`grep '^[ 	]*#[ 	]*include[ 	]*[a-zA-Z0-9_][a-zA-Z0-9_]*' $src | \
          sed 's,^[ 	]*#[ 	]*include[ 	]*\([a-zA-Z0-9_]*\),\1,'`
        includes2=`grep '^[ 	]*#[ 	]*include[ 	]*".*"' $src | \
          sed 's,^[ 	]*#[ 	]*include[ 	]*"\(.*\)",\1,'`
	headers=
	if test -n "$includes1"; then
		for header in $includes1; do
			if test -f "$1/$header"; then
				headers="$headers $1/$header"
			fi
			if test -f "$2/$header"; then
				headers="$headers \$(SRC_DIR)/$header"
			fi
			if test -f "$2/amesa/$header"; then
				headers="$headers \$(SRC_DIR)/amesa/$header"
			fi
		done
	fi
	if test -n "$includes2"; then
		for header in $includes2; do
			if test -f "$1/$header"; then
				headers="$headers $1/$header"
			fi
			if test -f "$2/$header"; then
				headers="$headers \$(SRC_DIR)/$header"
			fi
			if test -f "$2/amesa/$header"; then
				headers="$headers \$(SRC_DIR)/amesa/$header"
			fi
		done
	fi
	echo "\$(OBJ_DIR)/$name.o: \$(SRC_DIR)/$file $headers"
done

# 2. Dependencies of examples
sources=`ls $3 | grep '\.c'`
for file in $sources; do
        src="$3/$file"
    	name=`echo $file | sed 's,^.*/,,;s,\.[^.]*$,,'`
        includes1=`grep '^[ 	]*#[ 	]*include[ 	]*[a-zA-Z0-9_][a-zA-Z0-9_]*' $src | \
          sed 's,^[ 	]*#[ 	]*include[ 	]*\([a-zA-Z0-9_]*\),\1,'`
        includes2=`grep '^[ 	]*#[ 	]*include[ 	]*".*"' $src | \
          sed 's,^[ 	]*#[ 	]*include[ 	]*"\(.*\)",\1,'`
	headers=
	if test -n "$includes1"; then
		for header in $includes1; do
			if test -f "$1/$header"; then
				headers="$headers $1/$header"
			fi
		done
	fi
	if test -n "$includes2"; then
		for header in $includes2; do
			if test -f "$1/$header"; then
				headers="$headers $1/$header"
			fi
		done
	fi
	echo "\$(EXAMPLE_DIR)/$name: \$(EXAMPLE_DIR)/$file $headers \$(LIB_PATH_U)"
done
