/*
 * xf2pcx.c --- convert X-Windows font to pcx file
 *
 * Copyright (C) 1998  Michael Bukin
 * Minor changes in 2002  Peter Wang
 */

/*
 * Converter for X-Windows font to pcx files, suitable for grabbing.
 * Has some command-line options, you can get a summary on them, by
 * using `-h' option.
 *
 * To compile this utility use command:
 *
 * gcc -Wall -O2 -g -o xf2pcx xf2pcx.c -lX11 -lm
 *
 * Probably you will need to tell compiler where to find include
 * files and libraries, e.g.:
 *
 * gcc -Wall -O2 -g -I/usr/X11R6/include -o xf2pcx xf2pcx.c -L/usr/X11R6/lib -lX11 -lm
 *
 * Examples of usage:
 *
 * ./xf2pcx -h
 *    get a help on command-line options.
 *
 * ./xf2pcx -f '8x15' -o font8x15.pcx
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct BITMAP
{
  int w, h;
  unsigned char** line;
} BITMAP;

typedef struct RGB
{
  unsigned char r, g, b;
  unsigned char filler;
} RGB;

void usage (char* _argv0);
BITMAP* create_bitmap (int _w, int _h);
void destroy_bitmap (BITMAP* _bmp);
void putpixel (BITMAP* _bmp, int _x, int _y, int _color);
int getpixel (BITMAP* _bmp, int _x, int _y);
int save_pcx (char* _filename, BITMAP* _bmp, RGB* _pal);

int
main (int _argc, char* _argv[])
{
  int i, opt;
  char* fontname = "*";
  char* filename = "font.pcx";
  Display* display;
  int screen_number;
  int default_depth;
  Window window;
  Font font;
  unsigned long val_mask;
  XGCValues val_bits;
  GC gc;
  int fixup_height = 1;
  int bitmap_width;
  int bitmap_height;
  int max_ascent;
  int max_descent;
  int max_height;
  int x, y, cx, cy, sx, sy;
  unsigned long black;
  unsigned long white;
  Pixmap pixmap;
  XImage* image;
  XCharStruct overall;
  BITMAP* bitmap;
  RGB palette[256];
  long ccolor = 255255255;
  long bcolor = 0;
  long gcolor = 128128128;

  /* Show usage if no options.  */
  if (_argc == 1)
    {
      usage (_argv[0]);
      exit (EXIT_SUCCESS);
    }

  /* Parse options.  */
  opterr = 0;
  while ((opt = getopt (_argc, _argv, "f:o:z:c:b:g:h")) != EOF)
    {
      switch (opt)
	{
	case 'f':
	  fontname = optarg;
	  break;
	case 'o':
	  filename = optarg;
	  break;
	case 'z':
	  fixup_height = atoi (optarg);
	  break;
	case 'c':
	  ccolor = atol (optarg);
	  break;
	case 'b':
	  bcolor = atol (optarg);
	  break;
	case 'g':
	  gcolor = atol (optarg);
	  break;
	case 'h':
	  usage (_argv[0]);
	  exit (EXIT_SUCCESS);
	default:
	  fprintf (stderr, "%s: unrecognized option -- '%c'\n", _argv[0], optopt);
	  fprintf (stderr, "%s: try '%s -h' for more information\n", _argv[0], _argv[0]);
	  exit (EXIT_FAILURE);
	}
    }

  /* Open display.  */
  display = XOpenDisplay (0);
  if (display == 0)
    {
      fprintf (stderr, "%s: XOpenDisplay failed\n", _argv[0]);
      exit (EXIT_FAILURE);
    }

  /* Default screen number and window.  */
  screen_number = XDefaultScreen (display);
  default_depth = XDefaultDepth (display, screen_number);
  window = XDefaultRootWindow (display);

  /* Load font.  */
  font = XLoadFont (display, fontname);

  /* Create gc.  */
  val_mask = GCForeground | GCBackground | GCFont | GCFunction;
  val_bits.function = GXcopy;
  val_bits.foreground = white = WhitePixel (display, screen_number);
  val_bits.background = black = BlackPixel (display, screen_number);
  val_bits.font = font;
  gc = XCreateGC (display, window, val_mask, &val_bits);

  /* Calculate bitmap width and maximum ascent and descent of characters.  */
  bitmap_width = 0;
  max_ascent = 0;
  max_descent = 0;
  for (cy = 0; cy < 14; cy++)
    {
      int row_width = 1;

      for (cx = 0; cx < 16; cx++)
	{
	  int dir, ascent, descent;
	  int width;
	  char string[2] = { '\0', '\0' };

	  /* Query character size.  */
	  string[0] = 32 + cy * 16 + cx;
	  XQueryTextExtents (display, font, string, 1, &dir, &ascent, &descent, &overall);
	  width = overall.width;
	  if (width < 1)
	    width = 1;
	  if (max_ascent < overall.ascent)
	    max_ascent = overall.ascent;
	  if (max_descent < overall.descent)
	    max_descent = overall.descent;

	  row_width += width + 1;
	}

      if (bitmap_width < row_width)
	bitmap_width = row_width;
    }

  /* Calculate max height.  */
  max_height = max_ascent + max_descent;
  if (max_height < 1)
    max_height = 1;

  /* Calculate bitmap height.  */
  bitmap_height = 1;
  for (cy = 0; cy < 14; cy++)
    {
      int row_height = 0;

      for (cx = 0; cx < 16; cx++)
	{
	  int dir, ascent, descent;
	  int height;
	  char string[2] = { '\0', '\0' };

	  /* Query character size.  */
	  string[0] = 32 + cy * 16 + cx;
	  XQueryTextExtents (display, font, string, 1, &dir, &ascent, &descent, &overall);
	  height = max_ascent + overall.descent;
	  if (height < 1)
	    height = 1;
	  if ((fixup_height != 0) && (cx == 0) && (cy == 0))
	    height = max_height;

	  if (row_height < height)
	    row_height = height;
	}

      bitmap_height += row_height + 1;
    }

  /* Create bitmap.  */
  bitmap = create_bitmap (bitmap_width, bitmap_height);
  if (bitmap == 0)
    {
      fprintf (stderr, "%s: can not create bitmap\n", _argv[0]);
      exit (EXIT_FAILURE);
    }

  /* Fill with filler color.  */
  for (y = 0; y < bitmap_height; y++)
    for (x = 0; x < bitmap_width; x++)
      putpixel (bitmap, x, y, 255);

  /* Process all characters.  */
  sy = 1;
  for (cy = 0; cy < 14; cy++)
    {
      int row_height = 0;

      sx = 1;
      for (cx = 0; cx < 16; cx++)
	{
	  int dir, ascent, descent;
	  int width, height;
	  char string[2] = { '\0', '\0' };

	  /* Query character size.  */
	  string[0] = 32 + cy * 16 + cx;
	  XQueryTextExtents (display, font, string, 1, &dir, &ascent, &descent, &overall);
	  width = overall.width;
	  if (width < 1)
	    width = 1;
	  height = max_ascent + overall.descent;
	  if (height < 1)
	    height = 1;
	  if ((fixup_height != 0) && (cx == 0) && (cy == 0))
	    height = max_height;

	  /* Create pixmap and draw character there.  */
	  pixmap = XCreatePixmap (display, window, width, height, default_depth);
	  XDrawImageString (display, pixmap, gc, 0, max_ascent, string, 1);

	  /* Create image with pixmap contents.  */
	  image = XGetImage (display, pixmap, 0, 0, width, height, AllPlanes, ZPixmap);
	  if (image == 0)
	    {
	      fprintf (stderr, "%s: can not get image\n", _argv[0]);
	      exit (EXIT_FAILURE);
	    }

	  /* Copy image to bitmap.  */
	  for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++)
	      {
		if ((fixup_height != 0) && (cx == 0) && (cy == 0)
		    && (y >= (max_ascent + overall.descent)))
		  putpixel (bitmap, sx + x, sy + y, 0);
		else if (y < (max_ascent - overall.ascent))
		  putpixel (bitmap, sx + x, sy + y, 0);
		else if (XGetPixel (image, x, y) == white)
		  putpixel (bitmap, sx + x, sy + y, 1);
		else
		  putpixel (bitmap, sx + x, sy + y, 0);
	      }

	  XDestroyImage (image);
	  XFreePixmap (display, pixmap);

	  sx += width + 1;
	  if (row_height < height)
	    row_height = height;
	}

      sy += row_height + 1;
    }

  /* Initialize palette.  */
  for (i = 0; i < 256; i++)
    palette[i].r = palette[i].g = palette[i].b = 0;

#define CLAMP(v) (((v) > 255) ? 255 : (v))
  palette[0].r = CLAMP (bcolor / 1000000);
  palette[0].g = CLAMP ((bcolor % 1000000) / 1000);
  palette[0].b = CLAMP (bcolor % 1000);
  palette[1].r = CLAMP (ccolor / 1000000);
  palette[1].g = CLAMP ((ccolor % 1000000) / 1000);
  palette[1].b = CLAMP (ccolor % 1000);
  palette[255].r = CLAMP (gcolor / 1000000);
  palette[255].g = CLAMP ((gcolor % 1000000) / 1000);
  palette[255].b = CLAMP (gcolor % 1000);
#undef CLAMP
  save_pcx (filename, bitmap, palette);

  /* Clean up.  */
  destroy_bitmap (bitmap);
  XFreeGC (display, gc);
  XUnloadFont (display, font);
  XCloseDisplay (display);

  return EXIT_SUCCESS;
}

/* usage:
 *  Show usage information.
 */
void
usage (char* _argv0)
{
  printf ("Usage: %s [OPTIONS]\n\n", _argv0);
  printf ("  -f FONTNAME  Font name pattern ('*' by default).\n"
	  "  -o FILENAME  Output file name ('font.pcx' by default).\n"
	  "  -z ARG  Fixup height of the first character or not.\n"
	  "          ARG can be 0 -- don't fixup, or any other number -- do fixup.\n"
	  "          Default is to fixup height.\n"
	  "\n"
	  "  -c COLOR  Character color (default 255255255 -- white).\n"
	  "  -b COLOR  Background color (default 000000000 -- black).\n"
	  "  -g COLOR  Grid color (default 128128128 -- gray).\n"
	  "            Colors are specified as RRRGGGBBB values.\n"
	  "            With each RRR, GGG and BBB in the range [0, 255].\n"
	  "\n"
	  "  -h  This help.\n"
	  "\n"
	  "  Notes:\n"
	  "    Allegro uses height of the first character of proportional font\n"
	  "    to determine height of the font.  Fixup here means that the first\n"
	  "    character is made not smaller than any other character in this font.\n"
	  "    No fixup means that first character is left unchanged.\n"
	  "\n"
	  "    Find font you want to convert, using xlsfonts or xfontsel.\n"
	  "\n"
	  "bash> xlsfonts -fn '*' | less\n"
	  "bash> %s -f '-adobe-courier-medium-r-normal--34-*'\n"
	  "bash> grabber\n"
	  "\n"
	  "    In grabber create new font 'Object|New|Font'.\n"
	  "    Use 'Object|Grab' on this object and choose pcx file with this font.\n"
	  "\n"
	  "    This utility may fail if display is not accessible or there is not\n"
	  "    enough memory, in which case it will print error message and exit.\n",
	  _argv0);
}

/* create_bitmap:
 *  Creates bitmap of size w x h.
 */
BITMAP*
create_bitmap (int _w, int _h)
{
  int y;
  BITMAP* bmp;
  unsigned char* data;

  if ((_w <= 0) || (_h <= 0))
    return 0;

  bmp = (BITMAP*) malloc (sizeof (BITMAP) + sizeof (unsigned char*) * _h + _w * _h);
  if (bmp == 0)
    return 0;

  bmp->w = _w;
  bmp->h = _h;
  bmp->line = (unsigned char**) ((unsigned char*) bmp + sizeof (BITMAP));

  data = (unsigned char*) (bmp->line) + sizeof (unsigned char*) * _h;
  for (y = 0; y < _h; y++)
    bmp->line[y] = data + y * _w;

  return bmp;
}

/* destroy_bitmap:
 *  Destroys bitmap and its memory.
 */
void
destroy_bitmap (BITMAP* _bmp)
{
  if (_bmp != 0)
    free (_bmp);
}

/* getpixel:
 *  Returns value of pixel in bitmap.
 */
int
getpixel (BITMAP* _bmp, int _x, int _y)
{
  if ((_bmp == 0) || (_x < 0) || (_x >= _bmp->w) || (_y < 0) || (_y >= _bmp->h))
    return 0;
  else
    return _bmp->line[_y][_x];
}

/* putpixel:
 *  Sets pixel value.
 */
void
putpixel (BITMAP* _bmp, int _x, int _y, int _color)
{
  if ((_bmp != 0) && (_x >= 0) && (_x < _bmp->w) && (_y >= 0) && (_y < _bmp->h))
    _bmp->line[_y][_x] = _color;
}

/* pack_putc:
 *  Writes an 8 bit int to a file.
 */
int pack_putc(int c, FILE *f)
{
   fputc (c, f);
   return c;
}

/* pack_iputw:
 *  Writes a 16 bit int to a file, using intel byte ordering.
 */
int pack_iputw(int w, FILE *f)
{
   int b1, b2;

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (pack_putc(b2,f)==b2)
      if (pack_putc(b1,f)==b1)
	 return w;

   return EOF;
}

/* pack_iputw:
 *  Writes a 32 bit long to a file, using intel byte ordering.
 */
long pack_iputl(long l, FILE *f)
{
   int b1, b2, b3, b4;

   b1 = (int)((l & 0xFF000000L) >> 24);
   b2 = (int)((l & 0x00FF0000L) >> 16);
   b3 = (int)((l & 0x0000FF00L) >> 8);
   b4 = (int)l & 0x00FF;

   if (pack_putc(b4,f)==b4)
      if (pack_putc(b3,f)==b3)
	 if (pack_putc(b2,f)==b2)
	    if (pack_putc(b1,f)==b1)
	       return l;

   return EOF;
}

/* save_pcx:
 *  Writes a bitmap into a PCX file, using the specified pallete (this
 *  should be an array of at least 256 RGB structures).
 */
int save_pcx(char *filename, BITMAP *bmp, RGB *pal)
{
   FILE *f;
   int c;
   int x, y;
   int runcount;
   int depth, planes;
   char runchar;
   char ch;

   f = fopen(filename, "wb");
   if (!f)
      return errno;

   depth = 8;
   planes = 1;

   pack_putc(10, f);                      /* manufacturer */
   pack_putc(5, f);                       /* version */
   pack_putc(1, f);                       /* run length encoding  */
   pack_putc(8, f);                       /* 8 bits per pixel */
   pack_iputw(0, f);                      /* xmin */
   pack_iputw(0, f);                      /* ymin */
   pack_iputw(bmp->w-1, f);               /* xmax */
   pack_iputw(bmp->h-1, f);               /* ymax */
   pack_iputw(320, f);                    /* HDpi */
   pack_iputw(200, f);                    /* VDpi */
   for (c=0; c<16; c++) {
      pack_putc(pal[c].r, f);
      pack_putc(pal[c].g, f);
      pack_putc(pal[c].b, f);
   }
   pack_putc(0, f);                       /* reserved */
   pack_putc(planes, f);                  /* one or three color planes */
   pack_iputw(bmp->w, f);                 /* number of bytes per scanline */
   pack_iputw(1, f);                      /* color pallete */
   pack_iputw(bmp->w, f);                 /* hscreen size */
   pack_iputw(bmp->h, f);                 /* vscreen size */
   for (c=0; c<54; c++)                   /* filler */
      pack_putc(0, f);

   for (y=0; y<bmp->h; y++) {             /* for each scanline... */
      runcount = 0;
      runchar = 0;
      for (x=0; x<bmp->w*planes; x++) {   /* for each pixel... */
	 ch = getpixel (bmp, x, y);
	 if (runcount==0) {
	    runcount = 1;
	    runchar = ch;
	 }
	 else {
	    if ((ch != runchar) || (runcount >= 0x3f)) {
	       if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
		  pack_putc(0xC0 | runcount, f);
	       pack_putc(runchar,f);
	       runcount = 1;
	       runchar = ch;
	    }
	    else
	       runcount++;
	 }
      }
      if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
	 pack_putc(0xC0 | runcount, f);
      pack_putc(runchar,f);
   }

   pack_putc(12, f); 
   for (c=0; c<256; c++) {
     pack_putc(pal[c].r, f);
     pack_putc(pal[c].g, f);
     pack_putc(pal[c].b, f);
   }

   fclose(f);
   return errno;
}

/*
 * xf2pcx.c ends here
 */
