# Scons build scripts for Allegro by Jon Rafkind
# Circa 12/25/2005

# Possible targets should be
# shared - Shared allegro library. Either a .so( unix ) or .dll( windows )
# static - Static allegro library, .a file.
# debug-shared - Shared library with debugging turned on
# debug-static - Static library with debugging turned on
# examples - All the examples in examples/
# tools - All the tools in tools/
# docs - All the documentation
# tests - All the tests in tests/
# demo - The demo game

# debug=1 is the same as prefixing debug- to the target and
# static=1 is the same as using the static target
# Thus debug=1 static=1 is the same as debug-static

# TODO:
# Compile the modules in unix( alsa, oss, etc ) - Mostly done
# Get the build system working on other platforms supported by Allegro, Windows being the most important one
# Allow arbitrary libraries to be dropped into the Allegro directory and automatically compiled using the Allegro SCons environments 

import os
import sys

import SCons

def allegroHelp():
    return """
scons [options] [targets]

Possible options:
static=1|0 : If static=1 is supplied a static Allegro library will be built and all extra programs will link using this library
debug=1|0 : If debug=1 is supplied a debug Allegro library will be built and all extra programs will link using this library
E.g:
Build liballeg.a and link with this
$ scons static=1
Build liballeg.a in debug mode and link with this
$ scons debug=1 static=1
Build liballeg.so in debug mode and link with this
$ scons debug=1

Possible targets are:
debug-static : Build a static library with debug mode on
debug-shared : Build a shared library with debug mode on
static : Build a static library with debug off
shared : Build a shared library with debug off
library : Build the library which is configured by static=X and debug=X
examples : Build all the examples
docs : Build the docs
demo : Build the demo
    """

Help(allegroHelp())

# Generate build directory (since we put the signatures db there)
try: os.mkdir("build")
except OSError: pass

allegroVersion = '4.3.0'

def getPlatform():
    return sys.platform

# Do not change directories when reading other scons files via SConscript
SConscriptChdir(0)

librarySource = []

# Returns a function that takes a directory and a list of files
# and returns a new list of files with a build directory prepended to it
def sourceFiles(dir, files):
    return map(lambda x: dir + '/' + x, files)

def appendDir(dir, files):
    return map(lambda x: dir + '/' + x, files)

def addFiles(dir,fileList):
    librarySource.extend(sourceFiles(dir,fileList))

# Subsequent scons files can call addExtra to add arbitrary targets
# that will be evaluated in this file
extras = []
def addExtra(func):
    extras.append(func)

# Returns a tuple( env, files, dir ). Each platform that Allegro supports should be
# listed here and the proper scons/X.scons file should be SConscript()'ed.
# env - environment
# files - list of files that compose the Allegro library
# dir - directory where the library( dll, so ) should end up
def getLibraryVariables():
    if getPlatform() == "openbsd3":
        return tuple([SConscript('scons/bsd.scons', exports = [ 'addFiles', 'addExtra' ]),"lib/unix/"])
    if getPlatform() == "linux2":
        return tuple([SConscript('scons/linux.scons', exports = [ 'addFiles', 'addExtra' ]),"lib/unix/"])
    if getPlatform() == "win32":
        return tuple([SConscript('scons/win32.scons', exports = [ 'addFiles', 'addExtra' ]),"lib/win32/" ])
    if getPlatform() == "darwin":
        return tuple([SConscript('scons/osx.scons', exports = [ 'addFiles', 'addExtra' ]),"lib/macosx/" ])

env, libDir = getLibraryVariables()

# Stop cluttering everything with .sconsign files, use a single db file instead
env.SConsignFile("build/signatures")

debugBuildDir = 'build/debug/'
optimizedBuildDir = 'build/release/'

def getLibraryName(debug):
    if debug:
        return 'allegd-' + allegroVersion
    else:
        return 'alleg-' + allegroVersion

def getAllegroTarget(debug,static):
    def build(function,lib,dir):
        env.BuildDir(dir, 'src', duplicate = 0)
        return function(lib, appendDir(dir, librarySource))

    def buildStatic(env,debug,dir):
        return build(env.StaticLibrary,libDir + '/static/' + getLibraryName(debug),dir)

    def buildShared(env,debug,dir):
        return build(env.SharedLibrary,libDir + '/shared/' + getLibraryName(debug),dir)

    debugEnv = env.Copy()
    debugEnv.Append(CCFLAGS = '-DDEBUGMODE=1')

    debugStatic = buildStatic(debugEnv, 1, debugBuildDir)
    debugShared = buildShared(debugEnv, 1, debugBuildDir)
    normalStatic = buildStatic(env, 0, optimizedBuildDir)
    normalShared = buildShared(env, 0, optimizedBuildDir)

    Alias('debug-static',debugStatic)
    Alias('debug-shared',debugShared)
    Alias('static',normalStatic)
    Alias('shared',normalShared)

    if debug == 1 and static == 1:
        return debugStatic
    elif debug == 1:
        return debugShared
    elif static == 1:
        return normalStatic
    else:
        return normalShared
        
debug = int(ARGUMENTS.get('debug',0))
static = int(ARGUMENTS.get('static',0))

if debug:
    normalBuildDir = debugBuildDir
else:
    normalBuildDir = optimizedBuildDir

env.Append(CPPPATH = [ normalBuildDir ])

library = getAllegroTarget(debug,static)
Alias('library', library)
Install(libDir, library)

docs = SConscript("scons/docs.scons", exports = ["normalBuildDir"])
Alias('docs', docs)

def buildDemo(env,appendDir,buildDir,libDir):
    env.BuildDir(buildDir + 'demo', 'demo', duplicate = 0)
    files = Split("""
        animsel.c
        aster.c
        bullet.c
        demo.c
        demodisp.c
        dirty.c
        expl.c
        game.c
        star.c
        title.c
    """);
    demo = env.Program('demo/demo', appendDir(buildDir + '/demo/', files))
    Alias('demo', demo)
    return demo

addExtra(buildDemo)

plugins_h = env.Cat( normalBuildDir + 'plugins.h', appendDir( 'tools/plugins/', Split("""
datalpha.inc
datfli.inc
datfname.inc
datfont.inc
datgrab.inc
datgrid.inc
datimage.inc
datitype.inc
datmidi.inc
datpal.inc
datsamp.inc
datworms.inc
""")));

# For some reason I have to call Program() from this file
# otherwise 'scons/' will be appended to all the sources
# and targets. 

# Build all other miscellaneous targets using the same environment
# that was used to build allegro but only link in liballeg
extraEnv = env.Copy()
liballeg = getLibraryName(debug)
extraEnv.Append(LIBPATH = [ libDir ])
if not static:
    extraEnv.Replace(LIBS = [ liballeg ])
else:
    extraEnv.Append(LIBS = [ liballeg ])

extraTargets = []
for func in extras:
    extraTargets.append(func(extraEnv,appendDir,normalBuildDir,libDir))

extraTargets.append(plugins_h)
Default(library, extraTargets, docs)
