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
install
install-static
install-debug
install-static-debug
install-everything

To turn an option on use the form option=1. Possible options are:
    """

Help(allegroHelp())

# Make this an option?
BUILD_DIR = "scons_build"

# Generate build directory (since we put the signatures db there)
try: os.mkdir(BUILD_DIR)
except OSError: pass

SConsignFile(BUILD_DIR + "/scons-signatures")

def readVersion():
    # read version
    defines = {}
    for line in open("include/allegro5/base.h"):
        if line.startswith("#define"):
            x = line[8:].split()
            if len(x) == 2:
                defines[x[0]] = x[1]
    majorVersion = defines["ALLEGRO_VERSION"]
    minorVersion = defines["ALLEGRO_SUB_VERSION"]
    microVersion = defines["ALLEGRO_WIP_VERSION"]
    return [majorVersion, minorVersion, microVersion]

majorVersion, minorVersion, microVersion = readVersion()

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

    # Directory that files are installed to before being really installed
    def temporaryInstallDir(self):
        return "tmp-install"
    
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
    
    def aliasInstall(self,name,target):
        Alias(name,target)
        Alias('install',target)

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
    
    def aliasInstall(self,name,target):
        Alias(name + '-static',target)
        Alias('install-static',target)

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
    
    def aliasInstall(self,name,target):
        Alias(name + '-debug',target)
        Alias('install-debug',target)

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
        env.Append(CFLAGS = ['-DDEBUGMODE=1'])
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
    
    def aliasInstall(self,name,target):
        Alias(name + '-static-debug',target)
        Alias('install-static-debug',target)

    def libraryName(self, name):
        return name + '_sd'

    def buildLibrary(self, env, name, source):
        return env.StaticLibrary(name, source)

    def isStatic(self):
        return True

    def isDebug(self):
        return True
    
    def defaultEnvironment(self):
        env = Environment(ENV = os.environ)
        env.Append(CFLAGS = ['-DDEBUGMODE=1'])
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
        import re
        m = re.compile(".*%s.*" % name)
        return (m.match(getPlatform()) != None)

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
    return SConscript(getPlatformFile(), build_dir = context.getBuildDir(), exports = ['context','env'])
    
def buildNormal():
    return doBuild(AllegroContextNormal())

def buildStatic():
    return doBuild(AllegroContextStatic())

def buildDebug():
    return doBuild(AllegroContextDebug())

def buildStaticDebug():
    return doBuild(AllegroContextStaticDebug())

installNormal = buildNormal()
installStatic = buildStatic()
installDebug = buildDebug()
installStaticDebug = buildStaticDebug()

# Combine all the possible installed files into a giant list
def combineInstall(*installs):
    # all is a hashmap from the path minus the common prefix to
    # the number of times path has been seen
    all = {}
    # real is a hashmap of path minus the common prefix to
    # an array containing the real FS node and the prefix
    real = {}
    for targets in installs:
        common = os.path.commonprefix([str(f[0]) for f in targets])
        for f in targets:
            base = str(f[0]).replace(common, '')
            # If its unique there will only be one entry, if its not
            # unique then whoever was last will be the installed on
            real[base] = [f[0], base]
            try:
                all[base] += 1
            except KeyError:
                all[base] = 1

    return [all, real]


# Filter files that are repeated for each variant - mostly header files
def filterCommon(all_real):
    all = all_real[0]
    real = all_real[1]
    def keep(c):
        if all[c] > 1:
            # print "Keep " + str(real[c])
            return real[c]
        else:
            return None
    
    return filter(lambda n: n != None, [keep(c) for c in all.keys()])

# Filter files that only occur once - mostly libraries
def filterUncommon(all_real):
    all = all_real[0]
    real = all_real[1]
    def keep(c):
        if all[c] == 1:
            # print "Keep " + str(real[c])
            return real[c]
        else:
            return None
    
    return filter(lambda n: n != None, [keep(c) for c in all.keys()])

# 'all' is supposed to be the list of uncommon files as computed by filterUncommon
# and 'files' is the installed files from one variant, such as normal, or debug
# This function returns the intersection of the two lists so that only the uncommon
# files from a specific variant will be returned
def filterType(files, all):
    names = map(lambda z: z[0].path, files)
    # names = files
    # print "Filtering from " + str(names)
    def check(n):
        # print "Checking for " + n[0].path
        return n[0].path in names
    return filter(check, all)

def doInstall(targets):
    dir = helpers.install
    if not os.path.isabs(dir):
        dir = '#' + dir
    def install(t):
        return InstallAs(os.path.join(dir, t[1]), t[0])
    all = [install(target) for target in targets]
    return all

allInstall = combineInstall(installNormal, installStatic,
                            installDebug, installStaticDebug)

common = filterCommon(allInstall)
uncommon = filterUncommon(allInstall)

Alias('install-common', doInstall(common))
Alias('install-normal-files', doInstall(filterType(installNormal, uncommon)))
Alias('install-static-files', doInstall(filterType(installStatic, uncommon)))
Alias('install-debug-files', doInstall(filterType(installDebug, uncommon)))
Alias('install-static-debug-files', doInstall(filterType(installStaticDebug, uncommon)))

# Regular install is the normal library
Alias('install', ['install-common', 'install-normal-files'])
Alias('install-static', ['install-common', 'install-static-files'])
Alias('install-debug', ['install-common', 'install-debug-files'])
Alias('install-static-debug', ['install-common', 'install-static-debug-files'])

# Default is what comes out of buildNormal()
Default('all')

# Build the world!
Alias('everything',['all','all-static','all-debug','all-static-debug'])
Alias('install-everything', ['install', 'install-static', 'install-debug', 'install-static-debug'])
