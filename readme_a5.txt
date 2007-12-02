Thanks for trying this development release of Allegro. This is currently work
in progress and only intended for developers. Here are some short notes:

== Windows ==

The only graphics driver implemented so far requires DirectX 9. Here are some
files you may need:

http://trent.gamblin.ca/dx/

Some of those are originally from:

http://www.g-productions.net/list.php?c=files_devpak

== Linux ==

The only graphics driver implemented so far requires a fairly recent GLX, it
was only tested with GLX 1.4 so far.

== OSX ==

Sorry, nobody did yet write any drivers for OSX.

== Compilation ==

We provide two build systems, cmake and scons. You can use either one.

=== Cmake ===

Cmake works by first creating a makefile, which then can be used to compile.

To use it:

TODO: I have no idea about cmake..

=== Scons ===

Scons uses more sophisticated dependency tracking than make, for example it
does not use timestamps, and in general will always know what to rebuild even
if files are added/removed/renamed, options are changed, or even external
dependencies are changed.

To compile Allegro, simply type this inside the allegro directory:

scons

To install, run (as root):

scons install

TODO: Options, how to run as non-root

== Running the examples ==

Currently, only a few examples work using the new API.

TODO: list them
