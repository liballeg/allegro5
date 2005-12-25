import os
import sys

allegroVersion = '4.3.0'

def getPlatform():
    return sys.platform

buildDir = 'obj/'

## Do not change directories when reading other scons files via SConscript
SConscriptChdir(0)

## Returns a function that takes a directory and a list of files
## and returns a new list of files with a build directory prepended to it
def sourceFiles(dir, files):
    return map( lambda x: buildDir + dir + '/' + x, files)

## Returns a tuple( env, files, dir ). Each platform that Allegro supports should be
## listed here and the proper scons/X.scons file should be SConscript()'ed.
## env - environment
## files - list of files that compose the Allegro library
## dir - directory where the library( dll, so ) should end up
def getLibraryVariables():
    if getPlatform() == "openbsd3":
        return SConscript('scons/bsd.scons', exports = 'sourceFiles') + tuple( [ "lib/unix" ] )
    if getPlatform() == "linux2":
        return SConscript('scons/linux.scons', exports = [ 'sourceFiles' ]) + tuple([ "lib/unix" ])

env, files, libDir = getLibraryVariables()

env.BuildDir(buildDir, 'src', duplicate = 0)
sharedLib = env.SharedLibrary('alleg-' + allegroVersion, files)
staticLib = env.StaticLibrary('alleg-' + allegroVersion, files)
env.Install(libDir, sharedLib)

env.Alias('lib', sharedLib)
env.Alias('shared', sharedLib)
env.Alias('library', sharedLib)
env.Alias('static', staticLib)

#exampleEnv = Environment(ENV = os.environ)

exampleEnv = env.Copy()

exampleBuildDir = buildDir + 'examples/'
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
liballeg = 'alleg-' + allegroVersion
exampleEnv.Append( LIBPATH = [ libDir ] )
exampleEnv.Replace( LIBS = [ liballeg ] )
exampleFunc = SConscript('scons/examples.scons', exports = [ 'exampleSourceFiles' ] )

examples = exampleFunc(exampleEnv)
Alias('examples', examples)

Default(sharedLib, examples)

