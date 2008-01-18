AllegroGL - 0.4.4
=================

README file


About the library
~~~~~~~~~~~~~~~~~

The library mostly provides functions to allow you to use OpenGL alongside
Allegro -- you use OpenGL for your rendering to the screen, and Allegro for
miscellaneous tasks like gathering input, doing timers, getting cross-platform
portability, loading data, and drawing your textures.  So this library fills
the same hole that things like glut do.

AllegroGL also automatically exposes most, if not all, OpenGL extensions
available to user programs. This means you no longer have to manually load
them; extension management is already done for you.



License
~~~~~~~

The code is (C) AllegroGL contributors, and double licensed under the GPL and
zlib licenses. See gpl.txt or zlib.txt for details. (You can pick whichever one
you prefer.)


About this document
~~~~~~~~~~~~~~~~~~~

This document contains installation instructions and pointers to
other resources such as the FAQ, reference manual, web page and
mailing list.


General build instructions
--------------------------

From version 0.4.4 onwards, AllegroGL is an addon library for Allegro and the
usual way to build it is by following the allegro compilation procedure. It
will configure, build and install AllegroGL using default options (a shared
release library).

However, you if you need a custom compiled library, e.g. with debug symbols or
a statically linked one, you can build AllegroGL manually, passing custom build
options.


Requirements:
-------------

 General requirement: If you are building AllegroGL separatelly (outside Allegro
 compilation procedure) it will depend on Allegro being successfully built two
 directories above.

 Unix:
	You need an X server which provides the OpenGL/GLX functionality.
	If you have successfully installed 3D drivers (NVidia, DRI drivers, ...)
	then OpenGL/GLX libraries are already installed.
	Otherwise XFree86 4.x has OpenGL/GLX built in. We have also successfully
	used an earlier version, with development snapshots of Mesa3D 3.2 and
	GLX extensions.
	
	You also need to get the GLU library, preferably the SGI's one (see the
	Mesa sourceforge webpage - http://Mesa3D.sf.net/ ).
	
	If you want to build the generic driver you need Mesa 4.0 or higher
	(only the "MesaLib" archive is mandatory, the "MesaDemos" one is
	optionnal).
	
	Links to the relevant sites can be found on the AllegroGL web site.

 Windows/MSVC:
	MSVC6 or MSVC2005 IDE
	or
	GNU make is required for MSVC.

 Windows/Mingw:
	If you use Mingw, you'll need the OpenGL header files and
	libraries. These are normally included with Mingw.
	
	In case your copy of Mingw does not have the OpenGL headers,
	you can grab them here:
	ftp://ftp.microsoft.com/softlib/mslfiles/opengl95.exe
	This is a self-extracting archive.

	You'll also need GNU make (mingw32-make) to compile AllegroGL.

 DOS:
	You need DJGPP and Mesa 4.0 http://Mesa3D.sf.net/ for OpenGL rendering
	(only the "MesaLib" archive is mandatory, the "MesaDemos" one is
	optionnal).

Mac OS X:
	System version 10.1.x or newer is required to build AllegroGL.
	Allegro WIP 4.1.11 or newer is also required, as older versions
	did not support OS X.


Unix instructions
-----------------

For an optimised build, run `./configure' and `make' from the directory
containing this file. This will build the library (in 'lib/unix/') and the
examples (in 'examp/'). Use `make install' to install the library and
header file.  For that part you need write access to /usr/local.

If you want to build the generic driver, run `./configure --enable-generic'
and `make MESADIR=xxx' where xxx is the path to the Mesa 4.0 directory.
This will build both Mesa (GL and GLU) and AllegroGL


For a debug build, add `DEBUGMODE=1' on each of the command lines.


If you get errors about missing header files or libraries,
either for X or GL, see the instructions at the top of
`makefile'.  Note that you need to have the X development
package installed, if you are using Red Hat Linux or Debian
GNU/Linux.



Mingw/32 instructions
---------------------

Mingw 2.0 and higher already come with the OpenGL header files
and libraries, so you can skip the next step.

If you don't have the OpenGL header files (GL/gl.h) and libraries, you will
first need to acquire thrm. These can be obtained from the Microsoft site,
or from MSVC.

If you obtained the self-extracting archive from the Microsoft site, then
run it. Move the produced header files (*.h) into C:\Mingw32\include\GL\
(replace C:\Mingw32 by wherever you happen to have installed Mingw).
Ignore the other files, as they are only useful for MSVC.

You need to set up your environment if you haven't done that already.
Environment variable "PATH" should point to the "bin" directory of Mingw.
You can check that by typing "gcc" in the console. It must display something
like: "gcc: no input files". If it complains about a unknown command then type
"set PATH=%PATH%;C:\Mingw32\bin" (replace C:\Mingw32 by wherever you happen to
have installed Mingw).
Also, you need to set MINGDIR env. variable. It must point to Mingw instalation
directory: "set MINGDIR=C:\Mingw32". You can check that with: "echo %MINGDIR%".

You will need to run 'fix mingw32' in the AllegroGL directory
to update makefile for Mingw32.
Since both Allegro and AllegroGL have native Mingw support
I am happy to say that you can build Allegro/AllegroGL programs
entirely using free software.

For an optimised build, run `make' from the directory containing
this file.  Use `make install' to install the library and header
file. Some versions of Mingw come with `mingw32-make' instead of
`make', so you may need to run that instead.

For a debug build, do the same but write `DEBUGMODE=1' on each
of the command lines; for example, 'make DEBUGMODE=1' and
'make install DEBUGMODE=1'.

Add 'STATICLINK=1' to the last two commands to build AllegroGL that can be
linked to statically linked allegro.




MSVC6/7/8 instructions
------------------

There are two completely different ways of compiling AllegroGL using MSVC. You
can use the old-fashion way decribed bellow (similar to building allegro, using
a GNU makefile and a command line), or you can locate the project file for your
MSVC version in "projects" directory and compile AllegroGL lib and examples with
a few mouse clicks. You'll have to manually copy libs and headers to appropiate
locations.
You can choose between several build configuration, depending against which
Allegro library you want AllegroGL to be linked to. These configurations are:
   _____________________________________________________________
  | Configuration name  |    AGL lib name    | Allegro lib name |
  |-------------------------------------------------------------|
  | Release             | agl.lib            | alleg.lib        |
  | DLL Release         | agl.lib & agl.dll  | alleg.lib        |
  | Static Release      | agl_s.lib          | alleg_s.lib      |
  | Static Release CRT  | agl_s_crt.lib      | alleg_s_crt.lib  |
  | Debug               | agld.lib           | alld.lib         |
  | Static Debug        | agld_s.lib         | alld_s.lib       |
   -------------------------------------------------------------
   
All configuration except "DLL Release" produce AllegroGL library that is
statically linked to the executable.



Instructions for compiling AllegroGL using command line follow.

You must have a working copy of GNU Make (useful for
building Allegro, anyway). This can be either DJGPP's or
Mingw's (recommended).

Step 1:

  The first thing you need to do is find `vcvars32.bat',
  somewhere in your Visual Studio directories (most probably,
  it's in 'vc98/bin'). Running this batch file will enable
  the command line compiler for the current DOS session. If you
  will use it often, or find that typing
  'c:\progra~1\micros~2\vc98\bin\vcvars32.bat' gets annoying after a
  while, then (under Windows 9X) simply add that command to your
  autoexec.bat**

   Note: If at any stage, you get an "Out of Environment space"
   message, then please see the Allegro FAQ for how to fix this.


** The procedure is different for Windows ME and later.
   If you're running Windows ME, you'll need to select "Run" off the
   start menu, then type in "msconfig". Select the environment
   tab. Add the lines inside vcvars32.bat in there by copy/pasting them.
   Reboot your computer for the changes to take effect.

   If you're running Windows 2000/XP (NT?), then open Control Pannel,
   then the "System" applet, then the "Advanced" tab, and
   finally the "Environment" button. Add the environment variables as
   they are in vcvars32.bat. This has to be done manually (yes it's long
   and painful, please redirect all flames to billg@microsoft.com)
   You will need to log off and log back in for the changes to take effect.

Step 2:

  Much like making Allegro, to configure AllegroGL for your compiler please run
  in the command prompt:

       fix.bat msvc6  - for MSVC 6 and older
       fix.bat msvc7  - for MSVC 7 (.NET) and 7.1 (.NET 2003)
       fix.bat msvc8  - for MSVC 8 (.NET 2005)

  Then type `make' to build the library in optimized mode, then `make install'
  to install the library and header files in your MSVC directory.

  For a debugging version, add `DEBUGMODE=1', to the last two command lines.

  To link against a static runtime libraries, rather than the default dynamic
  runtime libraries, add STATICRUNTIME=1,  to the last two command lines.

  To link aginst a static version of allegro, add STATICLINK=1, to the last
  two command lines.
  
  If you are using Mingw for GNU make, then you may need to run `mingw32-make'
  instead of `make'.


DOS instructions
----------------

Please note that DOS support is currently in experimental stages.

Unzip the archive files MesaLib-4.0.zip wherever you want.
Unzip AllegroGL

Create the environment variable MESADIR which defines the Mesa sources path :
	set MESADIR=xxx
where 'xxx' is the path to the Mesa root directory

Go to the root directory of AllegroGL type 'fix djgpp' (without
quotes) followed by 'make'. The GL, GLU  and AllegroGL libraries are built.
Finally type 'make install' to install the library

You're done! You can now use AllegroGL on DOS. Try the example demos...

For a debug build, do the same but write `DEBUGMODE=1' on each
of the command lines.

Note that you can also build :
- The GLUT library (based on AMesa, Allegro and Mesa). Type 'make glut' to build it
  then 'make install-glut' to install it.
- The Mesa samples to test both Mesa and GLUT (note that you need to download
  the MesaDemos archive file to compile mesa samples). Type 'make mesa-samples'
  (Please note that not all the sample programs will be built. Now, as
  of Mesa 3.4.2, most demos are for GLUT which has not yet been completely 
  ported to AMesa/Allegro).



Mac OS X instructions
---------------------

First you will need to run `./fix.sh macosx' in order to prepare the
makefile for Mac OS X. Then `make' will build the library (in `lib/macosx')
and the examples (in `examp/'). Use `sudo make install' to install the
library and the header files; this step will require your system root
password and will install into /usr/local.

For a debug build, add `DEBUGMODE=1' on `make' and `make install' calls,
for example, `make DEBUGMODE=1' and `sudo make install DEBUGMODE=1'.



Further documentation
---------------------


A reference to the functions and macros available to user
programs can be found online at the web site (see below), and is
also downloadable from there in various formats.



More information
----------------


web site: http://allegrogl.sf.net/

    The web site has introductory information, system
    requirements, downloads, and Allegro patches, along with an
    online version of the reference manual.


mailing list: allegrogl-general@lists.sourceforge.net


    All the developers are on this mailing list, and most of the
    discussion here is about future development and bug fixing.
    General questions about using the library make a welcome
    change from the hard work of development, so newcomers are
    very welcome!  If you're not subscribed to the list, make
    this clear when you post, so that people can Cc: their
    replies to you.



Good luck...

- The AllegroGL Team!

