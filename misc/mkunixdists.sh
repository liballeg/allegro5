#!/bin/sh

################################################################
# mkunixdists.sh -- shell script to generate Unix distributions
#
# Usage: mkunixdists.sh <archive>[.zip] [tmpdir]
#
# This generates all the Unix-specific distributions.  The
# existing ZIP format is fine on Unix too, but we'll generate
# here a .tar.gz in Unix format (no `fix.sh unix' necessary) and
# also an end-user distribution which just creates and installs
# the library, without examples, documentation, etc.  I suppose
# there's a danger that people will download this as a cut-down
# development version, but if we shoot those people then this
# problem will be solved.  This script might need a lot of disk
# space, but no more than the existing zipup.sh needs. :)



################################################################
# First process the arguments

if [ $# -lt 1 -o $# -gt 2 ]; then
	echo "Usage: mkunixdists.sh <archive>[.zip] [tmpdir]"
	exit 1
fi

# Sort out `dir', adding a trailing `/' if necessary
if [ $# -gt 1 ]; then
	dir=$(echo "$2" | sed -e 's/\([^/]\)$/\0\//').tmp
else
	dir=.tmp
fi



################################################################
# Error reporter

error() {
	echo "Error occured, aborting" ; rm -rf $dir ; exit 1
}


################################################################
# Unzip the archive and convert line endings

mkdir $dir || error

echo "Unzipping $1 to $dir"
	unzip -qq $1 -d $dir || error
echo "Running 'convert_line_endings.sh --dtou'"
	(cd $dir/allegro &&
		rm -f makefile &&
		sh misc/convert_line_endings.sh --dtou >/dev/null
	) || error
echo "Checking version number"
	# Derive version number from the archive name, as the number in
	# CMakeLists.txt won't reflect the fourth version digit.
	version=$( echo $1 | sed -e 's/^allegro-// ; s/\.zip$//' )
	test -n "$version" || error
	basename="allegro-$version"
	basename2="allegro-enduser-$version"
echo "Renaming 'allegro' to '$basename'"
	mv $dir/allegro $dir/$basename || error

################################################################
# Make .tar.gz distributions

mktargz() {
	echo "Creating $1.tar"
	(cd $dir && tar -cf - $basename) > $1.tar || error
	echo "gzipping to $1.tar.gz"
	gzip $1.tar || error
}

# Create the developers' archive
mktargz $basename

# Hack'n'slash
# XXX end-user distribution is useless while we are in WIP mode
# This code is outdated anyway.
if false
then
	echo "Stripping to form end-user distribution"
	(cd $dir/$basename && {
		(cd src && rm -rf beos qnx dos mac ppc win)
		(cd obj && rm -rf bcc32 beos qnx djgpp mingw32 msvc watcom)
		(cd lib && rm -rf bcc32 beos qnx djgpp mingw32 msvc watcom)
		(cd include && rm -f bealleg.h qnxalleg.h winalleg.h)
		(cd misc && rm -f cmplog.pl dllsyms.lst findtext.sh fixpatch.sh fixver.sh)
		(cd misc && rm -f allegro-config-qnx.sh zipup.sh zipwin.sh *.bat *.c)
		mkdir .saveme
		cp readme.txt docs/build/unix.txt docs/build/linux.txt .saveme
		rm -rf demo docs examples resource setup tests tools
		rm -f AUTHORS CHANGES THANKS *.txt fix* indent* readme.* allegro.mft
		rm -f makefile.all makefile.be makefile.qnx makefile.bcc makefile.dj
		rm -f makefile.mgw makefile.mpw makefile.vc makefile.wat makefile.tst
		rm -f xmake.sh
		rm -f keyboard.dat language.dat
		mv .saveme/* .
		rmdir .saveme
		{       # Tweak makefile.in
			cp makefile.in makefile.old &&
			cat makefile.old |
			sed -e "s/INSTALL_TARGETS = .*/INSTALL_TARGETS = mini-install/" |
			sed -e "s/DEFAULT_TARGETS = .*/DEFAULT_TARGETS = lib modules/" |
			cat > makefile.in &&
			rm -f makefile.old
		}
	})

	# Create the end users' archive
	mktargz $basename2
fi  # false


################################################################
# Create SRPM distribution
#
# We don't actually create the binary RPMs here, since that
# will really need to be done on many different machines.
# Instead we'll build the source RPM.
#
# This requires you to have Red Hat's default RPM build system
# properly set up, so we'll skip it if that's not the case.

# XXX SRPMs disabled because they must be broken by now
# Also, they are useless.
if false
then
	rpmdir=
	[ -d /usr/src/redhat ] && rpmdir=/usr/src/redhat
	[ -d /usr/src/packages ] && rpmdir=/usr/src/packages
	[ -d /usr/src/RPM ] && rpmdir=/usr/src/RPM
	[ -d /usr/src/rpm ] && rpmdir=/usr/src/rpm

	if [ -n "$rpmdir" ]; then
		echo "Creating SRPM"
		echo "Enter your root password if prompted"
		su -c "(\
			cp -f $basename.tar.gz $rpmdir/SOURCES ;\
			cp -f $dir/$basename/misc/icon.xpm $rpmdir/SOURCES ;\
			rpm -bs $dir/$basename/misc/allegro.spec ;\
			mv -f $rpmdir/SRPMS/allegro-*.rpm . ;\
			rm -f $rpmdir/SOURCES/icon.xpm ;\
			rm -f $rpmdir/SOURCES/$basename.tar.gz ;\
		)"
	fi
fi  # false


################################################################
# All done!

rm -rf $dir
echo "All done!"

