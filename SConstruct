## Scons build scripts for Allegro by Jon Rafkind
## Circa 12/25/2005
## Any line starting with a double hash(#) is a documentation comment
## while any line starting with a single hash(#) is commenting old/broken code

## Possible targets should be
## shared - Shared allegro library. Either a .so( unix ) or .dll( windows )
## static - Static allegro library, .a file.
## debug-shared - Shared library with debugging turned on
## debug-static - Static library with debugging turned on
## examples - All the examples in examples/
## tools - All the tools in tools/
## docs - All the documentation
## tests - All the tests in tests/
## demo - The demo game

## debug=1 is the same as prefixing debug- to the target and
## static=1 is the same as using the static target
## Thus debug=1 static=1 is the same as debug-static

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
    return map(lambda x: dir + '/' + x, files)

def appendDir(dir, files):
    return map(lambda x: dir + '/' + x, files)

## Subsequent scons files can call addExtra to add arbitrary targets
## that will be evaluated in this file
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
        return SConscript('scons/bsd.scons', exports = [ 'sourceFiles', 'addExtra' ]) + tuple([ "lib/unix/" ])
    if getPlatform() == "linux2":
        return SConscript('scons/linux.scons', exports = [ 'sourceFiles', 'addExtra' ]) + tuple([ "lib/unix/" ])
    if getPlatform() == "win32":
        return SConscript('scons/win32.scons', exports = [ 'sourceFiles', 'addExtra' ]) + tuple([ "lib/win32/" ])

env, files, libDir = getLibraryVariables()


debugBuildDir = 'build/debug/'
optimizedBuildDir = 'build/release/'

def getLibraryName(debug):
    if debug:
        return 'allegd-' + allegroVersion
    else:
        return 'alleg-' + allegroVersion

def getAllegroTarget(debug,static):
    def build(function,dir):
        env.BuildDir(dir, 'src', duplicate = 0)
        return function(libDir + getLibraryName(debug), appendDir(dir, files))

    if debug == 1 and static == 1:
        env.Append(CCFLAGS = '-DDEBUG=1')
        lib = build(env.StaticLibrary, debugBuildDir)
        Alias('static', lib)
        return lib
    elif debug == 1:
        env.Append(CCFLAGS = '-DDEBUG=1')
        lib = build(env.SharedLibrary, debugBuildDir)
        Alias('shared', lib)
        return lib
    elif static == 1:
        lib = build(env.StaticLibrary, optimizedBuildDir)
        Alias('debug-static', lib)
        return lib
    else:
        lib = build(env.SharedLibrary, optimizedBuildDir)
        Alias('debug-shared', lib)
        return lib

debug = int(ARGUMENTS.get('debug',0))
static = int(ARGUMENTS.get('static',0))

if debug:
    normalBuildDir = debugBuildDir
else:
    normalBuildDir = optimizedBuildDir

env.Append(CPPPATH = [ normalBuildDir ])

library = getAllegroTarget(debug,static)
Alias('library', library)

## Build the documentation
docEnv = Environment(ENV = os.environ)
docEnv.BuildDir(normalBuildDir + 'makedoc/', 'docs/src/makedoc', duplicate = 0)
makeDocFiles = Split("""
   makechm.c
   makedevh.c
   makedoc.c
   makehtml.c
   makeman.c
   makemisc.c
   makertf.c
   makesci.c
   maketexi.c
   maketxt.c
    """);

docs = [] 
docs.append(docEnv.Program('docs/makedoc', appendDir(optimizedBuildDir + '/makedoc/', makeDocFiles)))

makeHTML = Builder(action = 'docs/makedoc -html $TARGETS $SOURCES', 
           suffix = '.html', 
           src_suffix = '._tx');

makeScite = Builder(action = 'docs/makedoc -scite $TARGETS $SOURCES', 
           suffix = '.api', 
           src_suffix = '._tx');

makeTexi = Builder(action = 'docs/makedoc -texi $TARGETS $SOURCES', 
           suffix = '.texi', 
           src_suffix = '._tx');

makeRTF = Builder(action = 'docs/makedoc -rtf $TARGETS $SOURCES', 
           suffix = '.rtf', 
           src_suffix = '._tx');

makeInfo = Builder(action = 'makeinfo --no-split -o $TARGETS $SOURCES', 
           suffix = '.info', 
           src_suffix = '.texi');

makeMan = Builder(action = 'docs/makedoc -man $TARGETS $SOURCES', 
           src_suffix = '._tx');

makeASCII = Builder(action = 'docs/makedoc -ascii $TARGETS $SOURCES', 
           suffix = '.txt', 
           src_suffix = '._tx');

docEnv.Append(BUILDERS = {'DocHTML' : makeHTML,
              'DocRTF' : makeRTF,
              'DocTexi' : makeTexi,
              'DocMan' : makeMan,
              'DocInfo' : makeInfo,
              'DocScite' : makeScite,
              'DocASCII' : makeASCII})

def addRTF(source, target):
    docs.append(docEnv.DocRTF(target,source))

def addHTML(source, target):
    docs.append(docEnv.DocHTML(target,source))

def addTexi(source, target):
    docs.append(docEnv.DocTexi(target,source))

def addInfo(source, target):
    docs.append(docEnv.DocInfo(target,source))

def addMan(source, target):
    docs.append(docEnv.DocMan(target,source))

def addScite(source, target):
    docs.append(docEnv.DocScite(target,source))

def addASCII(source, target):
    docs.append(docEnv.DocASCII(target,source))

addHTML('docs/src/changes._tx', 'docs/html/changes')
addHTML('docs/src/allegro._tx', 'docs/html/allegro.html')
addHTML('docs/src/readme._tx', 'docs/html/readme.html')
addTexi('docs/src/allegro._tx', 'docs/texi/allegro.texi')
addRTF('docs/src/changes._tx', 'docs/rtf/changes.rtf')
addRTF('docs/src/allegro._tx', 'docs/rtf/allegro.rtf')
addRTF('docs/src/readme._tx', 'docs/rtf/readme.rtf')
addMan('docs/src/allegro._tx', 'docs/man/dummyname.3')
addASCII('docs/src/changes._tx', 'docs/txt/changes.txt')
addASCII('docs/src/allegro._tx', 'docs/txt/allegro.txt')
addASCII('docs/src/readme._tx', 'docs/txt/readme.txt')
addScite('docs/src/allegro._tx', 'docs/scite/allegro.api')
addInfo('docs/texi/allegro.texi', 'docs/info/allegro.info')
docs.append(docEnv.Command('CHANGES', 'docs/txt/changes.txt', Copy('$TARGET', '$SOURCE')))
docs.append(docEnv.Command('readme.txt', 'docs/txt/readme.txt', Copy('$TARGET', '$SOURCE')))

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

## For some reason I have to call Program() from this file
## otherwise 'scons/' will be appended to all the sources
## and targets. 

## Build all other miscellaneous targets using the same environment
## that was used to build allegro but only link in liballeg
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
