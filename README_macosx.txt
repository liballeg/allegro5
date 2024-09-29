Mac OS X-specific notes
=======================

Building Allegro on Mac OS X is the same as on other Unix-like operating systems.
See README_make.txt.  

Building with Xcode
-------------------

You may also use Xcode to build Allegro. This works similar to the instructions
in README_make.txt, except add the parameter -GXcode when invoking cmake.

Instead of creating a makefile this will create an Xcode project which you can
double click and open in Xcode and then hit the arrow button to compile.

Using the Clang compiler (OS X 10.6+)
-------------------------------------

It is possible to build Allegro using the Clang compiler that ships with OS
X 10.6 (Snow Leopard). Clang is installed in /Developer/usr/bin. 
To use it, you have to tell Cmake to use Clang instead of gcc. From the
terminal, this is most easily accomplished by the commands

        export PATH=/Developer/usr/bin:$PATH
        export CC=clang

before you run Cmake. If you use the graphical version of Cmake, you will
be given the option of selecting the C compiler to use. Simply select
/Developer/usr/bin/clang.
The installation otherwise continues as usual.

Retina display support (OS X 10.7+)
-----------------------------------

Allegro has an option to support high DPI displays by treating them as a regular
high-resolution display. To do so, use XCode to set the NSHighResolutionCapable
property in the Info.plist of your bundled app to YES. Alternatively, add these
two lines to the Info.plist using a text editor:

    <key>NSHighResolutionCapable</key>
    <true/>

If you are making an unbundled app this feature appears to be enabled by
default, but it is not recommended to rely on this working: make a bundled
app for the most predictable behavior.

Allegro uses a pixel-based coordinate system, meaning that high-DPI displays
will be larger than is reported by the OS. When changing the display DPI or
moving the window between two displays with different DPIs, Allegro displays
behave in the following way:

- If the A5O_DISPLAY was created with the A5O_RESIZABLE flag it will
  send an A5O_DISPLAY_RESIZE event. This will have the effect of your app's
  window remaining visually the same, while the display size in pixels will
  increase or decrease. This is the recommended situation.

- If the A5O_DISPLAY was not created with the A5O_RESIZABLE flag, then
  the display size in pixels will remain the same, but the app's window will
  appear to grow or shrink.
