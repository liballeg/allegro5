#!/bin/sh
#
#	Shell script to create a MacOS X package of the library.
#	The package will hold an end-user version of Allegro,
#	installing just the framework and Project Builder template.
#
#	Thanks to macosxhints.com for the hint!
#
#	Usage: mkpkg <allegro_dir>
#
#	Will create a package in the current directory; allegro_dir
#	must be a valid Allegro directory structure, holding the
#	previously compiled MacOS X dynamic library.
#

if [ ! $# -eq 1 ]; then
        echo "Usage: mkpkg.sh <allegro_dir>"
        exit 1
fi


echo "Checking version number"
	version=$(sed -n -e 's/shared_version = //p' $1/makefile.ver)

libname=$1/lib/macosx/liballeg-${version}.dylib

if [ ! -f $libname ]; then
	echo "Cannot find valid dynamic library archive"
	exit 1
fi


if [ -d dstroot ]; then
	sudo rm -fr dstroot
fi


echo "Setting up package structure"


#############################################
# Prepare structure for the Allegro framework
#############################################

framework=dstroot/Library/Frameworks/Allegro.framework
mkdir -p $framework
mkdir -p ${framework}/Versions/${version}/Headers
mkdir -p ${framework}/Versions/${version}/Resources
cp $libname ${framework}/Versions/${version}/allegro
cp $1/include/allegro.h ${framework}/Versions/${version}/Headers
cp $1/include/osxalleg.h ${framework}/Versions/${version}/Headers
cp -r $1/include/allegro ${framework}/Versions/${version}/Headers
(cd $framework && {
	(cd Versions; ln -s $version Current)
	ln -s Versions/Current/Headers Headers
	ln -s Versions/Current/Resources Resources
	ln -s Versions/Current/allegro allegro
})
sed -e "s/@NAME@/allegro/" $1/misc/Info.plist >temp
sed -e "s/@VERSION@/${version}/" temp >${framework}/Resources/Info.plist
rm -f temp


########################################################
# Prepare structure for the Allegro application template
########################################################

template="dstroot/Developer/ProjectBuilder Extras/Project Templates/Application/Allegro Application"
mkdir -p "$template"
mkdir -p "${template}/AllegroApp.pbproj"
cp $1/misc/template.c "${template}/main.c"
cp $1/misc/TemplateInfo.plist "${template}/AllegroApp.pbproj"
cp $1/misc/project.pbxproj "${template}/AllegroApp.pbproj"
cp $1/misc/project.pbxuser "${template}/AllegroApp.pbproj"


##########################
# Create package info file
##########################

infofile=allegro-enduser-${version}.info
echo "Title Allegro" > $infofile
echo "Version $version" >> $infofile
echo "Description A multiplatform game programming library" >> $infofile
echo "DefaultLocation /" >> $infofile
echo "DeleteWarning" >> $infofile
echo "NeedsAuthorization YES" >> $infofile
echo "Required NO" >> $infofile
echo "Relocatable NO" >> $infofile
echo "RequiresReboot NO" >> $infofile
echo "UseUserMask YES" >> $infofile
echo "OverwritePermissions NO" >> $infofile
echo "InstallFat NO" >> $infofile
echo "RootVolumeOnly YES" >> $infofile
echo "Application NO" >> $infofile
echo "InstallOnly NO" >> $infofile
echo "DisableStop NO" >> $infofile
echo "LongFilenames YES" >> $infofile


##################
# Make the package
##################

packagefile=allegro-enduser-${version}.pkg
if [ -d $packagefile ]; then sudo rm -fr $packagefile; fi
find dstroot -name .DS_Store -delete
sudo chown -R root:staff dstroot
package dstroot $infofile -d . -ignoreDSStore
rm -f $infofile 1
gcc3 -o _makedoc $1/docs/src/makedoc/*.c
./_makedoc -rtf ${packagefile}/Contents/Resources/ReadMe.rtf $1/misc/pkgreadme._tx

echo "Writing post installation script"
postflight=${packagefile}/Contents/Resources/postflight
sudo echo "#!/bin/sh" > $postflight
sudo echo "mkdir -p /usr/local/lib" >> $postflight
sudo echo "if [ -f /usr/local/lib/liballeg-${version}.dylib ]; then" >> $postflight
sudo echo "   rm -f /usr/local/lib/liballeg-${version}.dylib" >> $postflight
sudo echo "fi" >> $postflight
sudo echo "ln -s /Library/Frameworks/Allegro.framework/Versions/${version}/Allegro /usr/local/lib/liballeg-${version}.dylib" >> $postflight
sudo chmod a+x $postflight
sudo chown -R root:staff ${packagefile}/Contents/Resources


######################
# Build the disk image
######################

echo "Creating compressed disk image"
volume=allegro-enduser-${version}
diskimage=${volume}.dmg
tempimage=temp.dmg
rm -f $diskimage $tempimage
hdiutil create $tempimage -megabytes 4 -layout NONE
drive=`hdid -nomount $tempimage`
newfs_hfs -v "$volume" -b 4096 /dev/r${drive:t}
hdiutil eject ${drive}
hdid $tempimage
mountpoint=`df -l | grep $drive | awk '{print $6}'`

cp -r $packagefile $mountpoint
./_makedoc -ascii ${mountpoint}/readme.txt $1/misc/pkgreadme._tx
./_makedoc -ascii ${mountpoint}/CHANGES $1/docs/src/changes._tx
./_makedoc -part -ascii ${mountpoint}/AUTHORS $1/docs/src/thanks._tx
./_makedoc -part -ascii ${mountpoint}/THANKS $1/docs/src/thanks._tx

hdiutil eject $drive
hdiutil convert -format UDCO $tempimage -o $diskimage
gzip -f -9 $diskimage
sudo rm -fr $tempimage ${packagefile} dstroot _makedoc

echo "Done!"
