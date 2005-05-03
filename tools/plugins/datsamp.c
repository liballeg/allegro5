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
 *      Grabber plugin for managing sample objects.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>

#include "allegro.h"
#include "../datedit.h"



/* creates a new sample object */
static void *makenew_sample(long *size)
{
   return create_sample(8, FALSE, 11025, 1024);
}



/* displays a sample in the grabber object view window */
static void plot_sample(AL_CONST DATAFILE *dat, int x, int y)
{
   textout_ex(screen, font, "Double-click in the item list to play it", x, y+32, gui_fg_color, gui_bg_color);
}



/* handles double-clicking on a sample in the grabber */
static int dclick_sample(DATAFILE *dat)
{
   play_sample(dat->dat, 255, 127, 1000, FALSE);
   return D_O_K;
}



/* returns an information string describing a sample object */
static void get_sample_desc(AL_CONST DATAFILE *dat, char *s)
{
   SAMPLE *sample = (SAMPLE *)dat->dat;
   long sec = (sample->len + sample->freq/2) * 10 / MAX(sample->freq, 1);
   char *type = (sample->stereo) ? "stereo" : "mono";

   sprintf(s, "sample (%d bit %s, %d, %ld.%ld sec)", sample->bits, type, sample->freq, sec/10, sec%10);
}



/* exports a sample into an external file */
static int export_sample(AL_CONST DATAFILE *dat, AL_CONST char *filename)
{
   SAMPLE *spl = (SAMPLE *)dat->dat;
   int bps = spl->bits/8 * ((spl->stereo) ? 2 : 1);
   int len = spl->len * bps;
   int i;
   int16_t s;
   PACKFILE *f;

   errno = 0;
   
   f = pack_fopen(filename, F_WRITE);

   if (f) {
      pack_fputs("RIFF", f);                 /* RIFF header */
      pack_iputl(36+len, f);                 /* size of RIFF chunk */
      pack_fputs("WAVE", f);                 /* WAV definition */
      pack_fputs("fmt ", f);                 /* format chunk */
      pack_iputl(16, f);                     /* size of format chunk */
      pack_iputw(1, f);                      /* PCM data */
      pack_iputw((spl->stereo) ? 2 : 1, f);  /* mono/stereo data */
      pack_iputl(spl->freq, f);              /* sample frequency */
      pack_iputl(spl->freq*bps, f);          /* avg. bytes per sec */
      pack_iputw(bps, f);                    /* block alignment */
      pack_iputw(spl->bits, f);              /* bits per sample */
      pack_fputs("data", f);                 /* data chunk */
      pack_iputl(len, f);                    /* actual data length */

      if (spl->bits == 8) {
	 pack_fwrite(spl->data, len, f);     /* write the data */
      }
      else {
	 for (i=0; i < (int)spl->len * ((spl->stereo) ? 2 : 1); i++) {
	    s = ((int16_t *)spl->data)[i];
	    pack_iputw(s^0x8000, f);
	 }
      }

      pack_fclose(f);
   }

   return (errno == 0);
}



/* imports a sample from an external file */
static DATAFILE *grab_sample(int type, AL_CONST char *filename, DATAFILE_PROPERTY **prop, int depth)
{
   return datedit_construct(type, load_sample(filename), 0, prop);
}



/* saves a sample into the datafile format */
static int save_sample_in_datafile(DATAFILE *dat, AL_CONST int *fixed_prop, int pack, int pack_kids, int strip, int sort, int verbose, int extra, PACKFILE *f)
{
   SAMPLE *spl = (SAMPLE *)dat->dat;

   *allegro_errno = 0;

   pack_mputw((spl->stereo) ? -spl->bits : spl->bits, f);
   pack_mputw(spl->freq, f);
   pack_mputl(spl->len, f);
   if (spl->bits == 8) {
      pack_fwrite(spl->data, spl->len * ((spl->stereo) ? 2 : 1), f);
   }
   else {
      int i;

      for (i=0; i < (int)spl->len * ((spl->stereo) ? 2 : 1); i++) {
	 pack_iputw(((int16_t *)spl->data)[i], f);
      }
   }

   if (*allegro_errno)
      return FALSE;
   else
      return TRUE;
}



/* returns a description string for a GUS patch object */
static void get_patch_desc(AL_CONST DATAFILE *dat, char *s)
{
   sprintf(s, "MIDI instrument (%ld bytes)", dat->size);
}



/* plugin interface header */
DATEDIT_OBJECT_INFO datpatch_info =
{
   DAT_PATCH,
   "GUS patch",
   get_patch_desc,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL
};



DATEDIT_GRABBER_INFO datpatch_grabber =
{
   DAT_PATCH,
   "pat",
   "pat",
   NULL,
   NULL,
   NULL
};



/* plugin interface header */
DATEDIT_OBJECT_INFO datsample_info =
{
   DAT_SAMPLE, 
   "Sample", 
   get_sample_desc,
   makenew_sample,
   save_sample_in_datafile,
   plot_sample,
   dclick_sample,
   NULL
};



DATEDIT_GRABBER_INFO datsample_grabber =
{
   DAT_SAMPLE, 
   "voc;wav",
   "wav",
   grab_sample,
   export_sample,
   NULL
};

