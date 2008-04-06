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
minorVersion = '9'
microVersion = '3'

## Version of Allegro
allegroVersion = '%s.%s.%s' % (majorVersion, minorVersion, microVersion)

# Do not change directories when reading other scons files via SConscript
SConscriptChdir(0)

def appendDir(directory, files):
    return [directory + "/" + x for x in files]

class AllegroContext:
    """This is simply a class to hold together all the various build info."""

    def __init__(self, env):
        self.librarySource = []
        self.extraTargets = []

	# Set in one of the scons/*.scons files
        self.installDir = "tmp"
        self.libDir = "lib/dummy"

        self.libraryEnv = env
	
	# Where md5 signatures are placed
	self.sconsignFile = 'build/signatures'

        self.debug = int(self.getLibraryEnv()['debug'])
        self.static = int(self.getLibraryEnv()['static'])
        self.platform = self.getLibraryEnv()['platform']

        # Each platform should set its own install function
        # install :: library -> list of targets
        self.install = lambda lib: []

        # Platform specific scons scripts should set the example env via
        # setExampleEnv(). In most cases the library env can be used:
        # context.setExampleEnv(context.getLibraryEnv().Copy())
        self.exampleEnv = Environment()

        # libraries - list of libraries to link into Allegro test/example programs
        # Usually is just liballeg.so/dylib/dll but could also be something
        # like liballeg-main.a
        self.libraries = []
        self.setEnvs()

    def getPlatform(self):
        if self.platform: return self.platform
        return sys.platform

    def matchPlatform(self, name):
        return name in self.getPlatform()

    def onBsd(self):
        return self.matchPlatform('openbsd')

    def onLinux(self):
        return self.matchPlatform('linux')

    def onWindows(self):
        return self.matchPlatform('win32')

    def onOSX(self):
        return self.matchPlatform('darwin')

    def setLibraryDir(self, d):
        self.libDir = d

    def getDebug(self):
        return self.debug

    def getStatic(self):
        return self.static

    def addLibrary(self, library):
        self.libraries.append(library)

    def getLibraries(self):
        return self.libraries

    def setInstaller(self, installer):
        self.install = installer

    def getLibraryDir(self):
        return self.libDir

    def getVersion(self):
        return allegroVersion

    def getExtraTargets(self):
        return self.extraTargets

    def setEnvs(self):
        self.envs = [self.libraryEnv, self.exampleEnv]
	# Force all envs to use the same dblite file
        for i in self.envs:
            i.SConsignFile(self.sconsignFile)

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

    def setInstallDir(self, d):
        self.installDir = d 

    ## pass 'depends = True' if the result of 'func' should
    ## depend on the Allegro library being built.
    ## This is important to make parallel builds, -j 2, work.
    def addExtra(self, func, depends = False, use_build_env = False):
        class ExtraTarget:
            pass
        et = ExtraTarget()
        et.func = func
	et.depends = depends
        et.use_build_env = use_build_env
        self.extraTargets.append(et)

    def setLibraryEnv(self, env):
        self.libraryEnv = env
        self.setEnvs()

    def getLibraryEnv(self):
        return self.libraryEnv

    #def setSConsignFile(self, file):
    #    self.sconsignFile = file

    def getExampleEnv(self):
        return self.exampleEnv

    def setExampleEnv(self, env):
        self.exampleEnv = env
        self.setEnvs()

    def add_files(self, files):
        self.librarySource.extend(files)

    def addFiles(self, d, fileList):
        self.librarySource.extend(appendDir(d, fileList))

    def libraryName(self,name):
        if self.static == 1:
            return name + '_s'
        else:
            return name

    # Build a library given an env
    # Library is static if Allegro is configured with static=1
    # or shared if static=0
    def makeLibrary(self,env):
        if self.static == 1:
            return lambda name, *rest : apply(env.StaticLibrary, [name + '_s'] + list(rest) )
        else:
            return lambda name, *rest : apply(env.SharedLibrary, [name] + list(rest) )

    def getAllegroTarget(self):
        def build(function, lib, d):
            return function(self.getLibraryDir() + '/' + lib, appendDir(d, self.librarySource))

        def buildStatic(env, debug, d):
            env.BuildDir(d, '.', duplicate = 0)
            # return build(env.StaticLibrary, self.libDir + '/static/' + getLibraryName(debug), d)
            return build(env.StaticLibrary, getLibraryName(debug,True), d)

        def buildShared(env, debug, d):
            env.BuildDir(d, '.', duplicate = 0)
            # return build(env.SharedLibrary, self.libDir + '/shared/' + getLibraryName(debug), d)
            return build(env.SharedLibrary, getLibraryName(debug,False), d)

        debugEnv = self.libraryEnv.Copy()
        debugEnv.Append(CCFLAGS = '-DDEBUGMODE=1')

        debugStatic = buildStatic(debugEnv, 1, debugBuildDir)
        debugShared = buildShared(debugEnv, 1, debugBuildDir)
        normalStatic = buildStatic(self.libraryEnv, 0, optimizedBuildDir)
        normalShared = buildShared(self.libraryEnv, 0, optimizedBuildDir)

        Alias('debug-static', debugStatic)
        Alias('debug-shared', debugShared)
        Alias('static', normalStatic)
        Alias('shared', normalShared)

        if self.debug == 1 and self.static == 1:
            return debugStatic
        elif self.debug == 1:
            return debugShared
        elif self.static == 1:
            return normalStatic
        else:
            return normalShared

# Returns a function that takes a directory and a list of files
# and returns a new list of files with a build directory prepended to it
#def sourceFiles(d, files):
#    return map(lambda x: d + '/' + x, files)

def defaultEnvironment():
    import os
    env = Environment( ENV = os.environ )
    if ARGUMENTS.get("mingw"):
        Tool("mingw")(env)
    opts = Options('options.py', ARGUMENTS)
    opts.Add('static', 'Set Allegro to be built statically', 0)
    opts.Add('debug', 'Build the debug version of Allegro', 0)
    opts.Add('platform', 'Use a specific platform', "")
    opts.Add('CC', 'Use a specific c compiler', env["CC"])
    opts.Add('CXX', 'Use a specific c++ compiler', env["CXX"])
    opts.Add('CFLAGS', 'Override compiler flags', env.get("CFLAGS", ""))
    opts.Add('mingw', 'For using mingw', "")
    opts.Update(env)
    opts.Save('options.py', env)
    Help(opts.GenerateHelpText(env))
    return env

# Subsequent scons files can call addExtra to add arbitrary targets
# that will be evaluated in this file

# Returns a tuple( env, files, d ). Each platform that Allegro supports should be
# listed here and the proper scons/X.scons file should be SConscript()'ed.
# env - environment
# files - list of files that compose the Allegro library
# d - directory where the library( dll, so ) should end up
def getAllegroContext():
    context = AllegroContext(defaultEnvironment())

    context.cmake = helpers.read_cmake_list("cmake/FileList.cmake")

    file = ""
    if context.onBsd():
        file = 'scons/bsd.scons'
    elif context.onLinux():
        file = 'scons/linux.scons'
    elif context.onWindows():
        file = 'scons/win32.scons'
    elif context.onOSX():
        file = 'scons/osx.scons'
    else:
        print "Warning: unknown system type %s. Defaulting to linux." % (
            context.getPlatform())
        file = 'scons/linux.scons'
    SConscript(file, exports = ['context'])
    return context

context = getAllegroContext()

debugBuildDir = 'build/debug/' + context.getPlatform() + "/"
optimizedBuildDir = 'build/release/' + context.getPlatform() + "/"

def getLibraryName(debug,static):
    if debug:
        if static:
            return 'allegd_s-' + context.getAllegroVersion()
        else:
            return 'allegd-' + context.getAllegroVersion()
    else:
	if static:
            return 'alleg_s-' + context.getAllegroVersion()
        else:
            return 'alleg-' + context.getAllegroVersion()
        
if context.getDebug():
    normalBuildDir = debugBuildDir
else:
    normalBuildDir = optimizedBuildDir

context.getLibraryEnv().Append(CPPPATH = [ normalBuildDir ])

library = context.getAllegroTarget()
if context.getStatic() == 1 and context.getDebug() == 1:
	print "Building static debug library"
elif context.getStatic() == 1:
	print "Building static release library"
elif context.getDebug() == 1:
	print "Building shared debug library"
else:
	print "Building shared release library"
Alias('library', library)

# m = Move(context.getLibraryEnv(), library)

# In scons 0.96.92 the Move() action only accepts strings for filenames
# as opposed to targets. This method should replace Move() in scons at
# some point.
# *Not used* - 12/9/2007
def XMove(env, target, source):
    sources = env.arg2nodes(source, env.fs.File)
    targets = env.arg2nodes(target, env.fs.Dir)
    result = []
    def moveFunc(target, source, env):
        import shutil
        shutil.move(source[0].path, target[0].path)
        return 0

    def moveStr(target, source, env):
        return "Moving %s to %s" % (source[0].path, target[0].path)

    MoveBuilder = SCons.Builder.Builder(action = SCons.Action.Action(moveFunc, moveStr), name='MoveBuilder')
    for src, tgt in map(lambda x, y: (x, y), sources, targets):
        result.extend(MoveBuilder(env, env.fs.File(src.name, tgt), src))
    return result

# mover = Builder(action = XMove)
# context.getLibraryEnv().Append( BUILDERS = { 'XMove' : mover } )
# context.getLibraryEnv().XMove( Dir(context.getLibraryDir()), library )

# m = context.getLibraryEnv().Move(context.getLibraryDir(), library)
# m = context.getLibraryEnv().Move(context.getLibraryDir(), library)
# install_to_lib_dir = XMove(context.getLibraryEnv(), context.getLibraryDir(), library)
# install_to_lib_dir = Install(context.getLibraryDir(), library)

# Execute(Move(context.getLibraryDir(), library))
# Execute(Action(os.rename(library, context.getLibraryDir() + '/' + library)))

# context.addLibrary(library)
context.addLibrary('-l%s' % getLibraryName(context.getDebug(),context.getStatic()))

docs = SConscript("scons/docs.scons", exports = ["normalBuildDir"])
Alias('docs', docs)

SConscript("scons/naturaldocs.scons")

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
if not context.getStatic():
    extraEnv.Replace(LIBS = [context.getLibraries()])
else:
    extraEnv.Append(LIBS = [context.getLibraries()])

extraTargets = []
for et in context.getExtraTargets():
    useEnv = extraEnv
    if et.use_build_env:
        useEnv = context.getLibraryEnv()
    make = et.func(useEnv,appendDir, normalBuildDir, context.getLibraryDir())
    if et.depends:
        useEnv.Depends(make,library)
    extraTargets.append(make)

extraTargets.append(plugins_h)
Default(library, extraTargets, docs)

# Depends(library, extraTargets)

Alias('install', context.install(library))
