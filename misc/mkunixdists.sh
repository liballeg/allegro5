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
	echo "Error occured, aborting" ; exit 1
}


################################################################
# Unzip the archive and run fix.sh unix

mkdir $dir || error

echo "Unzipping $1 to $dir"
	unzip -qq $1 -d $dir || error
echo "Running 'fix.sh unix'"
	(cd $dir/allegro && . fix.sh unix >/dev/null) || error
echo "Running autoconf"
	(cd $dir/allegro && autoconf >/dev/null) || error
echo "Checking version number"
	basename=$(sed -n -e 's/shared_version = /allegro-/p' $dir/allegro/makefile.ver)
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
echo "Stripping to form end-user distribution"
(cd $dir/$basename && {
	(cd src && rm -rf beos qnx dos mac ppc win)
	(cd obj && rm -rf bcc32 beos qnx djgpp mingw32 msvc rsxnt watcom)
	(cd lib && rm -rf bcc32 beos qnx djgpp mingw32 msvc rsxnt watcom)
	(cd include && rm -f bealleg.h qnxalleg.h macalleg.h winalleg.h)
	(cd misc && rm -f cmplog.pl findtext.sh fixpatch.sh fixver.sh zipup.sh)
	rm -rf demo docs examples setup tests tools
	rm -f AUTHORS CHANGES THANKS *.txt fix* indent* readme.* allegro.mft
	rm -f makefile.all makefile.be makefile.qnx makefile.bcc makefile.dj
	rm -f makefile.mgw makefile.mpw makefile.rsx makefile.vc makefile.wat
	{       # Tweak makefile.in
		cp makefile.in makefile.old &&
		cat makefile.old |
		sed -e "s/INSTALL_TARGETS = .*/INSTALL_TARGETS = mini-install/" |
		sed -e "s/DEFAULT_TARGETS = .*/DEFAULT_TARGETS = lib/" |
		cat > makefile.in &&
		rm -f makefile.old
	}
})

# Create the end users' archive
mktargz $basename-enduser


################################################################
# Create RPM distribution(s)
#
# I'm a little hazy on this. :)  These are binary RPMs which
# actually contain source code that gets built, installed and
# deleted when you install the RPM.  I hope...
#
# This requires you to have Red Hat's default RPM build system
# properly set up, so we'll skip it if that's not the case.

mkrpm() {
	echo "Creating $1 RPM from $2.tar.gz"
	echo "Enter your root password if prompted"
	su -c "(\
		rm -f /usr/src/redhat/RPMS/i386/$1-*.rpm ;\
		cp -f $2.tar.gz /usr/src/redhat/SOURCES/$2.tar.gz ;\
		rpm -bb $dir/$basename/misc/$1.spec ;\
		mv -f /usr/src/redhat/RPMS/i386/$1-*.rpm . ;\
		rm -f /usr/src/redhat/SOURCES/$1.tar.gz ;\
	)"
}

if [ -d /usr/src/redhat ]; then
	mkrpm allegro $basename
	mkrpm allegro-enduser $basename-enduser
fi


################################################################
# All done!

rm -rf $dir
echo "All done!"

