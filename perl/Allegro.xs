#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include <allegro.h>

void *_mangled_main_address;

MODULE = Allegro		PACKAGE = Allegro		

PROTOTYPES: DISABLE


int
allegro_init()
CODE:
	RETVAL = allegro_init();
OUTPUT:
	RETVAL

int
set_gfx_mode(mode,w,h,vw,vh)
	int mode
	int w
	int h
	int vw
	int vh
CODE:
	RETVAL = set_gfx_mode (mode, w, h, vw, vh);
OUTPUT:
	RETVAL

int
GFX_AUTODETECT()
CODE:
	RETVAL = GFX_AUTODETECT;
OUTPUT:
	RETVAL

void
line(bitmap,x1,y1,x2,y2,c)
	int bitmap
	int x1
	int y1
	int x2
	int y2
	int c
CODE:
	line ((BITMAP *)bitmap, x1, y1, x2, y2, c);

int
screen()
CODE:
	RETVAL = (int)screen;
OUTPUT:
	RETVAL

int
install_keyboard()
CODE:
	RETVAL = install_keyboard();
OUTPUT:
	RETVAL

int
readkey()
CODE:
	RETVAL = readkey();
OUTPUT:
	RETVAL

