Building Allegro with MSVC
==========================

There are a lot of variations to the build process, but we will just stick with
one to keep things simple.  If you know what you are doing, you can do
something else.

1. Unpack Allegro into a directory *without spaces or other "weird" characters
in the path*.  This is a known problem.

2. Create a build directory under the Allegro directory.  Optionally, create a
deps directory and place the headers and libraries of third party dependencies
in there.  See README_cmake.txt about this.

3. Start the Microsoft Visual Studio IDE, then bring up a command prompt by
clicking "Tools > Visual Studio 20xx Command Prompt".

4. Make sure cmake is in your PATH.  Typing "cmake" should display the help
message.  If it doesn't, you can set the PATH manually by typing
"SET PATH=C:\cmake\bin\;%PATH%" or similar.
Make sure the MSVC compiler cl.exe and lib.exe also run correctly.

5. Go to the Allegro build directory:

        cd \allegro\Build

6. Run CMake to configure everything.  We will use the CMake GUI.

        cmake-gui ..

7. Press "Configure".  Watch for messages about missing headers and libraries.
If anything is missing, you can give CMake a helping hand by setting
_INCLUDE_DIR and _LIBRARY options manually, e.g. if ZLIB is not found you
would set ZLIB_INCLUDE_DIR and ZLIB_LIBRARY.
You may have to switch to "Advanced View" in order to see the variables.
Once done, press "Configure" again.

8. Press "Generate".  CMake will now generate a project solution.

*Note:*
As of the time this is written, CMake has a bug that causes the DLLs in
MSVC 10 to be named incorrectly. To work around this, generate MSVC 9 projects
instead.  You may need to uncomment the line "#define A5O_HAVE_STDINT_H"
in alplatf.h.  Alternatively, use nmake instead of project files.

9. Open up the project solution with the MSVC IDE and start building.



Running the examples
====================

If you build Allegro as a shared library (the default), the example programs
will probably not run as-is, because they cannot find the Allegro DLLs.
You may:

- manually copy the Allegro DLLs into the Build/examples directory where they
  will be found when the example is run; or

- build the INSTALL project, which copies the library files into the MSVC
  search path (%VCINSTALLDIR%\bin).  You may not want to make a mess in there.

- The most drastic solution is to copy them into your C:\WINDOWS\SYSTEM32
  directory, but most people prefer not to make a mess in that directory.

By default, Allegro will load FLAC and Ogg Vorbis DLLs at runtime.  If it
cannot find them, FLAC and Vorbis file format support will be disabled.
Again, the solution is to copy those DLLs where they can be found.



Hints on setting up Visual C++ 2005 Express Edition
===================================================

After installing Visual C++, you need to install the Platform SDK (or Windows
SDK), otherwise CMake will fail at a linking step.  You can do a web install to
avoid downloading a massive file.  For me, installation seemed to take an
inordinately long (half an hour or more), but eventually it completed.
Don't be too quick to hit Cancel.

You also need to install the DirectX SDK.  This is a big download, which I
don't know how to avoid.

Next, you'll need to tell VC++ about the Platform SDK.  Start the IDE.  Under
"Tools > Options > Projects and Solutions > VC++ Directories", add the Platform
SDK executable (bin), include and lib directories to the respective lists.
The DirectX SDK seems to add itself when it's installed.

For debugging, use the DirectX control panel applet to switch to the debugging
runtime.  It's really useful.

