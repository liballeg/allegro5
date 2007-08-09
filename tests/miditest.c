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
 *      MIDI instrument playing test program for the Allegro library.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"



extern DIALOG thedialog[];


#define DRIVER_STR   4
#define DESC_STR     6
#define INSTLIST     7
#define PIANO        8
#define VOLUME       10
#define PAN          12



void set_patch(int channel, int prog)
{
   unsigned char msg[2];

   msg[0] = 0xC0+channel;
   msg[1] = prog;

   midi_out(msg, 2);
}



void set_pan(int channel, int pan)
{
   unsigned char msg[3];

   msg[0] = 0xB0+channel;
   msg[1] = 10;
   msg[2] = pan / 2;

   midi_out(msg, 3);
}



void note_on(int channel, int pitch, int vel)
{
   unsigned char msg[3];

   msg[0] = 0x90+channel;
   msg[1] = pitch;
   msg[2] = vel / 2;

   midi_out(msg, 3);
}



void note_off(int channel, int pitch)
{
   unsigned char msg[3];

   msg[0] = 0x80+channel;
   msg[1] = pitch;
   msg[2] = 0;

   midi_out(msg, 3);
}



char *instlist_getter(int index, int *list_size)
{
   static char *names[] = 
   {
      "Acoustic Grand",
      "Bright Acoustic",
      "Electric Grand",
      "Honky-Tonk",
      "Electric Piano 1",
      "Electric Piano 2",
      "Harpsichord",
      "Clav",
      "Celesta",
      "Glockenspiel",
      "Music Box",
      "Vibraphone",
      "Marimba",
      "Xylophone",
      "Tubular Bells",
      "Dulcimer",
      "Drawbar Organ",
      "Percussive Organ",
      "Rock Organ",
      "Church Organ",
      "Reed Organ",
      "Accoridan",
      "Harmonica",
      "Tango Accordian",
      "Acoustic Guitar (nylon)",
      "Acoustic Guitar (steel)",
      "Electric Guitar (jazz)",
      "Electric Guitar (clean)",
      "Electric Guitar (muted)",
      "Overdriven Guitar",
      "Distortion Guitar",
      "Guitar Harmonics",
      "Acoustic Bass",
      "Electric Bass (finger)",
      "Electric Bass (pick)",
      "Fretless Bass",
      "Slap Bass 1",
      "Slap Bass 2",
      "Synth Bass 1",
      "Synth Bass 2",
      "Violin",
      "Viola",
      "Cello",
      "Contrabass",
      "Tremolo Strings",
      "Pizzicato Strings",
      "Orchestral Strings",
      "Timpani",
      "String Ensemble 1",
      "String Ensemble 2",
      "SynthStrings 1",
      "SynthStrings 2",
      "Choir Aahs",
      "Voice Oohs",
      "Synth Voice",
      "Orchestra Hit",
      "Trumpet",
      "Trombone",
      "Tuba",
      "Muted Trumpet",
      "French Horn",
      "Brass Section",
      "SynthBrass 1",
      "SynthBrass 2",
      "Soprano Sax",
      "Alto Sax",
      "Tenor Sax",
      "Baritone Sax",
      "Oboe",
      "English Horn",
      "Bassoon",
      "Clarinet",
      "Piccolo",
      "Flute",
      "Recorder",
      "Pan Flute",
      "Blown Bottle",
      "Skakuhachi",
      "Whistle",
      "Ocarina",
      "Lead 1 (square)",
      "Lead 2 (sawtooth)",
      "Lead 3 (calliope)",
      "Lead 4 (chiff)",
      "Lead 5 (charang)",
      "Lead 6 (voice)",
      "Lead 7 (fifths)",
      "Lead 8 (bass+lead)",
      "Pad 1 (new age)",
      "Pad 2 (warm)",
      "Pad 3 (polysynth)",
      "Pad 4 (choir)",
      "Pad 5 (bowed)",
      "Pad 6 (metallic)",
      "Pad 7 (halo)",
      "Pad 8 (sweep)",
      "FX 1 (rain)",
      "FX 2 (soundtrack)",
      "FX 3 (crystal)",
      "FX 4 (atmosphere)",
      "FX 5 (brightness)",
      "FX 6 (goblins)",
      "FX 7 (echoes)",
      "FX 8 (sci-fi)",
      "Sitar",
      "Banjo",
      "Shamisen",
      "Koto",
      "Kalimba",
      "Bagpipe",
      "Fiddle",
      "Shanai",
      "Tinkle Bell",
      "Agogo",
      "Steel Drums",
      "Woodblock",
      "Taiko Drum",
      "Melodic Tom",
      "Synth Drum",
      "Reverse Cymbal",
      "Guitar Fret Noise",
      "Breath Noise",
      "Seashore",
      "Bird Tweet",
      "Telephone ring",
      "Helicopter",
      "Applause",
      "Gunshot",
      "Acoustic Bass Drum",
      "Bass Drum 1",
      "Side Stick",
      "Acoustic Snare",
      "Hand Clap",
      "Electric Snare",
      "Low Floor Tom",
      "Closed Hi-Hat",
      "High Floor Tom",
      "Pedal Hi-Hat",
      "Low Tom",
      "Open Hi-Hat",
      "Low-Mid Tom",
      "Hi-Mid Tom",
      "Crash Cymbal 1",
      "High Tom",
      "Ride Cymbal 1",
      "Chinese Cymbal",
      "Ride Bell",
      "Tambourine",
      "Splash Cymbal",
      "Cowbell",
      "Crash Cymbal 2",
      "Vibraslap",
      "Ride Cymbal 2",
      "Hi Bongo",
      "Low Bongo",
      "Mute Hi Conga",
      "Open Hi Conga",
      "Low Conga",
      "High Timbale",
      "Low Timbale",
      "High Agogo",
      "Low Agogo",
      "Cabasa",
      "Maracas",
      "Short Whistle",
      "Long Whistle",
      "Short Guiro",
      "Long Guiro",
      "Claves",
      "Hi Wood Block",
      "Low Wood Block",
      "Mute Cuica",
      "Open Cuica",
      "Mute Triangle",
      "Open Triangle"
   };

   if (index < 0) {
      if (list_size)
	 *list_size = sizeof(names) / sizeof(char *);

      return NULL;
   }

   return names[index];
}



int instlist_proc(int msg, DIALOG *d, int c)
{
   int ret = d_list_proc(msg, d, c);

   if (ret & D_CLOSE) {
      ret &= ~D_CLOSE;
      object_message(thedialog+PIANO, MSG_KEY, 0);
   }

   return ret;
}



int piano_proc(int msg, DIALOG *d, int c)
{
   static char blackkey[12] = 
   { 
      FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, 
      TRUE, FALSE, TRUE, FALSE, TRUE, FALSE 
   };

   static int playing_channel = -1;
   static int playing_pitch = -1;

   int channel = 0;
   int patch = 0;
   int pitch = 60;
   int delay = 140;
   int i, l, r;

   switch (msg) {

      case MSG_START:
	 d->d1 = -1;
	 d->d2 = -1;
	 break;

      case MSG_DRAW:
	 for (i=0; i<d->w/12; i++) {
	    if (!blackkey[i%12]) {
	       l = i*12;
	       r = (i+1)*12;
	       if (blackkey[(i-1)%12])
		  l -= 6;
	       if (blackkey[(i+1)%12])
		  r += 6;
	       rectfill(screen, d->x+l+1, d->y+1, d->x+r-1, d->y+d->h-1, (i == d->d1) ? 1 : d->bg);
	       rect(screen, d->x+l, d->y, d->x+r, d->y+d->h, d->fg);
	    }
	 }
	 for (i=0; i<d->w/12; i++) {
	    if (blackkey[i%12]) {
	       l = i*12 + 1;
	       r = (i+1)*12 - 1;
	       rectfill(screen, d->x+l, d->y+d->h/4, d->x+r, d->y+d->h, (i == d->d1) ? 1 : d->fg);
	    }
	 }
	 break;

      case MSG_CLICK:
	 d->d1 = (mouse_x - d->x) / 12;

	 set_clip_rect(screen, d->x+d->d1*12-6, d->y, d->x+d->d1*12+18, d->y+d->h);
	 object_message(d, MSG_DRAW, 0);
	 set_clip_rect(screen, 0, 0, SCREEN_W-1, SCREEN_H-1);

	 pitch = 36 + d->d1;
	 delay = 0;
	 /* fallthrough */

      case MSG_KEY:
	 d->d2 = retrace_count+delay;

	 if (playing_channel >= 0)
	    note_off(playing_channel, playing_pitch);

	 patch = thedialog[INSTLIST].d1;

	 if (patch >= 128) {
	    channel = 9;
	    pitch = patch - 93;
	 }
	 else {
	    set_patch(channel, patch);
	 }

	 set_pan(channel, CLAMP(0, atoi(thedialog[PAN].dp), 127));
	 note_on(channel, pitch, CLAMP(0, atoi(thedialog[VOLUME].dp), 127));

	 playing_channel = channel;
	 playing_pitch = pitch;

	 do {
	    poll_mouse();
	 } while ((mouse_b) && (d->d1 == (mouse_x - d->x) / 12));

	 if (d->d1 >= 0) {
	    set_clip_rect(screen, d->x+d->d1*12-6, d->y, d->x+d->d1*12+18, d->y+d->h);
	    d->d1 = -1;
	    object_message(d, MSG_DRAW, 0);
	    set_clip_rect(screen, 0, 0, SCREEN_W-1, SCREEN_H-1);
	 }
	 break;

      case MSG_IDLE:
	 if ((d->d2 >= 0) && (retrace_count > d->d2)) {
	    if (playing_channel >= 0) {
	       note_off(playing_channel, playing_pitch);
	       playing_channel = -1;
	       playing_pitch = -1;
	    }
	    d->d2 = -1;
	 }
	 break;
   }

   return D_O_K;
}



char volume_str[4] = "255";
char pan_str[4] = "127";


DIALOG thedialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)              (dp2) (dp3) */
   { d_clear_proc,      0,    0,    0,    0,    0,    8,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { d_ctext_proc,      320,  4,    0,    0,    255,  8,    0,       0,          0,             0,       "MIDI test program for Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR, NULL, NULL },
   { d_ctext_proc,      320,  20,   0,    0,    255,  8,    0,       0,          0,             0,       "By Shawn Hargreaves, " ALLEGRO_DATE_STR, NULL, NULL },
   { d_text_proc,       320,  128,  0,    0,    255,  8,    0,       0,          0,             0,       "Driver:",        NULL, NULL  },
   { d_text_proc,       352,  152,  0,    0,    255,  8,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { d_text_proc,       320,  192,  0,    0,    255,  8,    0,       0,          0,             0,       "Description:",   NULL, NULL  },
   { d_text_proc,       352,  216,  0,    0,    255,  8,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { instlist_proc,     16,   56,   257,  356,  255,  0,    32,      D_EXIT,     0,             0,       instlist_getter,  NULL, NULL  },
   { piano_proc,        2,    440,  636,  40,   255,  0,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { d_text_proc,       320,  320,  0,    0,    255,  8,    0,       0,          0,             0,       "Volume:",        NULL, NULL  },
   { d_edit_proc,       392,  320,  64,   16,   255,  8,    0,       0,          3,             3,       volume_str,       NULL, NULL  },
   { d_text_proc,       320,  352,  0,    0,    255,  8,    0,       0,          0,             0,       "Pan:",           NULL, NULL  },
   { d_edit_proc,       392,  352,  64,   16,   255,  8,    0,       0,          3,             2,       pan_str,          NULL, NULL  },
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  }
};



int main(void)
{
   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_mouse();
   install_timer();

   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error setting graphics mode\n%s\n", allegro_error);
      return 1;
   }

   if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error initialising sound\n%s\n", allegro_error);
      return 1;
   }

   set_palette(desktop_palette);

   #ifdef MIDI_DIGMID
      if (midi_driver->id == MIDI_DIGMID)
	 textout_centre_ex(screen, font, "Loading patch set...", SCREEN_W/2, SCREEN_H/2, 255, 0);
   #endif

   load_midi_patches();

   thedialog[DRIVER_STR].dp = (void*)midi_driver->name;
   thedialog[DESC_STR].dp = (void*)midi_driver->desc;

   do_dialog(thedialog, INSTLIST);

   return 0;
}

END_OF_MAIN()
