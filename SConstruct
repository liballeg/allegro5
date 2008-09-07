# vi: syntax=python
# Scons build scripts for Allegro by Jon Rafkind
# Circa 12/25/2005

# If you are on Linux/*NIX you need at least version 0.96.92 of SCons 

# debug=1 is the same as prefixing debug- to the target and
# static=1 is the same as using the static target
# Thus debug=1 static=1 is the same as debug-static

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

Possible targets:
all
all-static
all-debug
all-static-debug
everything

To turn an option on use the form option=1. Possible options are:
    """

Help(allegroHelp())

# Make this an option?
BUILD_DIR = "scons_build"

# Generate build directory (since we put the signatures db there)
try: os.mkdir(BUILD_DIR)
except OSError: pass

majorVersion = '4'
minorVersion = '9'
microVersion = '4'

## Version of Allegro
allegroVersion = '%s.%s.%s' % (majorVersion, minorVersion, microVersion)

# Do not change directories when reading other scons files via SConscript
SConscriptChdir(0)

def appendDir(directory, files):
    return [directory + "/" + x for x in files]

# Contains getters for other modules to inspect the current system
# This is a base class which should not be instantiated. See one of
# the subclasses below.
class AllegroContext:
    def __init__(self):
        self.cmake = helpers.read_cmake_list("cmake/FileList.cmake")
        self.librarySource = []
    
    # List of source for the Allegro library (and nothing else!)
    def getLibrarySource(self):
        return self.librarySource
    
    # Version of allegro (major.minor.micro)
    def getVersion(self):
        return allegroVersion

    def getGlobalDir(self):
        return BUILD_DIR + '/'

    # Add files to the source for the Allegro library
    def addFiles(self,files):
        self.librarySource.extend(files)

    # Just the major version of Allegro
    def getMajorVersion(self):
        return majorVersion

    # Just the minor version of Allegro
    def getMinorVersion(self):
        return minorVersion

# Shared non-debug
class AllegroContextNormal(AllegroContext):
    def __init__(self):
        AllegroContext.__init__(self)
    
    # Alias a target and add it to the 'all' alias
    def alias(self,name,target):
        Alias(name,target)
        Alias('all',target)

    # Library name is as-is
    def libraryName(self, name):
        return name
    
    # Build the library
    def buildLibrary(self, env, name, source):
        return env.SharedLibrary(name, source)

    def isStatic(self):
        return False

    def isDebug(self):
        return False

    def defaultEnvironment(self):
        return Environment(ENV = os.environ)

    def getBuildDir(self):
        return BUILD_DIR + '/release'

# Static non-debug
class AllegroContextStatic(AllegroContext):
    def __init__(self):
        AllegroContext.__init__(self)
    
    # Anything aliased with this target ends up with -static on the end
    def alias(self,name,target):
        Alias(name + '-static',target)
        Alias('all-static',target)

    # Adds _s to all libraries
    def libraryName(self, name):
        return name + '_s'

    # Build libraries statically
    def buildLibrary(self, env, name, source):
        return env.StaticLibrary(name, source)

    def isStatic(self):
        return True

    def isDebug(self):
        return False
    
    def defaultEnvironment(self):
        return Environment(ENV = os.environ)

    def getBuildDir(self):
        return BUILD_DIR + '/release-static'

# Shared debug
class AllegroContextDebug(AllegroContext):
    def __init__(self):
        AllegroContext.__init__(self)
    
    def alias(self,name,target):
        Alias(name + '-debug',target)
        Alias('all-debug',target)

    def libraryName(self, name):
        return name + '_d'

    def buildLibrary(self, env, name, source):
        return env.SharedLibrary(name, source)

    def isStatic(self):
        return False

    def isDebug(self):
        return True
    
    def defaultEnvironment(self):
        env = Environment(ENV = os.environ)
        env.Append(CFLAGS = ['-DDEBUG'])
        return env

    def getBuildDir(self):
        return BUILD_DIR + '/debug'

# Static debug
class AllegroContextStaticDebug(AllegroContext):
    def __init__(self):
        AllegroContext.__init__(self)
    
    def alias(self,name,target):
        Alias(name + '-static-debug',target)
        Alias('all-static-debug',target)

    def libraryName(self, name):
        return name + '_sd'

    def buildLibrary(self, env, name, source):
        return env.StaticLibrary(name, source)

    def isStatic(self):
        return False

    def isDebug(self):
        return True
    
    def defaultEnvironment(self):
        env = Environment(ENV = os.environ)
        env.Append(CFLAGS = ['-DDEBUG'])
        return env

    def getBuildDir(self):
        return BUILD_DIR + '/debug-static'

def getPlatformFile():
    def getPlatform():
        import sys
        # if self.platform: return self.platform
        # return sys.platform
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

    if onBsd():
        return 'scons/bsd.scons'
    elif onLinux():
        return 'scons/linux.scons'
    elif onWindows():
        return 'scons/win32.scons'
    elif onOSX():
        return 'scons/osx.scons'
    else:
        return 'scons/linux.scons'

def doBuild(context):
    env = context.defaultEnvironment()
    SConscript(getPlatformFile(), build_dir = context.getBuildDir(), exports = ['context','env'])
    
def buildNormal():
    doBuild(AllegroContextNormal())

def buildStatic():
    doBuild(AllegroContextStatic())

def buildDebug():
    doBuild(AllegroContextDebug())

def buildStaticDebug():
    doBuild(AllegroContextStaticDebug())

buildNormal()
buildStatic()
buildDebug()
buildStaticDebug()

# Default is what comes out of buildNormal()
Default('all')

# Build the world!
Alias('everything',['all','all-static','all-debug','all-static-debug'])

if False:
    context = getAllegroContext()
    
    debugBuildDir = BUILD_DIR + '/debug/' + context.getPlatform() + "/"
    optimizedBuildDir = BUILD_DIR + '/release/' + context.getPlatform() + "/"
    
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
    
    if context.onBsd():
        SConscript('scons/bsd.scons', build_dir = normalBuildDir, exports = 'context')
    elif context.onLinux():
        SConscript('scons/linux.scons', build_dir = BUILD_DIR, exports = 'context')
    elif context.onWindows():
        SConscript('scons/win32.scons', build_dir = BUILD_DIR, exports = 'context')
    elif context.onOSX():
        SConscript('scons/osx.scons', build_dir = BUILD_DIR, exports = 'context')
    else:
        SConscript('scons/linux.scons', build_dir = BUILD_DIR, exports = 'context')
        
    Default(SConscript("scons/naturaldocs.scons"))

if False:
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
    
    # we don't use makedoc any longer
    #docs = SConscript("scons/docs.scons", exports = ["normalBuildDir"])
    #Alias('docs', docs)
    
    SConscript("scons/naturaldocs.scons")
    
    # For some reason I have to call Program() from this file
    # otherwise 'scons/' will be appended to all the sources
    # and targets. 
    
    # Build all other miscellaneous targets using the same environment
    # that was used to build allegro but only link in liballeg
    extraEnv = context.getExampleEnv().Clone()
    
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
    
    Default(library, extraTargets)
    
    # Depends(library, extraTargets)
    
    Alias('install', context.install(library))
