     ______   ___    ___
    /\  _  \ /\_ \  /\_ \
    \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
     \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
      \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
       \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
	\/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
				       /\____/
				       \_/__/     Version 4.0.0


		A game programming library.

	     By Shawn Hargreaves, Dec 10, 2001.

		See the AUTHORS file for a
	       complete list of contributors.



#include <std_disclaimer.h>

   "I do not accept responsibility for any effects, adverse or otherwise, 
    that this code may have on you, your computer, your sanity, your dog, 
    and anything else that you can think of. Use it at your own risk."



======================================
============ Introduction ============
======================================

   Allegro is a cross-platform library intended for use in computer games 
   and other types of multimedia programming. It was initially conceived on 
   the Atari ST, but that platform sadly died during childbirth. After a 
   brief stay with Borland C, it was adopted by the fantastic djgpp 
   compiler, where it grew to maturity. In the fullness of time it gave 
   birth to children of its own, who went to live in such exotic locations 
   as DirectX and the X Server, but the entire family is now back together 
   again, living in harmony as a single portable entity. How about that for 
   a mixture of metaphors? :-)

   A wide range of extension packages and add-on modules are also available, 
   which can be found in the "Library Extensions" section of the Allegro.cc
   website, http://www.allegro.cc/.

   According to the Oxford Companion to Music, Allegro is the Italian for 
   "quick, lively, bright". It is also a recursive acronym which stands for 
   "Allegro Low Level Game Routines".



=============================================
============ Supported platforms ============
=============================================

   For instructions on how to install Allegro, how to link your programs 
   with it, and any additional information specific to each of the supported 
   platforms, see one of the files:

   DOS/djgpp         - see docs/build/djgpp.txt
   DOS/Watcom        - see docs/build/watcom.txt
   Windows/MSVC      - see docs/build/msvc.txt
   Windows/MinGW32   - see docs/build/mingw32.txt
   Windows/Cygwin    - see docs/build/mingw32.txt
   Windows/Borland   - see docs/build/bcc32.txt
   Linux (console)   - see docs/build/linux.txt
   Unix (X)          - see docs/build/unix.txt
   BeOS              - see docs/build/beos.txt
   QNX               - see docs/build/qnx.txt
   MacOS/MPW         - see docs/build/macos.txt

   General API information can be found in docs/txt/allegro.txt, which is
   also available in HTML, TexInfo, and RTF format in the docs directory.



==================================
============ Features ============
==================================

   Cross-platform support for DOS, Windows, Unix, BeOS, QNX and MacOS systems.

   Drawing functions including putpixel, getpixel, lines, rectangles, flat 
   shaded, gouraud shaded, texture mapped, and z-buffered polygons, circles,
   floodfill, bezier splines, patterned fills, masked, run length encoded,
   and compiled sprites, blitting, bitmap scaling and rotation,
   translucency/lighting, and text output with proportional fonts. Supports
   clipping, and can draw directly to the screen or to memory bitmaps of any
   size.

   DOS graphics drivers for VGA mode 13h, mode-X (twenty three tweaked VGA 
   resolutions plus unchained 640x400 Xtended mode), and SVGA modes with 8, 
   15, 16, 24, and 32 bit color depths, taking full advantage of VBE 2.0 
   linear framebuffers and the VBE/AF hardware accelerator API if they are 
   available. Additional video hardware support is available from the 
   FreeBE/AF project (http://www.talula.demon.co.uk/freebe/).

   Windows graphics drivers using DirectX in fullscreen and windowed modes, 
   plus routines for drawing onto GDI device contexts.

   Unix graphics drivers for X, DGA, fbcon, SVGAlib, VBE/AF, mode-X, and
   standard VGA.

   BeOS graphics drivers using BWindowScreen in fullscreen and BDirectWindow
   in windowed modes.

   Hardware scrolling and triple buffering (where available), mode-X split 
   screens, and palette manipulation.

   FLI/FLC animation player.

   Plays background MIDI music and up to 64 simultaneous sound effects, and 
   can record sample waveforms and MIDI input. Samples can be looped 
   (forwards, backwards, or bidirectionally), and the volume, pan, pitch, 
   etc, can be adjusted while they are playing. The MIDI player responds to 
   note on, note off, main volume, pan, pitch bend, and program change 
   messages, using the General MIDI patch set and drum mappings. DOS version 
   currently supports Adlib, SB, SB Pro, SB16, AWE32, MPU-401, ESS 
   AudioDrive, Ensoniq Soundscape, and Windows Sound System. Windows version 
   supports WaveOut and DirectSound interfaces and the system MIDI drivers. 
   Unix version supports OSS, ESD, and ALSA sound drivers. BeOS version
   supports BSoundPlayer and BMidiSynth interfaces. All versions provide
   software wavetable MIDI playback.

   Easy access to the mouse, keyboard, joystick, and high resolution timer 
   interrupts, including a vertical retrace interrupt simulator in the DOS 
   version.

   Routines for reading and writing LZSS compressed files.

   Multi-object data files and a grabber utility.

   Math functions including fixed point arithmetic, lookup table trig, and 
   3d vector/matrix/quaternion manipulation.

   GUI dialog manager and file selector.

   Built-in support for 16-bit and UTF-8 format Unicode characters.



===================================
============ Copyright ============
===================================

   Allegro is gift-ware. It was created by a number of people working in 
   cooperation, and is given to you freely as a gift. You may use, modify, 
   redistribute, and generally hack it about in any way you like, and you do 
   not have to give us anything in return. However, if you like this product 
   you are encouraged to thank us by making a return gift to the Allegro 
   community. This could be by writing an add-on package, providing a useful 
   bug report, making an improvement to the library, or perhaps just 
   releasing the sources of your program so that other people can learn from 
   them. If you redistribute parts of this code or make a game using it, it 
   would be nice if you mentioned Allegro somewhere in the credits, but you 
   are not required to do this. We trust you not to abuse our generosity.



=======================================
============ Configuration ============
=======================================

   Allegro reads information about your hardware from a file called 
   allegro.cfg. If this file doesn't exist it will autodetect (ie. guess :-) 
   You can write your config file by hand with a text editor, or you can use 
   the setup utility program.

   Normally the setup program and allegro.cfg will go in the same directory 
   as the Allegro program they are controlling. This is fine for the end 
   user, but it can be a pain for a programmer using Allegro because you may 
   have several programs in different directories and want to use a single 
   allegro.cfg for all of them. If this is the case you can set the 
   environment variable ALLEGRO to the directory containing your 
   allegro.cfg, and Allegro will look there if there is no allegro.cfg in 
   the current directory.

   The mapping tables used to store different keyboard layouts are stored in 
   a file called keyboard.dat. This must either be located in the same 
   directory as your Allegro program, or in the directory pointed to by the 
   ALLEGRO environment variable. If you want to support different 
   international keyboard layouts, you must distribute a copy of 
   keyboard.dat along with your program.

   Various translations of things like the system error messages are stored 
   in a file called language.dat. This must either be located in the same 
   directory as your Allegro program, or in the directory pointed to by the 
   ALLEGRO environment variable. If you want to support non-English versions 
   of these strings, you must distribute a copy of language.dat along with 
   your program.

   Under Unix and BeOS, the config file routines also check for ~/allegro.cfg, 
   ~/.allegrorc, /etc/allegro.cfg, and /etc/allegrorc, in that order, and 
   the keyboard and language files can be stored in your home directory or 
   in /etc/.

   See docs/txt/allegro.txt for details of the config file format.



===========================================
============ Notes about sound ============
===========================================

   The DIGMID wavetable driver uses standard GUS format .pat files, and you 
   will need a collection of such instruments before you can use it. This 
   can either be in the standard GUS format (a set of .pat files and a 
   default.cfg index), or a patches.dat file as produced by the pat2dat 
   utility. You can also use pat2dat to convert AWE32 SoundFont banks into 
   the patches.dat format, and if you list some MIDI files on the command 
   line it will filter the sample set to only include the instruments that 
   are actually used by those tunes, so it can be useful for getting rid of 
   unused instruments when you are preparing to distribute a game. See the 
   Allegro website for some links to suitable sample sets.

   The DIGMID driver normally only loads the patches needed for each song 
   when the tune is first played. This reduces the memory usage, but can 
   result in a longish delay the first time you play each MIDI file. If you 
   prefer to load the entire patch set in one go, call the 
   load_midi_patches() function.

   The CPU sample mixing code can support between 1 and 64 voices, going up 
   in powers of two (ie. either 1, 2, 4, 8, 16, 32, or 64 channels). By 
   default it provides 8 digital voices, or 8 digital plus 24 MIDI voices (a 
   total of 32) if the DIGMID driver is in use. But the more voices, the 
   lower the output volume and quality, so you may wish to change this by 
   calling the reserve_voices() function or setting the digi_voices and 
   midi_voices parameters in allegro.cfg.



======================================
============ Contact info ============
======================================

   The latest version of Allegro can always be found on the Allegro 
   homepage, http://alleg.sourceforge.net/.

   There are three mailing lists for Allegro-related discussion, each with a 
   slightly different purpose.

   [AL] - Allegro - allegro@canvaslink.com.

      This list is for any kind of discussion about Allegro, questions about 
      Allegro, reports of problems with Allegro, suggestions for new 
      features, announcements of programs written using Allegro, etc. 
      General rule: if it has to do with Allegro, you can post it here. If 
      not, go somewhere else (for example comp.os.msdos.djgpp, comp.lang.c, 
      or the AGP list).

   [AGP] - Allegro Games Programming - agp@canvaslink.com.

      This list is for discussion of the more general kinds of issue that 
      may arise while writing a game using Allegro, but that are not 
      specifically related to Allegro itself. For example if you wanted to 
      talk about algorithms for scrolling a tile map, that would be 
      off-topic in the main Allegro list, but appropriate to discuss here.

   [AD] - Allegro Developers - conductors@canvaslink.com.

      This list is for the people currently working on Allegro, who use it to
      coordinate their efforts. You can use this address if you need to 
      contact the developers directly, for example to submit some new code 
      that you have written or to report a bug. Unlike the other lists, we 
      will be really rude to people who post inappropriate material here, so 
      please don't do that! Do not send tech-support questions to this list. 
      Don't post bug reports here unless you are 100% certain they aren't 
      your fault (if you are in any doubt, use the main Allegro list 
      instead, which is read by most of the same people who are subscribed 
      here). The Allegro development is a completely open process, and 
      everyone is welcome to drop by, have a listen, and start contributing 
      code patches. This list is for working rather than talking, though, so 
      please don't do anything that might get in our way.

   To subscribe to one of the lists, write to listserv@canvaslink.com with 
   the text "subscribe {list} myname" in the body of your message, where 
   {list} is one of allegro, agp, or conductors.

   To remove yourself from a list, write to listserv@canvaslink.com with the 
   text "unsubscribe {list}" in the body of your message, where {list} is 
   one of allegro, agp, or conductors.

   Note that the address for subscribing and unsubscribing is NOT the same 
   as the regular address for posting messages! Please remember to send your 
   administration requests to the listserv, rather than to the list itself.

   To send a message to one of the lists, write to allegro@canvaslink.com, 
   agp@canvaslink.com, or conductors@canvaslink.com. You don't need to be 
   subscribed to these lists before you can post to them, but it is a good 
   idea to subscribe in order to see the replies. It is ok to post code 
   contributions to the developers list without being subscribed, though, as 
   long as you mention this in your mail so we will know to reply by private 
   email. Before posting tech-support questions to the Allegro list, please 
   take a moment to read the guidelines in docs/txt/help.txt.

   You can search through the mailing list archives using the engine on 
   http://www.canvaslink.com/allegro/search.htm.

   Note: please don't send messages in HTML format. The increased size 
   places an unnecessary load on the server, and many subscribers have a 
   hard time reading these posts.

   Note: please do not crosspost between these lists. Choose the most 
   appropriate one for your message, and then send it only to that list.

   Note: please don't send large binary attachments to any of the lists. 
   Upload your files to a website and then post the URL, or if you can't do 
   that, post an annoucement asking people to write to you privately, and 
   then send the file by individual email to whoever responds.

   My personal address is shawn@talula.demon.co.uk, but please try to avoid 
   mailing me personally about anything other than personal issues. Use the 
   main Allegro list for questions about Allegro, and the developers list 
   for sending code contributions. I read all the lists, and by using them 
   you are likely to get a response even if I'm not around or am busy with 
   something else at the moment.

   For the ever-shrinking minority of people without net access, my snail 
   address is Flat 1, 71 Croham Road, Croydon, Surrey, England, CR2 7HG. For 
   the even smaller minority of people who can't get access to a postal 
   service, find your way to South Croydon railway station and head off down 
   Croham Road, away from the Croham Arms pub. Just after the turnoff to 
   Castlemaine Avenue, it is the first house on the left.

