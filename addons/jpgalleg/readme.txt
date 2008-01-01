
         __   _____    ______   ______   ___    ___
        /\ \ /\  _ `\ /\  ___\ /\  _  \ /\_ \  /\_ \
        \ \ \\ \ \L\ \\ \ \__/ \ \ \L\ \\//\ \ \//\ \      __     __
      __ \ \ \\ \  __| \ \ \  __\ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\
     /\ \_\/ / \ \ \/   \ \ \L\ \\ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \
     \ \____//  \ \_\    \ \____/ \ \_\ \_\/\____\/\____\ \____\ \____ \
      \/____/    \/_/     \/___/   \/_/\/_/\/____/\/____/\/____/\/___L\ \
                                                                  /\____/
                                                                  \_/__/

                  Version 2.6, by Angelo Mottola, 2000-2006
             (build system modified by Allegro development team)



Thanks for downloading JPGalleg! This add-on for Allegro will allow you to
load/save JPG images using standard Allegro image handling functions.
Its main advantages are:


* Supports baseline and progressive JPG decoding, and baseline JPG encoding.
  This should ensure it'll load almost any JPG image you'll throw at it

* Full Allegro integration: use load_bitmap() to load JPGs as if they were
  BMPs or other Allegro supported image formats

* Fast encoding/decoding

* Allegro datafile support: store JPGs in datafiles using the Grabber (plugin
  included), then load your datafiles from your programs and use JPG objects
  as if they were normal BITMAPs

* Encoding/decoding work on both files and memory data

* Small footprint: compiled library weights only 40K and is self-contained,
  in that it doesn't depend on external libraries other than Allegro


It does not support arithmetic coding, but this shouldn't be a problem at all
as most JPG don't use it as it's a patented type of encoding. Standard
baseline and progressive formats are fully supported; if you fail to load a
particular JPG image, please let me know!

JPGalleg needs Allegro 4.0 or newer to run, and should work on any platform
supported by Allegro, even if it has only been successfully tested under
Windows (MinGW32), Linux and MacOS X.



Install instructions
--------------------

JPGalleg is shipped in source code form, so you need to compile it first in
order to use it. Supported compilers are DJGPP (DOS), MinGW32 (Windows), MS
Visual C++ (6.0 or newer) and gcc (Linux, BeOS, MacOS X).
If you're willing to use Visual C++, you need GNU gcc and GNU make in order
to install; you can grab a working copy of these for Win32 from the downloads
section of the MinGW32 project site:

   http://www.mingw.org
   
To install, the steps to follow are almost the same under all platforms:


If you're under DOS/Windows:
   fix.bat <platform>

If you're under an Unix variant:
   ./fix.sh <platform>


Where <platform> must be one of "djgpp", "mingw32", "msvc", "unix", "beos"
or "macosx".
This will configure JPGalleg to be compiled for the selected environment.
Next you just have to start the build process:


   make


Under platforms that supports it, this will assume a dynamic version of Allegro
to be installed on the system; if you have a static Allegro, you need to use:


   make STATICLINK=1


This will build a version of JPGalleg specifically made to be linked with the
static version of Allegro.
You can also build a special debugging version of the library instead, by
issueing:


   make DEBUG=1


The debugging version will output lots of messages to stderr, and it could
be useful when dealing with a JPG image JPGalleg fails to load.
In any case, at the end of the build process you should get the compiled
library (in lib/libjpgal.a or lib/libjpgal.lib if you built the normal
library, in lib/libjpgad.a or lib/libjpgad.lib if you built the debug one;
the same but with a "_s" suffix if you built with STATICLINK=1); you need to
install it into your system so that your compiler will know where to look
for it when you link against it (warning: this step requires root permissions
under Unix and MacOS X):


   make install


Congratulations! You have now successfully installed JPGalleg, and you are
ready to use it in your own programs. See the next section "Using JPGalleg"
for details.
You may also want to install the JPGalleg Grabber plugin; this will let you
manage JPG images easily from the Allegro Grabber utility, so you can store
and use JPGs in datafiles. You need to have previously installed JPGalleg,
and you also have to set the ALLEGRO environmental variable before being able
to install the plugin; this variable must contain the full path to your local
Allegro directory. If you don't have it set yet, proceed this way:


If you're under DOS/Windows:
   set ALLEGRO=C:\ALLEGRO
If you're under an Unix variant:
   export ALLEGRO=~/allegro


supposing you previously installed Allegro C:\ALLEGRO (for DOS/Windows) or
into ~/allegro (for Unix-like systems).
Once you have the ALLEGRO environmental variable set, just type


   make plugin


to install the plugin. This will rebuild the Grabber that will now support
JPG images natively.
For more info on the plugin, see section "Grabber plugin" below.



Using JPGalleg
--------------

JPGalleg is a library, and as such you need to link against it every time you
want to use its functionalities. If you have successfully installed the
package, the library libjpgal.a is already available for your compiler to use;
you just have to specify the "-ljpgal" command line link option when linking
your programs (on gcc based platforms); for example:


   gcc myprog.c -o myprog.exe -lalleg -ljpgal


On MSVC, you need to add libjpgal.lib to the list of libraries linked to
your project.
You also need to include "jpgalleg.h" at the beginning of your source files,
in order to use the library functions:


   #include <jpgalleg.h>


See next section for a reference of the JPGalleg functions.



Functions reference
-------------------

int jpgalleg_init()

   Parameters:
      none

   Returns 0 on success, -1 on error.
   
   
   This function initializes the JPGalleg library, by registering the JPG file
   format with the Allegro image saving/loading and datafile subsystems. You
   don't really need to call this before calling any other library functions,
   but if you do, load_bitmap() and save_bitmap() will recognize files with
   the JPG extension, and datafiles holding JPEG objects will automatically
   decode them on datafile load, meaning you'll be able to work with them as
   if they were normal Allegro BITMAPs.
   Note that the invokation of this function automatically causes the static
   linkage of both the JPG encoder and decoder code to your executables; by
   not calling it you may loose full Allegro integration but you'd save EXE
   size if you only use one of either the encoder or the decoder of JPGalleg.
   Currently this function always returns 0.


______________________________________________________________________________

const char *jpgalleg_error_string()

   Parameters:
      none

   Returns the latest error message string.
   
   
   This utility function returns a string briefly describing the latest error
   occured during the encoding/decoding process, if any. See the "Error codes"
   section for more details.

______________________________________________________________________________

BITMAP *load_jpg(AL_CONST char *filename,
                 RGB *palette)

   Parameters:
      filename          Name of the JPG file to be loaded
      palette           PALETTE structure that will hold the JPG palette
   
   Returns the loaded image into a BITMAP structure, or NULL on error.
   
   
   Similar to the other Allegro image loading functions, this is a shortcut
   version of load_jpg_ex(), specifying no progress callback.

______________________________________________________________________________

BITMAP *load_jpg_ex(AL_CONST char *filename,
                    RGB palette,
		    void (*callback)(int progress))

   Parameters:
      filename          Name of the JPG file to be loaded
      palette           PALETTE structure that will hold the JPG palette
      callback          Progress callback (see below)
   
   Returns the loaded image into a BITMAP structure, or NULL on error.
   
   
   load_jpg() loads a JPG image from a file and returns it in a BITMAP; it is
   similar to the other Allegro image loading functions. The returned bitmap
   will be 24 or 8 bit, depending on if the loaded JPG stores a truecolor or a
   greyscale image. Allegro color conversion rules apply here, so pay attention
   as if the COLORCONV_TOTAL flag is set (as is by default), the image will
   be automatically converted to the current color depth before being given
   to the user (see Allegro documentation of function set_color_conversion()
   for details).
   If image is in greyscale, `palette' will be filled with a greyscale palette,
   otherwise the generate_optimized_palette() or generate_332_palette() Allegro
   functions will be used to build it depending on if the current gfx mode is 8
   bit or not. You can also specify NULL in this parameter, in which case no
   palette is returned.
   The callback parameter can be used to specify a callback function to be used
   as progress indicator. The function must return void and accept only one
   parameter, an integer, that will range 0-100 specifying the percentage of
   progress achieved at call time. Pay attention since the number of times this
   callback gets called varies; it can be called up to several hundreds times,
   and thus receive the same percentage value for more than one call.
   On error, this function returns NULL and sets the jpgalleg_error global
   variable to the proper error code (see the "Error codes" section).

______________________________________________________________________________

BITMAP *load_memory_jpg(void *buffer,
                        int size,
                        RGB *palette)

   Parameters:
      buffer            Pointer to a block of memory that holds the JPG data
      size              Size of the memory block
      palette           PALETTE structure that will hold the JPG palette
   
   Returns the loaded image into a BITMAP structure, or NULL on error.
   
   
   This function is a shortcut version of load_memory_jpg_ex(), specifying no
   progress callback.

______________________________________________________________________________

BITMAP *load_memory_jpg_ex(void *buffer,
                           int size,
                           RGB *palette,
			   void (*callback)(int progress))

   Parameters:
      buffer            Pointer to a block of memory that holds the JPG data
      size              Size of the memory block
      palette           PALETTE structure that will hold the JPG palette
      callback          Progress callback (see load_jpg_ex())
   
   Returns the loaded image into a BITMAP structure, or NULL on error.
   
   
   This function behaves exactly as load_jpg(), but tries to decode a JPG
   image held into a memory block. The memory block size is needed to ensure
   the JPG data integrity.

______________________________________________________________________________

int save_jpg(AL_CONST char *filename,
             BITMAP *image,
             AL_CONST RGB *palette)

   Parameters:
      filename          Name of the file that will hold the JPG image
      image             The BITMAP structure holding the image data to save
      palette           PALETTE structure holding the image palette
   
   Returns 0 on success, -1 on error.
   
   
   Similar to the other Allegro image saving functions, this is a shortcut
   version of save_jpg_ex(), saving the image with a default quality of 75,
   no subsampling, no optimization and no progress callback.

______________________________________________________________________________

int save_jpg_ex(AL_CONST char *filename,
                BITMAP *image,
                AL_CONST RGB *palette,
                int quality,
                int flags,
		void (*callback)(int progress))

   Parameters:
      filename          Name of the file that will hold the JPG image
      image             The BITMAP structure holding the image data to save
      palette           PALETTE structure holding the image palette
      quality           JPG encoding quality, in the range 1-100
      flags             Miscellaneous encoding flags (see below)
      callback		Progress callback (see load_jpg_ex())
   
   Returns 0 on success, -1 on error.
   
   
   Saves an image held in an Allegro BITMAP structure into a JPG file, using
   baseline encoding. The image can be any color depth, but the saved JPG will
   always be a 24 bit truecolor image (or 8 bit if saving in greyscale).
   Quality ranges from 1 (very low) to 100 (very high) (other values are
   clamped in this range); a value of 90 already gives an image that is hardly
   distinguishable from the original one. Obviously higher quality means bigger
   file size.
   JPGalleg supports chrominance subsampling; that is, while luminance (which
   specify the brightness of each pixel) is always sampled once for every
   image pixel, chrominance (which is used to specify the chromatic components
   of each pixel) can be sampled from every image pixel (no subsampling, also
   named mode 444), only on even pixels horizontally (mode 422), or only on
   even pixels both horizontally and vertically (mode 411). The mode name
   already tells you the subsampling type: for example 422 means that for
   4 luminance samples, only 2 chrominance samples are taken.
   Subsampling helps reducing the final JPG file size and is hardly noticeable
   by the human eye, expecially if you use high qualities. A good compromise
   in quality/size settings is for example a quality of 90 and a subsampling
   mode 411.
   You specify the subsampling mode in the `flags' parameter to this function,
   by setting one of the following constants (defined in jpgalleg.h):
   
      JPG_SAMPLING_444      For mode 444 (no subsampling)
      JPG_SAMPLING_422      For mode 422 (horizontal subsampling)
      JPG_SAMPLING_411      For mode 411 (horizontal and vertical subsampling)
   
   The flags parameter can accept more options, chained together by a bitwise
   OR. If you pass the JPG_GREYSCALE constant, the image will be saved in
   greyscale mode; only the luminance component of the image will be saved,
   therefore any specified subsampling of the chrominance is ignored.
   Passing JPG_OPTIMIZE to the flags parameter will activate optimized
   encoding; the routine will take twice the time to complete, but the saved
   file size will be reduced by an average 10% compared to unoptimized mode.
   The final function parameter can be used to specify a callback for progress
   indication, and works the same as seen in load_jpg_ex().
   This function returns 0 on success, or -1 on error. If an error occured,
   you can get more details on it by checking the jpgalleg_error global
   variable. See the "Error codes" section for more details.

______________________________________________________________________________

int save_memory_jpg(void *buffer,
                    int *size,
                    BITMAP *image,
                    AL_CONST RGB *palette)
   
   Parameters:
      buffer            Pointer to a memory buffer that will hold the JPG data
      size              Size of the memory buffer (see save_memory_jpg_ex())
      image             The BITMAP structure holding the image data to save
      palette           PALETTE structure holding the image palette
   
   Returns 0 on success, -1 on error.
   
   
   This is a shortcut version of save_memory_jpg_ex(), saving the image with a
   default quality of 75, no subsampling, no optimization and no progress
   callback.

______________________________________________________________________________

int save_memory_jpg_ex(void *buffer,
                       int *size,
                       BITMAP *image,
                       AL_CONST RGB *palette,
                       int quality,
                       int flags,
		       void (*callback)(int progress))
   
   Parameters:
      buffer            Pointer to a memory buffer that will hold the JPG data
      size              Size of the memory buffer (see below)
      image             The BITMAP structure holding the image data to save
      palette           PALETTE structure holding the image palette
      quality           JPG encoding quality, in the range 1-100
      flags             Miscellaneous encoding flags (see save_jpg_ex())
      callback          Progress callback (see load_jpg_ex())
   
   Returns 0 on success, -1 on error.
   
   
   This is the same as save_jpg_ex(), but encodes the image in a memory block
   rather than into a file. The `size' parameter has a double function: on
   input, you must set it to the size of the memory buffer that will hold the
   encoded JPG data (this ensures the function will fail if the buffer is too
   small to hold the encoded JPG), on output it'll hold the final size of the
   encoded JPG data in bytes.

______________________________________________________________________________



Error codes
-----------

All the JPGalleg functions set the jpgalleg_error global variable to a proper
error code on error, so if a function fails, you can check this variable for
more details on what went wrong. If you want a more human-readable error
message, the jpgalleg_error_string() function can be helpful, returning a
string briefly describing the last error occured.
Here's the list of possible numerical error codes (defined in jpgalleg.h):

   JPG_ERROR_NONE
      No error occured during last operation.
   
   JPG_ERROR_READING_FILE
      There was an I/O error reading from a file.
   
   JPG_ERROR_WRITING_FILE
      There was an I/O error writing to a file.
   
   JPG_ERROR_INPUT_BUFFER_TOO_SMALL
      The memory buffer holding the encoded JPG data is too small for the JPG
      itself. The JPG is truncated and can't be loaded.
   
   JPG_ERROR_OUTPUT_BUFFER_TOO_SMALL
      The buffer specified when saving a JPG image is too small to hold the
      encoded JPG data.
   
   JPG_ERROR_HUFFMAN
      An huffman encoding error has occured while saving a JPG. This should
      never happen and is considered an internal error, so if you ever get
      this error, please report it to me!
   
   JPG_ERROR_NOT_JPEG
      The file/data being loaded isn't recognized as a valid JPG image.
   
   JPG_ERROR_UNSUPPORTED_ENCODING
      The JPG file/data being loaded is encoded in a format not supported by
      JPGalleg. Currently the library can only load baseline and progressive
      encoded JPG images.
   
   JPG_ERROR_UNSUPPORTED_COLOR_SPACE
      JPGalleg supports only greyscale and luminance/chrominance JPG images.
      If the JPG being loaded isn't encoded in one of these color spaces, this
      error occurs.
   
   JPG_ERROR_UNSUPPORTED_DATA_PRECISION
      JPGalleg only supports 8 bit data precision. If the JPG being loaded is
      not encoded using this precision, this error occurs.
   
   JPG_ERROR_BAD_IMAGE
      The JPG file/data being loaded is corrupted.
   
   JPG_ERROR_OUT_OF_MEMORY
      JPGalleg ran out of memory while decoding a JPG image.



Grabber plugin
--------------

JPGalleg ships with a plugin for the Allegro Grabber utility. This allows the
Grabber to easily handle JPG images into datafiles, so you can store your JPGs
and later load the datafile and see them as normal Allegro BITMAPs.
For informations on how to install the Grabber plugin, see section "Install
instructions". Alternatively, you can copy the whole contents of the
jpgalleg/plugin directory into your allegro/tools/plugins directory, and then
recompile the grabber by calling

   make tools/grabber

from your root Allegro directory. Be sure you already successfully installed
JPGalleg before installing the plugin, otherwise the plugin will not compile!
Once you have a JPG-enabled Grabber, you can create and grab new data objects
of type "JPEG"; these will hold the compressed JPG data of your images.
To create a new JPEG object, use New -> JPG image. A new temp 32x32 JPG image
will be created and assigned to the new object, as if you created a normal
bitmap object. You can now use the grab command on that object to load any
Allegro supported image into it; obviously you can now grab JPG images, but
you can also grab the other Allegro supported image formats, which will be
converted to JPG before being assigned to the JPEG object. Conversion to JPG
only occurs if the selected image isn't already in JPG format, and by default
the conversion will use a quality of 75 and no subsampling.
You can alter these settings at any time by using the dialog which appears
when you select the menu item File -> JPG import settings. Beware though that
the new settings will only apply on the next image conversions, not to images
already imported.

When you load a datafile holding JPEG type objects, these will automatically
be decoded into normal BITMAP objects at datafile load time. This means that
if you have stored a JPEG object named "MY_JPG_IMAGE" into a datafile, after
you've loaded the datafile into `data', you'll be able to call:

   BITMAP *bmp = data[MY_JPG_IMAGE].dat;
   blit(bmp, screen, 0, 0, 0, 0, bmp->w, bmp->h);

As if MY_JPG_IMAGE was a normal BITMAP (and infact it is, after you've loaded
the datafile).
A final hint: when storing JPGs into your datafiles, do not use compression,
otherwise it is likely your datafile will be even bigger than the uncompressed
version...



Frequently Asked Questions
--------------------------

Q) I get a "undefined reference to: 'register_datafile_object'" message when
   linking my program, while the examples shipped with the library compiled
   nicely. What's wrong?

A) Try swapping the order with which you link Allegro and JPGalleg to your
   program. This should solve the problem.


Q) My JPG image won't load! What can I do?

A) That JPG is probably not a standard baseline or progressive encoded JPG.
   JPGalleg can't load arithmetic encoded JPGs; these are rare, but it could
   be your case. In order to load it, you first need to convert it to baseline
   or progressive format; most paint programs will support at least one of
   them. The encoding format is usually selected in the same dialog in which
   you set the JPG quality when saving.
   If your image is already a baseline or progressive JPG and it won't still
   load, please contact me and send me the image; I'll try to spot the bug and
   maybe fix it in the next JPGalleg release.


Q) Why isn't arithmetic encoding supported?

A) Arithmetic encoding uses a patented algorithm, so it is unlikely it'll ever
   be supported in a free library like JPGalleg.
   

Q) Why JPGs saved by JPGalleg are bigger than (my paint program) generated
   JPGs?

A) Saving a JPG is a complex task, and the compression relies on several
   factors. The quality factor as an example, is implementation dependent, and
   is therefore different from the one used by other programs; a JPGalleg
   quality of 90 may correspond to a completely different quality elsewhere.
   You should also check subsampling: activating it will help reducing final
   file size a lot. And you can also use the JPG_OPTIMIZE flag to generate
   optimized huffman encoding tables that will help reducing size by another
   average 10%.


Q) Why can't I save JPGs using progressive encoding?

A) In order to be compact, JPGalleg only supports saving in baseline encoding.
   Adding support for saving in progressive mode too would add weight to the
   lib, and IMHO it's not really worth it, and behind the scope of this
   library.



Versions history
----------------

Version 2.6 (May 2006)
    * Added more JFIF variants compatibility
    * Fixed 1-byte memory leak in the decoder
    * Made compatible with Allegro 4.2.x
    * Refactored source code to avoid linking decoder or encoder functions
      when one of these is not used by the user application
    * Introduced a new jpgalleg_error_string() function to return the last
      error message in string form
    * Small makefile fixes
    * Added Pyjpgalleg 1.0 in the contrib directory, a Python wrapper library
      for JPGalleg by Grzegorz Adam Hankiewicz

Version 2.5 (July 2004)
    * Now directly works on and returns a 24 bit image when dealing with
      truecolor JPGs, instead of using an intermediate 32 bpp buffer
    * New faster YCbCr <--> RGB MMX routines
    * Refactored I/O subsystem, now always works in memory internally
    * Optimized bit stream fetching resulting in big performance bump
    * Reduced memory footprint
    * Fixed a possible memory leak in the decoder

Version 2.4 (June 2004)
    * Changed the decoder IDCT algorithm to AAN, ensuring faster decoding
    * Added an experimental MMX IDCT routine
    * Small optimizations on other parts of the decoder
    * Fixed a bug in the decoder not recognizing some EXIF images
    * Tightened a bit the encoder code
    * Reworked build system to use different target directories on different
      platforms
    * Added a small benchmark utility
    
Version 2.3 (March 2004)
    * Fixed output colors bug when decoding JPGs while in 32 bits mode
    * Small optimization to the decoder
    * Refactored encoder code, enhanced compactness
    * Added progress callback functionality to both decoder and encoder
    * Enhanced errors reporting
    * Reworked encoder quantization resulting in big output size reduction at
      the same quality; also changed the quality curve formula
    * Made ex5 to report expected size of file when saving JPGs and to show
      progress when loading/saving
    * Added a license

Version 2.2 (February 2004)
    * Updated codebase to be compatible with latest Allegro WIPs
    * Added debug capabilities to the lib
    * Added optimized huffman tables generation to encoder
    * Added support for loading JPGs with a 422 subsampling mode with an 8x16
      MCU (previously supported only 16x8)
    * Can now load EXIF JPG images
    * Now loaded images are either 8 (greyscale) or 24 (truecolor) bit, rather
      than always 32 bit
    * Fixed loading of JPGs with a restart interval of 0
    * Fixed little endianess related issues
    * Reworked make system, added MS Visual C++ and MacOS X support, added a
      "plugin" target
    * Replaced example images with smaller ones
    * Fixed some small bugs in the grabber plugin

Version 2.1 (May 2003)
    * Small bugfixes to the library and the Grabber plugin
    * Optimized the decoder
    * Now supports progressive JPG decoding
    * The encoder has a new better quality curve formula
    * Updated the docs and added a FAQ

Version 2.0 (April 2003)
    * Complete rewrite of the library
    * Faster, more baseline JPG compliant decoder
    * Added a baseline JPG encoder
    * Uses dedicated MMX routines for YCbCr <--> RGB color space conversion
      when available
    * Proper Allegro datafile support: decodes JPG objects on datafile load
    * Added a Grabber plugin
    * New examples
    * Uses a dedicated makefile for each supported platform
    * Now compiles as a standalone library

Version 1.2 (March 2002)
    * Faster decoding by using fixed point math in the YCbCR -> RGB color
      space conversion.
    * Fixed compatibility with C++
    * Modified makefile to support multiple platforms.

Version 1.1 (June 2000)
    * Added support for loading JPGs from memory
    * New examples with better test images.

Version 1.0 (May 2000)
    * First public release, supports baseline decoding from JPG files.



Contact informations
--------------------

Got some questions about JPGalleg? Do you want to provide some feedback in
general? Feel free to contact me at the following email address:

                            a.mottola@gmail.com

Also, be sure to check for updates to JPGalleg at the official project page:

                    http://www.ecplusplus.com/jpgalleg



Thanks
------

* Shawn Hargreaves and the many others who contributed to the excellent
  Allegro library; keep on rockin'!
* The Independent JPEG Group: the fast DCT algorithm used here comes from
  their great jpeg codec package.
* Cristi Cuturicu for his JPEG compression and file format FAQ. Without your
  document, my work would have never been possible.
* David Wang and Vincent Penecherch for the patches for supporting 64bit CPUs
  and for Allegro 4.2.x compatibility respectively.
* Grzegorz Adam Hankiewicz for Pyjpgalleg, a nice Python wrapper for JPGalleg.
