Pyjpgalleg, 1.0 by Grzegorz Adam Hankiewicz, 2006.
--------------------------------------------------

I wrote this simple wrapper following python's extension tutorial
because I wanted to see how much I could improve the speed of loading
jpg files from my python image viewer.  The viewer is written in
pure python with alpy (http://pyallegro.sf.net/), and depending on
the system configuration it has three different loading modes.

1) It can use the external unix program convert to transform the
   input into a bmp which Allegro can read. This is really slow.

2) If PIL (Python Imaging Library) is available, the input file is
   loaded with it (really good C implementation), and then converted
   into an Allegro bitmap with alpy's Bitmap.from_string (quite fast
   memory copy conversion).

3) If pyjpgalleg is available, the viewer uses Allegro' load_bitmap,
   which makes the image available immediatelly to Allegro, without
   any extra conversion (fastest method).

I tested the speed of these three methods with a picture I took
with my digital camera, a 3.4 MB JPG, sized 2560x1920. Here are the
averaged speeds for each of the methods after loading and showing
the image ten times on my PIII 800Mhz:

    1) 6.3023s      2) 3.8314s      3) 3.3937s

So pyjpgalleg is about 12% faster than PIL + memory conversion.


Usage instructions:
-------------------

Pretty easy, you run the typical "python setup.py install" and
there you go, now you can do:

    import alpy
    alpy.init()
    import pyjpgalleg
    pyjpgalleg.init()

and there you go, load jpg files with alpy.load_bitmap().

I've tested this only under Linux, but I guess this could work as
is under Windows too.

Enjoy!


Download urls:
--------------

 jpgalleg, by Angelo Mottola, from http://www.ecplusplus.com/
 or redirected from http://www.allegro.cc/depot/project.php?_id=522.
 You should be getting this pyjpgalleg from the jpgalleg package
 in the contrib directory.

