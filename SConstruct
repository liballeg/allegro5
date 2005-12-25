import os
import sys

allegroVersion = '4.3.0'

def getPlatform():
    return sys.platform


## Do not change directories when reading other scons files via SConscript
SConscriptChdir(0)

## Returns a function that takes a directory and a list of files
## and returns a new list of files with a build directory prepended to it
def sourceFiles(dir, files):
    return map( lambda x: dir + '/' + x, files)

def appendDir(dir, files):
    return map( lambda x: dir + x, files )

extras = []
def addExtra(func):
	extras.append(func)

## Returns a tuple( env, files, dir ). Each platform that Allegro supports should be
## listed here and the proper scons/X.scons file should be SConscript()'ed.
## env - environment
## files - list of files that compose the Allegro library
## dir - directory where the library( dll, so ) should end up
def getLibraryVariables():
    if getPlatform() == "openbsd3":
        return SConscript('scons/bsd.scons', exports = [ 'sourceFiles', 'addExtra' ]) + tuple( [ "lib/unix" ] )
    if getPlatform() == "linux2":
        return SConscript('scons/linux.scons', exports = [ 'sourceFiles', 'addExtra' ]) + tuple([ "lib/unix" ])

env, files, libDir = getLibraryVariables()

optimizedDir = 'build/release/'
env.BuildDir(optimizedDir, 'src', duplicate = 0)
sharedLib = env.SharedLibrary('alleg-' + allegroVersion, appendDir(optimizedDir, files))
staticLib = env.StaticLibrary('alleg-' + allegroVersion, appendDir(optimizedDir, files))
env.Install(libDir, sharedLib)

env.Alias('lib', sharedLib)
env.Alias('shared', sharedLib)
env.Alias('library', sharedLib)
env.Alias('static', staticLib)

debugDir = 'build/debug/'
debugEnv = env.Copy()
debugEnv.Append(CCFLAGS = '-DDEBUG=1')
debugEnv.BuildDir(debugDir, 'src', duplicate = 0)
debugShared = debugEnv.SharedLibrary( 'allegd-' + allegroVersion, appendDir(debugDir, files))
Alias('debug-shared', debugShared)
Alias('debug', debugShared)
debugStatic = debugEnv.StaticLibrary( 'allegd-' + allegroVersion, appendDir(debugDir, files))
Alias('debug-static', debugStatic)

#exampleEnv = Environment(ENV = os.environ)

exampleEnv = env.Copy()

exampleBuildDir = optimizedDir + 'examples/'
def exampleSourceFiles(dir, files):
    return map(lambda x: exampleBuildDir + dir + '/' + x, files)

exampleEnv.BuildDir(exampleBuildDir, 'examples', duplicate = 0)
# env.BuildDir(exampleBuildDir, 'examples', duplicate = 0)

#exampleEnv.Append(CPPPATH = [ "include", "include/allegro" ])
#exampleEnv.Append(LIBPATH = [ libDir ])
#exampleEnv.Append(LIBS = [ 'alleg-' + allegroVersion ])

## This is wierd to the max. For some reason I have to call Program()
## from this file otherwise 'scons/' will be appended to all the sources
## and targets. 

extraEnv = env.Copy()
liballeg = 'alleg-' + allegroVersion
extraEnv.Append( LIBPATH = [ libDir ] )
extraEnv.Replace( LIBS = [ liballeg ] )

# exampleFunc = SConscript('scons/examples.scons', exports = [ 'exampleSourceFiles' ] )
# examples = exampleFunc(exampleEnv)

extraTargets = []
for func in extras:
	extraTargets.append(func(extraEnv,appendDir,optimizedDir))

# Alias('examples', examples)

Default(sharedLib, extraTargets)

