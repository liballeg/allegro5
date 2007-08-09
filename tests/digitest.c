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
 *      Digital sample playing test program for the Allegro library.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>

#include "allegro.h"



char samplename[80*6] = EMPTY_STRING; /* 80 chars * max UTF8 char width */


extern DIALOG thedialog[];


#define DRIVER_STR   4
#define DESC_STR     6
#define WAVEFORM     8
#define PLAYMODE     9
#define VOLUME       15
#define END_VOLUME   17
#define VOLUME_TIME  19
#define FREQ         21
#define END_FREQ     23
#define FREQ_TIME    25
#define PAN          27
#define END_PAN      29
#define PAN_TIME     31



int waveform_proc(int msg, DIALOG *d, int c)
{
   SAMPLE *s = (SAMPLE *)d->dp;
   BITMAP *b = (BITMAP *)d->dp2;
   unsigned char *cp = NULL;
   unsigned short *sp = NULL;
   int val, i, j, prev, previ, min, max;

   #define SAMPLE_TO_SCREEN(x)   ((x) * d->w / (int)s->len)
   #define SCREEN_TO_SAMPLE(x)   ((x) * (int)s->len / d->w)

   #define UPDATE_VALUE(oldval, newval)                     \
   {                                                        \
      if (newval < 0)                                       \
	 newval = 0;                                        \
							    \
      if (newval >= (int)s->len)                            \
	 newval = s->len;                                   \
							    \
      if ((int)newval != (int)oldval) {                     \
	 i = SAMPLE_TO_SCREEN(oldval);                      \
	 j = SAMPLE_TO_SCREEN(newval);                      \
	 min = MAX(d->x+MIN(i,j)-6, d->x+1);                \
	 max = MIN(d->x+MAX(i,j)+6, d->x+d->w-2);           \
	 oldval = newval;                                   \
	 set_clip_rect(screen, min, d->y+1, max, d->y+d->h-2); \
	 object_message(d, MSG_DRAW, 1);                    \
	 set_clip_rect(screen, 0, 0, SCREEN_W-1, SCREEN_H-1);  \
      }                                                     \
   }

   switch (msg) {

      case MSG_START:
	 if (d->d1 >= 0) {
	    voice_stop(d->d1);
	    deallocate_voice(d->d1);
	    d->d1 = -1;
	 }
	 if (s) {
	    destroy_sample(s);
	    d->dp = NULL;
	 }
	 if (b) {
	    destroy_bitmap(b);
	    d->dp2 = NULL;
	 }
	 s = load_sample(samplename);
	 if (!s) {
	    alert("Error reading", samplename, NULL, "Oh dear...", NULL, 13, 0);
	    *((char *)thedialog[FREQ].dp) = 0;
	    *((char *)thedialog[END_FREQ].dp) = 0;
	 }
	 else {
	    b = create_bitmap(d->w, d->h);
	    clear_to_color(b, d->bg);

	    if (s->bits == 8)
	       cp = (unsigned char *)s->data;
	    else if (s->bits == 16)
	       sp = (unsigned short *)s->data;

	    previ = 0;
	    prev = 0;

	    for (i=0; i<d->w; i++) {
	       if (cp)
		  val = cp[SCREEN_TO_SAMPLE(i)] - 128;
	       else if (sp)
		  val = sp[SCREEN_TO_SAMPLE(i)] / 256 - 128;
	       else
		  val = 0;

	       val = d->h/2 + val*(d->h-2)/256;

	       line(b, previ, prev, i, val, palette_color[1]);

	       previ = i;
	       prev = val;
	    }

	    rect(b, 0, 0, d->w-1, d->h-1, d->fg);

	    d->dp = s;
	    d->dp2 = b;

	    sprintf(thedialog[FREQ].dp, "%d", s->freq);
	    sprintf(thedialog[END_FREQ].dp, "%d", s->freq);
	 }
	 d->d1 = -1;
	 d->d2 = 0;
	 break;

      case MSG_END:
	 if (d->d1 >= 0) {
	    voice_stop(d->d1);
	    deallocate_voice(d->d1);
	    d->d1 = -1;
	 }
	 if (s) {
	    destroy_sample(s);
	    d->dp = NULL;
	 }
	 if (b) {
	    destroy_bitmap(b);
	    d->dp2 = NULL;
	 }
	 break;

      case MSG_DRAW:
	 if (b) {
	    blit(b, screen, 0, 0, d->x, d->y, d->w, d->h);

	    if (s) {
	       if (!c)
		  set_clip_rect(screen, d->x+1, d->y+1, d->x+d->w-2, d->y+d->h-2);

	       val = SAMPLE_TO_SCREEN(s->loop_start);
	       if ((val > 0) && (val < d->w)) {
		  for (i=0; i<d->h; i+=12) {
		     line(screen, d->x+val, d->y+1+i, d->x+val+4, d->y+7+i, palette_color[2]);
		     line(screen, d->x+val+4, d->y+7+i, d->x+val, d->y+13+i, palette_color[2]);
		  }
		  vline(screen, d->x+val, d->y+1, d->y+d->h-2, palette_color[2]);
	       }

	       val = SAMPLE_TO_SCREEN(s->loop_end);
	       if ((val > 0) && (val < d->w)) {
		  for (i=0; i<d->h; i+=12) {
		     line(screen, d->x+val, d->y+1+i, d->x+val-4, d->y+7+i, palette_color[3]);
		     line(screen, d->x+val-4, d->y+7+i, d->x+val, d->y+13+i, palette_color[3]);
		  }
		  vline(screen, d->x+val, d->y+1, d->y+d->h-2, palette_color[3]);
	       } 

	       val = SAMPLE_TO_SCREEN(d->d2);
	       if ((val > 0) && (val < d->w))
		  vline(screen, d->x+val, d->y+1, d->y+d->h-2, palette_color[4]);

	       if (!c)
		  set_clip_rect(screen, 0, 0, SCREEN_W-1, SCREEN_H-1);
	    }
	 }
	 else {
	    rectfill(screen, d->x+1, d->y+1, d->x+d->w-2, d->y+d->h-2, d->bg);
	    rect(screen, d->x, d->y, d->x+d->w-1, d->y+d->h-1, d->fg);
	 }
	 break;

      case MSG_KEY:
	 if (d->d1 >= 0) {
	    voice_stop(d->d1);
	    deallocate_voice(d->d1);
	    d->d1 = -1;
	 }
	 else if (s) {
	    d->d1 = allocate_voice(s);
	    if (d->d1 >= 0) {
	       switch (thedialog[PLAYMODE].d1) {

		  case 0:
                     voice_set_playmode(d->d1, PLAYMODE_PLAY);
                     break;

		  case 1:
                     voice_set_playmode(d->d1, PLAYMODE_LOOP);
                     break;

		  case 2:
                     voice_set_playmode(d->d1, PLAYMODE_BACKWARD);
                     if (d->d2 == 0)
                        d->d2 = s->len - 1;
                     break;

		  case 3:
                     voice_set_playmode(d->d1, PLAYMODE_BACKWARD | PLAYMODE_LOOP);
                     if (d->d2 == 0)
                        d->d2 = s->len - 1;
                     break;

		  case 4:
                     voice_set_playmode(d->d1, PLAYMODE_BIDIR | PLAYMODE_LOOP);
                     break;
	       }

	       voice_set_position(d->d1, d->d2);

	       val = atoi(thedialog[VOLUME].dp);
	       i = atoi(thedialog[END_VOLUME].dp);
	       j = atoi(thedialog[VOLUME_TIME].dp);
	       voice_set_volume(d->d1, CLAMP(0, val, 255));
	       if (j > 0)
		  voice_ramp_volume(d->d1, CLAMP(0, j, 999999), CLAMP(0, i, 255));

	       val = atoi(thedialog[FREQ].dp);
	       i = atoi(thedialog[END_FREQ].dp);
	       j = atoi(thedialog[FREQ_TIME].dp);
	       voice_set_frequency(d->d1, CLAMP(1000, val, 99999));
	       if (j > 0)
		  voice_sweep_frequency(d->d1, CLAMP(0, j, 999999), CLAMP(1000, i, 99999));

	       val = atoi(thedialog[PAN].dp);
	       i = atoi(thedialog[END_PAN].dp);
	       j = atoi(thedialog[PAN_TIME].dp);
	       voice_set_pan(d->d1, CLAMP(0, val, 255));
	       if (j > 0)
		  voice_sweep_pan(d->d1, CLAMP(0, j, 999999), CLAMP(0, i, 255));

	       voice_start(d->d1); 
	    }
	 }
	 break;

      case MSG_CLICK:
	 poll_keyboard();
	 if (key_shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG)) {
	    while (mouse_b) {
	       poll_mouse();
	       val = SCREEN_TO_SAMPLE(mouse_x - d->x);
	       UPDATE_VALUE(d->d2, val);
	       if (d->d1 >= 0)
		  voice_set_position(d->d1, d->d2);
	    }
	 }
	 else {
	    if (d->d1 >= 0) {
	       voice_stop(d->d1);
	       deallocate_voice(d->d1);
	       d->d1 = -1;
	    }
	    while (mouse_b) {
	       poll_mouse();
	       val = SCREEN_TO_SAMPLE(mouse_x - d->x);
	       if (mouse_b & 1) {
		  UPDATE_VALUE(s->loop_start, val);
	       }
	       else if (mouse_b & 2) {
		  UPDATE_VALUE(s->loop_end, val);
	       }
	    }
	 }
	 break;

      case MSG_IDLE:
	 if ((s) && (d->d1 >= 0)) {
	    val = voice_get_position(d->d1);
	    if (val < 0) {
	       voice_stop(d->d1);
	       deallocate_voice(d->d1);
	       d->d1 = -1;
	    }
	    UPDATE_VALUE(d->d2, val);
	 }
	 break;

      case MSG_WANTFOCUS:
	 return D_WANTFOCUS;
   }

   return D_O_K;
}



char *playmode_getter(int index, int *list_size)
{
   static char *playmodes[] = 
   {
      "Play",
      "Loop",
      "Reverse",
      "Reverse Loop",
      "Bidirectional"
   };

   if (index < 0) {
      if (list_size)
	 *list_size = sizeof(playmodes) / sizeof(char *);

      return NULL;
   }

   return playmodes[index];
}



int playmode_proc(int msg, DIALOG *d, int c)
{
   int ret = d_list_proc(msg, d, c);

   if (ret & D_CLOSE) {
      ret &= ~D_CLOSE;
      object_message(thedialog+WAVEFORM, MSG_KEY, 0);
   }

   return ret;
}



int load_proc(int msg, DIALOG *d, int c)
{
   int ret = d_button_proc(msg, d, c);

   if (ret & D_CLOSE) {
      ret &= ~D_CLOSE;

      if (file_select_ex("Load sample (voc;wav)", samplename, "VOC;WAV", sizeof(samplename), 0, 0)) {
	 object_message(thedialog+WAVEFORM, MSG_START, 0);
      }

      ret |= D_REDRAW;
   }

   return ret;
}



char vol_str[4] = "255";
char vol_end_str[4] = "255";
char vol_time_str[7] = "-1";

char freq_str[6] = "";
char freq_end_str[6] = "";
char freq_time_str[7] = "-1";

char pan_str[4] = "127";
char pan_end_str[4] = "127";
char pan_time_str[7] = "-1";



DIALOG thedialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)              (dp2) (dp3) */
   { d_clear_proc,      0,    0,    0,    0,    0,    8,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { d_ctext_proc,      320,  4,    0,    0,    255,  8,    0,       0,          0,             0,       "Digital sound test program for Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR, NULL, NULL },
   { d_ctext_proc,      320,  20,   0,    0,    255,  8,    0,       0,          0,             0,       "By Shawn Hargreaves, " ALLEGRO_DATE_STR, NULL, NULL },
   { d_text_proc,       32,   56,   0,    0,    255,  8,    0,       0,          0,             0,       "Driver:",        NULL, NULL  },
   { d_text_proc,       96,   56,   0,    0,    255,  8,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { d_text_proc,       32,   72,   0,    0,    255,  8,    0,       0,          0,             0,       "Desc:",          NULL, NULL  },
   { d_text_proc,       80,   72,   0,    0,    255,  8,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { d_text_proc,       32,   104,  0,    0,    255,  8,    0,       0,          0,             0,       samplename,       NULL, NULL  },
   { waveform_proc,     16,   304,  608,  160,  255,  0,    13,      0,          0,             0,       NULL,             NULL, NULL  },
   { playmode_proc,     32,   144,  129,  44,   255,  0,    0,       D_EXIT,     0,             0,       playmode_getter,  NULL, NULL  },
   { load_proc,         32,   208,  129,  17,   255,  0,    0,       D_EXIT,     0,             0,       "Load WAV",       NULL, NULL  },
   { d_text_proc,       32,   248,  0,    0,    255,  8,    0,       0,          0,             0,       "Click the left and right mouse buttons on the waveform to set", NULL, NULL },
   { d_text_proc,       32,   260,  0,    0,    255,  8,    0,       0,          0,             0,       "the loop points. Use ctrl+click to alter the sample position,", NULL, NULL },
   { d_text_proc,       32,   272,  0,    0,    255,  8,    0,       0,          0,             0,       "and hit enter to toggle the sound on and off.", NULL, NULL },
   { d_text_proc,       276,  144,  0,    0,    255,  8,    0,       0,          0,             0,       "Vol:",           NULL, NULL  },
   { d_edit_proc,       324,  144,  64,   16,   255,  8,    0,       0,          3,             3,       vol_str,          NULL, NULL  },
   { d_text_proc,       388,  144,  0,    0,    255,  8,    0,       0,          0,             0,       "End:",           NULL, NULL  },
   { d_edit_proc,       428,  144,  64,   16,   255,  8,    0,       0,          3,             3,       vol_end_str,      NULL, NULL  },
   { d_text_proc,       492,  144,  0,    0,    255,  8,    0,       0,          0,             0,       "Time:",          NULL, NULL  },
   { d_edit_proc,       540,  144,  96,   16,   255,  8,    0,       0,          6,             2,       vol_time_str,     NULL, NULL  },
   { d_text_proc,       276,  168,  0,    0,    255,  8,    0,       0,          0,             0,       "Freq:",          NULL, NULL  },
   { d_edit_proc,       324,  168,  64,   16,   255,  8,    0,       0,          5,             5,       freq_str,         NULL, NULL  },
   { d_text_proc,       388,  168,  0,    0,    255,  8,    0,       0,          0,             0,       "End:",           NULL, NULL  },
   { d_edit_proc,       428,  168,  64,   16,   255,  8,    0,       0,          5,             5,       freq_end_str,     NULL, NULL  },
   { d_text_proc,       492,  168,  0,    0,    255,  8,    0,       0,          0,             0,       "Time:",          NULL, NULL  },
   { d_edit_proc,       540,  168,  96,   16,   255,  8,    0,       0,          6,             2,       freq_time_str,    NULL, NULL  },
   { d_text_proc,       276,  192,  0,    0,    255,  8,    0,       0,          0,             0,       "Pan:",           NULL, NULL  },
   { d_edit_proc,       324,  192,  64,   16,   255,  8,    0,       0,          3,             3,       pan_str,          NULL, NULL  },
   { d_text_proc,       388,  192,  0,    0,    255,  8,    0,       0,          0,             0,       "End:",           NULL, NULL  },
   { d_edit_proc,       428,  192,  64,   16,   255,  8,    0,       0,          3,             3,       pan_end_str,      NULL, NULL  },
   { d_text_proc,       492,  192,  0,    0,    255,  8,    0,       0,          0,             0,       "Time:",          NULL, NULL  },
   { d_edit_proc,       540,  192,  96,   16,   255,  8,    0,       0,          6,             2,       pan_time_str,     NULL, NULL  },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  }
};



int main(int argc, char *argv[])
{
   if (argc > 1)
      strcpy(samplename, argv[1]);

   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_mouse();
   install_timer();

   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Error setting graphics mode\n%s\n", allegro_error);
	 return 1;
      }
   }

   /* we use only one voice */
   set_volume_per_voice(0);

   if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error initialising sound\n%s\n", allegro_error);
      return 1;
   }

   set_palette(black_palette);
   clear_to_color(screen, palette_color[8]);
   set_palette(desktop_palette);

   if (!samplename[0]) {
      if (!file_select_ex("Load sample (voc;wav)", samplename, "VOC;WAV", sizeof(samplename), 0, 0))
	 return 0;
   }

   /* we set up colors to match screen color depth (in case it changed) */
   for (argc = 0; thedialog[argc].proc; argc++) {
      thedialog[argc].fg = palette_color[thedialog[argc].fg];
      thedialog[argc].bg = palette_color[thedialog[argc].bg];
   }

   thedialog[DRIVER_STR].dp = (void*)digi_driver->name;
   thedialog[DESC_STR].dp = (void*)digi_driver->desc;

   do_dialog(thedialog, -1);

   return 0;
}

END_OF_MAIN()
