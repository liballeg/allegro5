     ______   ___    ___
    /\  _  \ /\_ \  /\_ \
    \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
     \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
      \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
       \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
	\/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
				       /\____/
				       \_/__/


		 BeOS-specific information.

	 See readme.txt for a more general overview.



====================================
============ BeOS notes ============
====================================

   Status: It compiles for Intel R4 & R5. Port is complete, but it needs
   beta testing.



===========================================
============ Required software ============
===========================================

   BeOS Intel R4 and R5 Pro Edition come with everything you need. If you
   have BeOS Intel R5 Personal Edition, you require the development tools;
   these can be found on the Be homepage at http://www.be.com.



============================================
============ Installing Allegro ============
============================================

   Allegro comes as a source distribuition; this means you'll have to
   compile it to get it to work. Unzip the library archive wherever you
   want, and cd into that directory with a Terminal. Due to the multi-platform
   nature of Allegro, you need to run:

      fix.sh beos

   This will set the library ready to be compiled on the Be platform.
   Now you must build it:

      make

   And then install it:

      make install

   With this last command the Allegro library will be installed into
   /boot/develop/lib/x86, while the headers will go into 
   /boot/develop/headers/cpp (the default locations where Be looks for them)



=======================================
============ Using Allegro ============
=======================================

   Linking Allegro to a program also requires you to link several other BeOS
   libraries. To simplify the linking process, the installation sets up a
   script, allegro-config, that will print out a suitable commandline. You
   can use this inside backtick command substitution, for example:

      gcc myfile.c -o myprogram `allegro-config --libs`

   Or if you want to build a debug version of your program, assuming that 
   you have installed the debug version of Allegro:

      gcc myfile.c -o myprogram `allegro-config --libs debug`

   Terminal newbies, take note that these are ` backticks, not normal ' quotes!

   There are also other switches for printing out the Allegro version number,
   or to override the install paths. Run allegro-config without any
   arguments for a full list of options.

   Don't forget that you need to use the END_OF_MAIN() macro right after 
   your main() function!
