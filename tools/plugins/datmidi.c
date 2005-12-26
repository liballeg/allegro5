/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Grabber plugin for managing MIDI objects.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "../datedit.h"



/* creates a new MIDI object */
static void *makenew_midi(long *size)
{
   MIDI *mid;
   int c;

   mid = _AL_MALLOC(sizeof(MIDI));
   mid->divisions = 120;

   for (c=0; c<MIDI_TRACKS; c++) {
      mid->track[c].data = NULL;
      mid->track[c].len = 0;
   }

   return mid;
}



/* displays a MIDI file in the grabber object view window */
static void plot_midi(AL_CONST DATAFILE *dat, int x, int y)
{
   textout_ex(screen, font, "Double-click in the item list to play it", x, y+32, gui_fg_color, gui_bg_color);
}



/* handles double-clicking on a MIDI file in the grabber */
static int dclick_midi(DATAFILE *dat)
{
   play_midi(dat->dat, FALSE);
   return D_O_K;
}



/* exports a MIDI object into an external file */
static int export_midi(AL_CONST DATAFILE *dat, AL_CONST char *filename)
{
   MIDI *midi = (MIDI *)dat->dat;
   PACKFILE *f;
   int num_tracks;
   int c;

   num_tracks = 0;
   for (c=0; c<MIDI_TRACKS; c++)
      if (midi->track[c].len > 0)
	 num_tracks++;

   errno = 0; 

   f = pack_fopen(filename, F_WRITE);

   if (f) {
      pack_fputs("MThd", f);                 /* MIDI header */
      pack_mputl(6, f);                      /* size of header chunk */
      pack_mputw(1, f);                      /* type 1 */
      pack_mputw(num_tracks, f);             /* number of tracks */
      pack_mputw(midi->divisions, f);        /* beat divisions */

      for (c=0; c<MIDI_TRACKS; c++) {        /* for each track */
	 if (midi->track[c].len > 0) {
	    pack_fputs("MTrk", f);           /* write track data */
	    pack_mputl(midi->track[c].len, f); 
	    pack_fwrite(midi->track[c].data, midi->track[c].len, f);
	 }
      }

      pack_fclose(f);
   }

   return (errno == 0);
}



/* imports a MIDI object from an external file */
static DATAFILE *grab_midi(int type, AL_CONST char *filename, DATAFILE_PROPERTY **prop, int depth)
{
   return datedit_construct(type, load_midi(filename), 0, prop);
}



/* saves a MIDI object in the datafile format */
static int save_midi(DATAFILE *dat, AL_CONST int *fixed_prop, int pack, int pack_kids, int strip, int sort, int verbose, int extra, PACKFILE *f)
{
   MIDI *midi = (MIDI *)dat->dat;
   int c;

   *allegro_errno = 0;

   pack_mputw(midi->divisions, f);

   for (c=0; c<MIDI_TRACKS; c++) {
      pack_mputl(midi->track[c].len, f);
      if (midi->track[c].len > 0)
	 pack_fwrite(midi->track[c].data, midi->track[c].len, f);
   }

   if (*allegro_errno)
      return FALSE;
   else
      return TRUE;
}



/* plugin interface header */
DATEDIT_OBJECT_INFO datmidi_info =
{
   DAT_MIDI, 
   "MIDI file", 
   NULL,
   makenew_midi,
   save_midi,
   plot_midi,
   dclick_midi,
   NULL
};



DATEDIT_GRABBER_INFO datmidi_grabber =
{
   DAT_MIDI, 
   "mid",
   "mid",
   grab_midi,
   export_midi,
   NULL
};


