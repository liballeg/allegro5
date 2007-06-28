# vi: syntax=python
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
# 1. Compile the modules in unix( alsa, oss, etc ) - Mostly done( 95% )

# 2. Get the build system working on other platforms supported by Allegro, Windows being the most important one
# Linux - 95%. Still need to handle install
# OSX - 80%. Still need to handle install
# Windows - 80%. asm doesnt compile yet

# 3. Allow arbitrary libraries to be dropped into the Allegro directory and automatically compiled using the Allegro SCons environments - 0%

import os, sys
import SCons
sys.path.append("scons")
import  helpers

def allegroHelp():
    return """
scons [options] [targets]

Possible targets are:
debug-static : Build a static library with debug mode on
debug-shared : Build a shared library with debug mode on
static : Build a static library with debug off
shared : Build a shared library with debug off
library : Build the library which is configured by static=X and debug=X
examples : Build all the examples
docs : Build the docs
demo : Build the demo
install : Install Allegro

To turn an option on use the form option=1. Possible options are:
    """

Help(allegroHelp())

# Generate build directory (since we put the signatures db there)
try: os.mkdir("build")
except OSError: pass

majorVersion = '4'
minorVersion = '3'
microVersion = '0'

## Version of Allegro
allegroVersion = '%s.%s.%s' % (majorVersion,minorVersion,microVersion)

def getPlatform():
    return sys.platform

def matchPlatform(name):
    return name in getPlatform()

def onBsd():
    return matchPlatform('openbsd')

def onLinux():
    return matchPlatform('linux')

def onWindows():
    return matchPlatform('win32')

def onOSX():
    return matchPlatform('darwin')
    
def appendDir(dir, files):
    return map(lambda x: dir + '/' + x, files)

# Do not change directories when reading other scons files via SConscript
SConscriptChdir(0)

class AllegroContext:
    """This is simply a class to hold together all the various build info."""

    def __init__(self,env):
        self.librarySource = []
        self.extraTargets = []
        self.installDir = "tmp"
        self.libDir = "lib/dummy"
        self.libraryEnv = env

        # Each platform should set its own install function
        # install :: library -> list of targets
        self.install = lambda lib: []

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

    def setInstaller(self,installer):
        self.install = installer

    def getLibraryDir(self):
        return self.libDir

    def getVersion(self):
        return allegroVersion

    def getExtraTargets(self):
        return self.extraTargets

    def setEnvs(self):
        self.envs = [self.libraryEnv,self.exampleEnv]

    def getMajorVersion(self):
        return majorVersion
    
    def getMinorVersion(self):
        return minorVersion
    
    def getMicroVersion(self):
        return microVersion

    def getAllegroVersion(self):
        return allegroVersion

    def getInstallDir(self):
        return self.installDir

    def setInstallDir(self,dir):
        self.installDir = dir 

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

    def add_files(self, files):
        # strip leading "src/"
        self.librarySource.extend([x[4:] for x in files])

    def addFiles(self,dir,fileList):
        self.librarySource.extend(appendDir(dir,fileList))

    def getAllegroTarget(self,debug,static):
        def build(function,lib,dir):
            return function(self.getLibraryDir() + '/' + lib, appendDir(dir, self.librarySource))

        def buildStatic(env,debug,dir):
            env.BuildDir(dir, 'src', duplicate = 0)
            # return build(env.StaticLibrary,self.libDir + '/static/' + getLibraryName(debug),dir)
            return build(env.StaticLibrary, getLibraryName(debug),dir)

        def buildShared(env,debug,dir):
            env.BuildDir(dir, 'src', duplicate = 0)
            # return build(env.SharedLibrary,self.libDir + '/shared/' + getLibraryName(debug),dir)
            return build(env.SharedLibrary, getLibraryName(debug),dir)

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

def defaultEnvironment():
    env = Environment()
    opts = Options('options.py', ARGUMENTS)
    opts.Add('static', 'Set Allegro to be built statically', 0)
    opts.Add('debug', 'Build the debug version of Allegro', 0)
    opts.Update(env)
    opts.Save('options.py',env)
    Help(opts.GenerateHelpText(env))
    return env

# Subsequent scons files can call addExtra to add arbitrary targets
# that will be evaluated in this file

# Returns a tuple( env, files, dir ). Each platform that Allegro supports should be
# listed here and the proper scons/X.scons file should be SConscript()'ed.
# env - environment
# files - list of files that compose the Allegro library
# dir - directory where the library( dll, so ) should end up
def getAllegroContext():
    context = AllegroContext(defaultEnvironment())
    context.cmake = helpers.read_cmake_list("cmake/FileList.cmake")

    file = ""
    if onBsd():
        file = 'scons/bsd.scons'
    elif onLinux():
        file = 'scons/linux.scons'
    elif onWindows():
        file = 'scons/win32.scons'
    elif onOSX():
        file = 'scons/osx.scons'
    else:
        print "Warning: unknown system type %s" % getPlatform()
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
        
# debug = int(ARGUMENTS.get('debug',0))
# static = int(ARGUMENTS.get('static',0))
debug = int(context.getLibraryEnv()['debug'])
static = int(context.getLibraryEnv()['static'])

if debug:
    normalBuildDir = debugBuildDir
else:
    normalBuildDir = optimizedBuildDir

context.getLibraryEnv().Append(CPPPATH = [ normalBuildDir ])

library = context.getAllegroTarget(debug,static)
if static == 1 and debug == 1:
	print "Building debugged static library"
elif static == 1:
	print "Building static library"
elif debug == 1:
	print "Building debug library"
else:
	print "Building normal library"
Alias('library', library)

# m = Move(context.getLibraryEnv(),library)

# In scons 0.96.92 the Move() action only accepts strings for filenames
# as opposed to targets. This method should replace Move() in scons at
# some point.
def XMove(env,target,source):
    sources = env.arg2nodes(source, env.fs.File)
    targets = env.arg2nodes(target, env.fs.Dir)
    result = []
    def moveFunc(target, source, env):
        import shutil
        shutil.move(source[0].path,target[0].path)
        return 0

    def moveStr(target, source, env):
        return "Moving %s to %s" % (source[0].path,target[0].path)

    MoveBuilder = SCons.Builder.Builder(action = SCons.Action.Action(moveFunc,moveStr), name='MoveBuilder')
    for src, tgt in map(lambda x, y: (x, y), sources, targets):
        result.extend(MoveBuilder(env, env.fs.File(src.name, tgt), src))
    return result

# mover = Builder(action = XMove)
# context.getLibraryEnv().Append( BUILDERS = { 'XMove' : mover } )
# context.getLibraryEnv().XMove( Dir(context.getLibraryDir()), library )

# m = context.getLibraryEnv().Move(context.getLibraryDir(),library)
# m = context.getLibraryEnv().Move(context.getLibraryDir(), library)
# install_to_lib_dir = XMove(context.getLibraryEnv(), context.getLibraryDir(), library)
# install_to_lib_dir = Install(context.getLibraryDir(),library)

# Execute(Move(context.getLibraryDir(), library))
# Execute(Action(os.rename(library,context.getLibraryDir() + '/' + library)))

context.addLibrary(library)

docs = SConscript("scons/docs.scons", exports = ["normalBuildDir"])
Alias('docs', docs)

SConscript("scons/naturaldocs.scons")

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

# context.addExtra(buildDemo)

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
# extraEnv.Append(LIBPATH = [ context.getLibraryDir() ])
if not static:
    extraEnv.Replace(LIBS = [context.getLibraries()])
else:
    extraEnv.Append(LIBS = [context.getLibraries()])

extraTargets = []
for func in context.getExtraTargets():
    extraTargets.append(func(extraEnv,appendDir,normalBuildDir,context.getLibraryDir()))

extraTargets.append(plugins_h)
Default(library, extraTargets, docs)

# Depends(library,extraTargets)

Alias('install', context.install(library))
