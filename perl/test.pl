# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'

######################### We start with some black magic to print on failure.

# Change 1..1 below to 1..last_test_to_print .
# (It may become useful if the test is moved to ./t subdirectory.)

BEGIN { $| = 1; print "1..1\n"; }
END {print "not ok 1\n" unless $loaded;}
use Allegro;
$loaded = 1;
print "ok 1\n";

######################### End of black magic.

# Insert your test code below (better if it prints "ok 13"
# (correspondingly "not ok 13") depending on the success of chunk 13
# of the test code):

Allegro::allegro_init() and die "error initialising Allegro\n";

Allegro::install_keyboard() and die "error installing keyboard\n";

Allegro::set_gfx_mode (Allegro::GFX_AUTODETECT, 640, 480, 0, 0) and die "error setting graphics mode\n";

Allegro::line (Allegro::screen, 100, 100, 500, 300, 14);

Allegro::readkey();

