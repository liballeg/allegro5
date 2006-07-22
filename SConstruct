# Scons build scripts for Allegro by Jon Rafkind
# Circa 12/25/2005


# If you are on Linux/*NIX you need at least version 0.96.92 of SCons 

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
# 1. Compile the modules in unix( alsa, oss, etc ) - Mostly done( 90% )

# 2. Get the build system working on other platforms supported by Allegro, Windows being the most important one
# Linux - 95%
# OSX - 80%
# Windows - 80%

# 3. Allow arbitrary libraries to be dropped into the Allegro directory and automatically compiled using the Allegro SCons environments - 0%

import os
import sys

import SCons

def allegroHelp():
    return """
scons [options] [targets]

Possible options:
config=1 : Rerun the configure checks( UNIX only )
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

## Version of Allegro
allegroVersion = '4.3.0'

def getPlatform():
    return sys.platform
    
def appendDir(dir, files):
    return map(lambda x: dir + '/' + x, files)

# Do not change directories when reading other scons files via SConscript
SConscriptChdir(0)

class AllegroContext:
    def __init__(self):
        self.librarySource = []
        self.extraTargets = []
        self.libDir = "lib/dummy"
        self.libraryEnv = Environment()
	# Platform specific scons scripts should set the example env via
	# setExampleEnv(). In most cases the library env can be used:
	# context.setExampleEnv(context.getLibraryEnv().Copy())
        self.exampleEnv = False
        # libraries - list of libraries to link into Allegro test/example programs
        # Usually is just liballeg.so/dylib/dll but could also be something
        # like liballeg-main.a
        self.libraries = []
        self.setEnvs()

    def setLibraryDir(self,dir):
        self.libDir = dir

    def addLibrary(self,library):
        self.libraries.append(library)

    def getLibraries(self):
        return self.libraries

    def getLibraryDir(self):
        return self.libDir

    def getExtraTargets(self):
        return self.extraTargets

    def setEnvs(self):
        self.envs = [self.libraryEnv,self.exampleEnv]

    def getAllegroVersion(self):
        return allegroVersion

    def addExtra(self,func):
        self.extraTargets.append(func)

    def setLibraryEnv(self,env):
        self.libraryEnv = env
        self.setEnvs()

    def getLibraryEnv(self):
        return self.libraryEnv

    def setSConsignFile(self,file):
        for i in self.envs:
            i.SConsignFile(file)

    def getExampleEnv(self):
        return self.exampleEnv

    def setExampleEnv(self,env):
        self.exampleEnv = env
	self.setEnvs()

    def addFiles(self,dir,fileList):
        self.librarySource.extend(appendDir(dir,fileList))

    def getAllegroTarget(self,debug,static):
        def build(function,lib,dir):
            return function(lib, appendDir(dir, self.librarySource))

        def buildStatic(env,debug,dir):
            env.BuildDir(dir, 'src', duplicate = 0)
            return build(env.StaticLibrary,self.libDir + '/static/' + getLibraryName(debug),dir)

        def buildShared(env,debug,dir):
            env.BuildDir(dir, 'src', duplicate = 0)
            return build(env.SharedLibrary,self.libDir + '/shared/' + getLibraryName(debug),dir)

        debugEnv = self.libraryEnv.Copy()
        debugEnv.Append(CCFLAGS = '-DDEBUGMODE=1')

        debugStatic = buildStatic(debugEnv, 1, debugBuildDir)
        debugShared = buildShared(debugEnv, 1, debugBuildDir)
        normalStatic = buildStatic(self.libraryEnv, 0, optimizedBuildDir)
        normalShared = buildShared(self.libraryEnv, 0, optimizedBuildDir)

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

# Returns a function that takes a directory and a list of files
# and returns a new list of files with a build directory prepended to it
#def sourceFiles(dir, files):
#    return map(lambda x: dir + '/' + x, files)


# Subsequent scons files can call addExtra to add arbitrary targets
# that will be evaluated in this file

# Returns a tuple( env, files, dir ). Each platform that Allegro supports should be
# listed here and the proper scons/X.scons file should be SConscript()'ed.
# env - environment
# files - list of files that compose the Allegro library
# dir - directory where the library( dll, so ) should end up
def getAllegroContext():
    context = AllegroContext()
    file = ""
    if getPlatform() == "openbsd3":
        file = 'scons/bsd.scons'
    elif getPlatform() == "linux2":
        file = 'scons/linux.scons'
    elif getPlatform() == "win32":
        file = 'scons/win32.scons'
    elif getPlatform() == "darwin":
        file = 'scons/osx.scons'
    elif getPlatform() == "darwin":
        file = 'scons/osx.scons'
    else:
        file = 'scons/linux.scons'
    SConscript(file, exports = ['context'])
    return context

context = getAllegroContext()

# Stop cluttering everything with .sconsign files, use a single db file instead
context.setSConsignFile("build/signatures")

debugBuildDir = 'build/debug/'
optimizedBuildDir = 'build/release/'

def getLibraryName(debug):
    if debug:
        return 'allegd-' + context.getAllegroVersion()
    else:
        return 'alleg-' + context.getAllegroVersion()
        
debug = int(ARGUMENTS.get('debug',0))
static = int(ARGUMENTS.get('static',0))

if debug:
    normalBuildDir = debugBuildDir
else:
    normalBuildDir = optimizedBuildDir

context.getLibraryEnv().Append(CPPPATH = [ normalBuildDir ])

library = context.getAllegroTarget(debug,static)
Alias('library', library)
Install(context.getLibraryDir(), library)

context.addLibrary(library)

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

context.addExtra(buildDemo)

plugins_h = context.getLibraryEnv().Cat( 'tools/plugins/plugins.h', appendDir( 'tools/plugins/', Split("""
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
extraEnv = context.getExampleEnv().Copy()
# liballeg = getLibraryName(debug)
extraEnv.Append(LIBPATH = [ context.getLibraryDir() ])
if not static:
    extraEnv.Replace(LIBS = [context.getLibraries()])
else:
    extraEnv.Append(LIBS = [context.getLibraries()])

extraTargets = []
for func in context.getExtraTargets():
    extraTargets.append(func(extraEnv,appendDir,normalBuildDir,context.getLibraryDir()))

extraTargets.append(plugins_h)
Default(library, extraTargets, docs)
