#! /bin/bash
#
# Purpose: Build Allegro 5 for OSX and iOS.
#
# WARNING!
#   Make sure you have the most recent version of the build tools. Apple moved
#   installation of Xcode 4.3 from /Developer to /Applications. cmake has been
#   being patched ever since. This script was tested with:
#       * Xcode 4.5.2 and Macports cmake 2.8.10. *
#   You will have problems with cmake 2.8.9 and other versions of cmake.
#       changelog: http://public.kitware.com/Bug/changelog_page.php?version_id=100
#       download: http://cmake.org/cmake/resources/software.html

function print_usage {
cat <<"EOT"
Usage: apple_build [options]

Options:
    --new       Start a clean build.
    --config=CONFIG,-c=CONFIG
                Config to build: Debug,Release,RelWithDebInfo.
    --uni,-u    Create universal libraries from iPhone & sim after build.
    --dummy,-d  Dummy run, print options, nothing done.
    --egs       Include examples in Allegro build.
    --demos     Include demos in Allegro build.
    --help,-h   This help information.

Target:
    --osx,-o    OSX.
    --ios       iPhone & iPhone simulator.
    --iphone,-p iPhone.
    --isim,-s   iPhone simulator.

Example usage:
    - Build OSX only from scratch. Will delete previous build directory:
        apple_build.sh --new --osx

    - Rebuild iOS without creating new project files, e.g. after git pull:
        apple_build.sh --ios

    - Build and create iOS universal libraries:
        apple_build.sh --ios --uni

EOT
}

BUILD_OSX=0
BUILD_IPHONE=0
BUILD_IPHONESIM=0
BUILD_UNIVERSAL=0
MAKE_LIBS=0
NEW_BUILD=0
DUMMY_RUN=0

CONFIG=RelWithDebInfo
AL_EGS=0
AL_DEMOS=0

# Allegro5 library dependencies for iOS, e.g. Freetype and Vorbis.
# Dependencies copied into build directory so Allegro finds them in "deps".
DEPS_IPHONE=./deps_iphone

# Version of SDK you have and the lowest you'd like to support.
# If, when you run an executable you get "Illegal instruction: 4" it is because
# your exe is linked against an SDK version higher than your OSX version. To fix
# this you need to lower the deployment version.
AL_OSX_DEPLOYMENT="10.7"
AL_IOS_DEPLOYMENT="5.1"

# Parse the command-line arguments:
for opt in $*
do
    case $opt in
        --new)
        NEW_BUILD=1
        echo Reseting and starting new build...
        ;;

        -o|--osx)
        BUILD_OSX=1
        echo Building OSX.
        ;;

        -p|--iphone)
        BUILD_IPHONE=1
        echo Building iPhone.
        ;;

        -s|--isim)
        BUILD_IPHONESIM=1
        echo Building iPhone simulator.
        ;;

        --ios)
        BUILD_IPHONE=1
        BUILD_IPHONESIM=1
        MAKE_LIBS=1
        echo Building iOS: iphone and simulator.
        ;;

        -u|--uni)
        BUILD_UNIVERSAL=1
        echo Will create universal binaries once built.
        ;;

        -c=*|--config=*)
        CONFIG=`echo $opt | sed 's/[-a-zA-Z0-9]*=//'`
        ;;

        --egs)
        AL_EGS=1
        ;;

        --demos)
        AL_DEMOS=1
        ;;

        -d|--dummy)
        DUMMY_RUN=1
        ;;

        -h|--help)
        print_usage
        exit 0
        ;;

        *)
        echo error: Unrecognised option \"$opt\"
        echo
        print_usage
        exit 1
        ;;
    esac
done

AL_CMAKE_OPTIONS="\
    -D CMAKE_BUILD_TYPE=$CONFIG -D SHARED=0 -D WANT_FRAMEWORKS=0 \
    -D WANT_EXAMPLES=$AL_EGS -D WANT_DEMO=$AL_DEMOS \
    -D WANT_FLAC=0 -D WANT_VORBIS=0 -D WANT_TREMOR=0"

echo "Building config: $CONFIG"
echo "Allegro5 options: $AL_CMAKE_OPTIONS"

######################################################################

# We want to use LLVM, not GCC (from Allegro5 Mac readme).
# The default for Xcode 4.5 is clang 4.1.
export CC=clang

# Cmake has lots of issues with generating Xcode projects.
#   - In cmake 2.8.10, can't set a deployment target because of broken
#     logic in Darwain.cmake.
#   - Macport version lags client at cmake.org.
#CMAKE=/opt/local/bin/cmake
CMAKE="/Applications/CMake 2.8-10.app/Contents/bin/cmake"

if [ ! -x "$CMAKE" ]; then
    echo "Please install cmake from cmake.org. We need the latest version."
    echo "URL: http://cmake.org/cmake/resources/software.html"
    exit 1
fi
echo Using cmake @ "$CMAKE"

if [ -n "${SDKROOT}" ]; then
    echo "Warning: Your SDKROOT environment setting will override script."
fi

if [ $DUMMY_RUN -eq 1 ]; then
    echo "Dummy run."
    exit 0
fi

function reset_dir_tree {
    if [ -d $1 ]; then
        rm -r $1
    fi
    mkdir $1
}

######################################################################
# OSX

if [ $BUILD_OSX -eq 1 ]; then
    BUILD_DIR=build_osx
    INSTALL_DIR=`pwd`/install_osx

    reset_dir_tree $INSTALL_DIR

    if [ $NEW_BUILD -eq 1 ]; then
        reset_dir_tree $BUILD_DIR
        cd $BUILD_DIR

        echo Generate Allegro Xcode OSX project
        "$CMAKE" $AL_CMAKE_OPTIONS -D CMAKE_INSTALL_PREFIX=$INSTALL_DIR \
            -D CMAKE_OSX_DEPLOYMENT_TARGET=$AL_OSX_DEPLOYMENT \
            -G Xcode ..
        cd ..
    fi

    cd $BUILD_DIR
	xcodebuild -project ALLEGRO.xcodeproj -target ALL_BUILD \
		-arch x86_64 -sdk macosx -configuration $CONFIG || exit 1

    "$CMAKE" -P cmake_install.cmake
	cd ..
fi

######################################################################
# iOS: phone & sim
#   - Keep iphone and sim directories separate to avoid problems.
#     We could build in same build_XXX dir if all target paths separate.

IOS_TOOLCHAIN="-D CMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-iphone.cmake"

#if [ $BUILD_IPHONESIM -o $BUILD_IPHONE ]; then
if [ $BUILD_IPHONE -eq 1 ]; then
    if [ ! -d $DEPS ]; then
        echo "You have no \"$DEPS\" directory."
        exit 1
    fi
fi

# Generate Xcode project for iOS.
# args: (name, install_dir, cmake_options)
function cmake_ios {
    echo Generate Allegro Xcode $1 project;
    "$CMAKE" $IOS_TOOLCHAIN $AL_CMAKE_OPTIONS -D CMAKE_INSTALL_PREFIX=$2 $3 \
        -G Xcode .. ;
}

if [ $BUILD_IPHONESIM -eq 1 ]; then
    BUILD_DIR=build_isim
    INSTALL_DIR=`pwd`/install_isim

    reset_dir_tree $INSTALL_DIR

    if [ $NEW_BUILD -eq 1 ]; then
        reset_dir_tree $BUILD_DIR
        cd $BUILD_DIR

        cmake_ios "iphone simulator" $INSTALL_DIR

        echo Modify Xcode project to target iOS simulator
        "$CMAKE" \
            -D CMAKE_OSX_SYSROOT=iphoneos \
            -D CMAKE_OSX_DEPLOYMENT_TARGET="" \
            -D CMAKE_OSX_ARCHITECTURES="i386" .

        cd ..
    fi

	cd $BUILD_DIR
	xcodebuild -project ALLEGRO.xcodeproj -target ALL_BUILD \
		-arch i386 -sdk iphonesimulator -configuration $CONFIG || exit 1

    "$CMAKE" -P cmake_install.cmake
	cd ..
fi

if [ $BUILD_IPHONE -eq 1 ]; then
    BUILD_DIR=build_iphone
    INSTALL_DIR=`pwd`/install_iphone

    reset_dir_tree $INSTALL_DIR

    # Nuke the build dir and start again?
    if [ $NEW_BUILD -eq 1 ]; then
        reset_dir_tree $BUILD_DIR

        # If we have iphone dependencies, copy and rename them.
        if [ -d $DEPS_IPHONE ]; then
            cp -r $DEPS_IPHONE $BUILD_DIR/deps
        fi

        cd $BUILD_DIR

        cmake_ios "iphone" $INSTALL_DIR

        echo Modify Xcode project to target iOS simulator
        $CMAKE \
            -D CMAKE_OSX_SYSROOT=iphoneos \
            -D CMAKE_OSX_DEPLOYMENT_TARGET="" \
            -D CMAKE_OSX_ARCHITECTURES="armv7" .
        cd ..
    fi

	cd $BUILD_DIR
	xcodebuild -project ALLEGRO.xcodeproj -target install \
		-arch armv7 -sdk iphoneos -configuration $CONFIG || exit 1

    "$CMAKE" -P cmake_install.cmake
	cd ..
fi

######################################################################
# Create universal binaries

if [ $BUILD_UNIVERSAL -eq 1 ]; then

echo Creating iOS universal libraries.

# Source directories: iphone arm and iphone sim i386 libs built against iOS SDK.
LIBS_SIM=install_isim/lib
LIBS_IOS=install_iphone/lib
HEADERS=$LIBS_IOS/../include

# Where we'll install the combined iphone/iphonesim universal library.
IOS_INSTALL_DIR=./install_ios

# Nuke target directory
reset_dir_tree $IOS_INSTALL_DIR

# Copy our dependencies. Creates include & lib.
cp -r $DEPS_IPHONE/* $IOS_INSTALL_DIR

# Copy Allegro headers.
cp -r $HEADERS/* $IOS_INSTALL_DIR/include

function make_universal {
    lipo -output $IOS_INSTALL_DIR/lib/$1 -create $LIBS_SIM/$1 $LIBS_IOS/$1;
    echo Created $1;
    lipo -detailed_info $IOS_INSTALL_DIR/lib/$1;
}

# Main library
make_universal "liballegro-static.a"

AL_LIBS="
acodec
audio
color
font
image
main
memfile
dialog
physfs
primitives
shader
ttf
video
"

# Addons
for lib in $AL_LIBS
do
    make_universal liballegro_$lib-static.a
done

fi
