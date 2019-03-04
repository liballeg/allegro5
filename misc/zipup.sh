#! /bin/sh -e
#
#  Shell script to create a distribution zip, generating the manifest
#  file and optionally building diffs against a previous version. This
#  should be run from the allegro directory, but will leave the zip 
#  files in the parent directory.
#
#  This script tries to generate dependencies for all the supported
#  compilers, which involves considerable skulduggery to make sure it
#  will work on any Unix-like platform, even if the compilers are not
#  available themselves.
#
#  Note: if you write datestamp in the archive_name field, then the
#  resulting archive will be datestamped. This is in particular useful
#  for making SVN snapshots.

export LC_ALL=C

for tool in doxygen git cmake gcc unzip zip perl ; do
	which $tool >/dev/null 2>&1 || \
		{ echo "error: program $tool not found" ; exit 1; }
done

if [[ -z $1 ]]; then
    VERSION=$(grep '^set(ALLEGRO_VERSION ' CMakeLists.txt | tr -cd '[0-9.]')
else
    VERSION=$1
fi
name=allegro-$VERSION
tmpdir=/tmp/"$name"
currdir="$PWD"

git_branch=$(git branch 2>/dev/null| sed -n '/^\*/s/^\* //p')
rm -rf "$tmpdir"
git clone . "$tmpdir"
cd "$tmpdir"
git checkout "$git_branch"
rm -rf .git

# make sure all shell scripts are executable
find . -name '*.sh' -exec chmod +x {} ';'

# convert documentation from the ._tx source files
echo "Building artifacts and documentation..."
(
    set -e
    mkdir -p .dist/build
    cd .dist/build
    rm -rf docs
    cmake $tmpdir
    make -j2 docs dat
    umask 022
    for x in build txt ; do
        mkdir -p $tmpdir/docs/$x
    done
    for x in AUTHORS CHANGES THANKS readme.txt ; do
        cp -v docs/$x $TOP
    done
    cp -v docs/build/* $tmpdir/docs/build
    cp -v docs/txt/*.txt $tmpdir/docs/txt
) || exit 1


# generate docs for AllegroGL addon
echo "Generating AllegroGL docs..."
(cd addons/allegrogl/docs; doxygen >/dev/null)

# create language.dat and keyboard.dat files
export DAT="$tmpdir/.dist/build/tools/dat"
misc/mkdata.sh || exit 1
rm -rf "$tmpdir/.dist"

# generate the tar.gz first as the files are in Unix format
# This is missing the manifest file, but who cares?
(
cd ..
tar zcf "$name".tar.gz "$name"
tar cJf "$name".tar.xz "$name"
mv "$name".tar.gz "$currdir"
mv "$name".tar.xz "$currdir"
)

# now the DOS stuff...

proc_filelist()
{
   # common files.
   AL_FILELIST=`find . -type f "(" ! -path "*/.*" ")" -a "(" \
      -name "*.c" -o -name "*.cfg" -o -name "*.cpp" -o -name "*.def" -o \
      -name "*.h" -o -name "*.hin" -o -name "*.in" -o -name "*.inc" -o \
      -name "*.m" -o -name "*.m4" -o -name "*.mft" -o -name "*.s" -o \
      -name "*.rc" -o -name "*.rh" -o -name "*.spec" -o -name "*.pl" -o \
      -name "*.txt" -o -name "*._tx" -o -name "makefile*" -o \
      -name "*.inl" -o -name "configure" -o -name "CHANGES" -o \
      -name "AUTHORS" -o -name "THANKS" \
   ")"`

   # touch DOS batch files
  AL_FILELIST="$AL_FILELIST `find . -type f -name '*.bat'`"
}


# emulation of the djgpp utod utility program (Unix to DOS text format)
utod()
{
   for file in $*; do
      if echo $file | grep "^\.\.\./" >/dev/null; then
	 # files like .../*.c recurse into directories (emulating djgpp libc)
	 spec=$(echo $file | sed -e "s/^\.\.\.\///")
	 find . -type f -name "$spec" -exec perl -p -i -e 's/([^\r]|^)\n/\1\r\n/' {} \;
      else
	 perl -p -e "s/([^\r]|^)\n/\1\r\n/" $file > _tmpfile
	 touch -r $file _tmpfile
	 mv _tmpfile $file
      fi
   done
}


# build the main zip archive
echo "Creating $name.zip..."
mkdir -p .dist/allegro
# It is recommended to build releases from a clean checkout.
# However, for testing it may be easier to use an existing workspace
# so let's ignore some junk here.
# Note: we use -print0 and xargs -0 to handle file names with spaces.
find . -type f \
	'!' -path "*/.*" -a \
	'!' -path '*/B*' -a \
	'!' -iname '*.rej' -a \
	'!' -iname '*.orig' -a \
	'!' -iname '*~' -a \
	-print0 |
    xargs -0 cp -a --parents -t .dist/allegro

# from now on, the scripts runs inside .dist
cd .dist

# convert files to DOS format for .zip and .7z archives
proc_filelist
utod "$AL_FILELIST"


# if 7za is available, use that to produce both .zip and .7z files
if 7za > /dev/null ; then
   7za a -mx9 $name.zip allegro
   7za a -mx9 -ms=on $name.7z allegro
else
   zip -9  -r $name.zip allegro
fi


# generate the manifest file
echo "Generating allegro.mft..."
unzip -Z1 $name.zip | sort > allegro/allegro.mft
echo "allegro/allegro.mft" >> allegro/allegro.mft
utod allegro/allegro.mft
zip -9 $name.zip allegro/allegro.mft

mv $name.zip "$currdir"
cd "$currdir"

echo "Done! Check . for the output."
