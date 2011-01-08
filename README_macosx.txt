Mac OS X-specific notes
=======================

Building Allegro on Mac OS X is the same as on other Unix-like operating systems.
See README_make.txt.  You may also use Xcode but that is not covered there.



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

