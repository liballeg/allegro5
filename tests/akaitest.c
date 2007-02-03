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
 *      Audio input test program for the Allegro library.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>
#include <math.h>

#include "allegro.h"



/* the current sound */
SAMPLE *the_sample = NULL;


/* table of voice numbers for each MIDI key */
int voice[128];


/* lookup table for the MIDI pitch -> sample frequency conversion */
float pitch[128]; 


/* ring buffer for storing MIDI input */
#define MIDI_BUFFER_SIZE    4096

volatile int midi_buffer_head = 0; 
volatile int midi_buffer_tail = 0;
volatile unsigned char midi_buffer[MIDI_BUFFER_SIZE];


/* the current MIDI command and data bytes */
int midi_cmd = -1;
int midi_data1 = -1;
int midi_data2 = -1;



/* interrupt hook for dealing with MIDI input */
void midi_input_callback(unsigned char data)
{
   midi_buffer[midi_buffer_tail] = data;
   midi_buffer_tail++;
   if (midi_buffer_tail >= MIDI_BUFFER_SIZE)
      midi_buffer_tail = 0;
}

END_OF_FUNCTION(midi_input_callback);



/* checks if there is any data in the MIDI input buffer */
int midi_waiting(void)
{
   if (midi_buffer_head == midi_buffer_tail)
      return 0;
   else
      return 1;
}



/* reads a byte from the MIDI input buffer */
int get_midi(void)
{
   int ret;

   if (midi_buffer_head == midi_buffer_tail)
      return -1;

   ret = midi_buffer[midi_buffer_head];
   midi_buffer_head++;
   if (midi_buffer_head >= MIDI_BUFFER_SIZE)
      midi_buffer_head = 0;

   return ret; 
}



/* trigger a sample */
void start_note(int note, int vol)
{
   if (voice[note] >= 0) {
      voice_stop(voice[note]);
      deallocate_voice(voice[note]);
   }

   voice[note] = allocate_voice(the_sample);

   if (voice[note] >= 0) {
      voice_set_frequency(voice[note], MIN(pitch[note]*the_sample->freq, 0x7FFFF));
      voice_set_volume(voice[note], vol);
      voice_start(voice[note]);
   }
}



/* kill off a playing sample */
void stop_note(int note)
{
   if (voice[note] >= 0) {
      voice_stop(voice[note]);
      deallocate_voice(voice[note]);
      voice[note] = -1;
   }
}



/* load samples in from disk */
int load_proc(int msg, DIALOG *d, int c)
{
   int ret = d_button_proc(msg, d, c);

   if (ret & D_CLOSE) {
      static char name[80*6] = EMPTY_STRING;  /* 80 chars * max UTF8 char width */

      if (file_select_ex("Load Sample", name, "wav;voc", sizeof(name), 0, 0)) {
	 if (the_sample)
	    destroy_sample(the_sample);

	 the_sample = load_sample(name);

	 if (!the_sample)
	    alert("Error reading sample", NULL, NULL, "What a pity", NULL, 13, 0);
      }

      return D_REDRAW;
   }

   return ret;
}



char record_length[16] = "5.0";
char record_freq[16] = "";

int waveform_proc(int msg, DIALOG *d, int c);



DIALOG record_dialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)              (dp2) (dp3) */
   { d_shadow_box_proc, 160,  120,  321,  265,  255,  0,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { d_ctext_proc,      320,  136,  0,    0,    255,  0,    0,       0,          0,             0,       "Record Sample",  NULL, NULL  },
   { d_button_proc,     208,  336,  97,   25,   255,  0,    13,      D_EXIT,     0,             0,       "OK",             NULL, NULL  },
   { d_button_proc,     338,  336,  97,   25,   255,  0,    27,      D_EXIT,     0,             0,       "Cancel",         NULL, NULL  },
   { d_text_proc,       208,  176,  0,    0,    255,  0,    0,       0,          0,             0,       "Length:",        NULL, NULL  },
   { d_edit_proc,       277,  176,  64,   8,    255,  0,    0,       0,          15,            0,       record_length,    NULL, NULL  },
   { d_text_proc,       208,  200,  0,    0,    255,  0,    0,       0,          0,             0,       "Freq:",          NULL, NULL  },
   { d_edit_proc,       277,  200,  64,   8,    255,  0,    0,       0,          15,            0,       record_freq,      NULL, NULL  },
   { d_text_proc,       208,  224,  0,    0,    255,  0,    0,       0,          0,             0,       "16 bit:",        NULL, NULL  },
   { d_check_proc,      272,  221,  17,   13,   255,  0,    0,       0,          0,             0,       "",               NULL, NULL  },
   { d_text_proc,       208,  248,  0,    0,    255,  0,    0,       0,          0,             0,       "Stereo:",        NULL, NULL  },
   { d_check_proc,      272,  245,  17,   13,   255,  0,    0,       0,          0,             0,       "",               NULL, NULL  },
   { d_radio_proc,      368,  176,  49,   13,   255,  0,    0,       D_SELECTED, 1,             0,       "Mic",            NULL, NULL  },
   { d_radio_proc,      368,  200,  49,   13,   255,  0,    0,       0,          1,             0,       "Line",           NULL, NULL  },
   { d_radio_proc,      368,  224,  49,   13,   255,  0,    0,       0,          1,             0,       "CD",             NULL, NULL  },
   { waveform_proc,     208,  280,  226,  32,   1,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  }
};



#define RECORD_OK       2
#define RECORD_CANCEL   3
#define RECORD_LENGTH   5
#define RECORD_FREQ     7
#define RECORD_16BIT    9
#define RECORD_STEREO   11
#define RECORD_MIC      12
#define RECORD_LINE     13
#define RECORD_CD       14



int waveform_size = 0;
unsigned char *waveform_buffer = NULL;



/* display the input waveform for the sampling dialog */
int waveform_proc(int msg, DIALOG *d, int c)
{
   int rate, source, i;
   static int last_source = -1;

   switch (msg) {

      case MSG_START:
	 rate = get_sound_input_cap_parm(8000, 8, FALSE);
	 if (rate > 0)
	    rate = 8000;
	 else if (rate < 0)
	    rate = -rate;
	 else
	    return D_O_K;

	 waveform_size = start_sound_input(rate, 8, FALSE);

	 if (waveform_size > 0) {
	    waveform_buffer = malloc(waveform_size);
	    memset(waveform_buffer, 0x80, waveform_size);
	 }
	 break;

      case MSG_END:
	 if (waveform_buffer) {
	    stop_sound_input();
	    free(waveform_buffer);
	    waveform_buffer = NULL;
	    waveform_size = 0;
	 }
	 break;

      case MSG_DRAW:
	 if (waveform_buffer) {
	    for (i=0; i<MIN(d->w, waveform_size); i++) {
	       if (waveform_buffer[i] < 0x80) {
		  vline(screen, d->x+i, d->y, d->y+waveform_buffer[i]*d->h/255-1, d->bg);
		  vline(screen, d->x+i, d->y+waveform_buffer[i]*d->h/255, d->y+d->h/2, d->fg);
		  vline(screen, d->x+i, d->y+d->h/2+1, d->y+d->h, d->bg);
	       }
	       else {
		  vline(screen, d->x+i, d->y, d->y+d->h/2-1, d->bg);
		  vline(screen, d->x+i, d->y+d->h/2, d->y+waveform_buffer[i]*d->h/255, d->fg);
		  vline(screen, d->x+i, d->y+waveform_buffer[i]*d->h/255+1, d->y+d->h, d->bg);
	       }
	    }
	 }
	 break;

      case MSG_IDLE:
	 if (waveform_buffer) {
	    if (read_sound_input(waveform_buffer)) {
	       freeze_mouse_flag = TRUE;
	       poll_mouse();
	       object_message(d, MSG_DRAW, 0);
	       freeze_mouse_flag = FALSE;
	    }
	 }

	 if (record_dialog[RECORD_MIC].flags & D_SELECTED)
	    source = SOUND_INPUT_MIC;
	 else if (record_dialog[RECORD_LINE].flags & D_SELECTED)
	    source = SOUND_INPUT_LINE;
	 else
	    source = SOUND_INPUT_CD;

	 if (source != last_source) {
	    set_sound_input_source(source);
	    last_source = source;
	 }
	 break;
   }

   return D_O_K;
}



/* main sample recording function */
int record_proc(int msg, DIALOG *d, int c)
{
   int ret = d_button_proc(msg, d, c);
   int sel = -1;
   int bits;
   int stereo;
   int freq, freq2;
   int buffer_size;
   int bytes_per_sample;
   int sample_len;
   int sample_todo;
   int sample_todo_orig;
   char *sample_data;
   float length;

   if (ret & D_CLOSE) {
      bits = get_sound_input_cap_bits();
      stereo = get_sound_input_cap_stereo();

      if (!bits) {
	 alert("The current audio input driver does", "not support sample recording", NULL, "I'll change it then", NULL, 13, 0);
	 return D_REDRAW;
      }

      if (bits&16)
	 record_dialog[RECORD_16BIT].flags |= D_SELECTED;
      else
	 record_dialog[RECORD_16BIT].flags &= ~D_SELECTED;

      if (stereo)
	 record_dialog[RECORD_STEREO].flags |= D_SELECTED;
      else
	 record_dialog[RECORD_STEREO].flags &= ~D_SELECTED;

      if (!record_freq[0])
	 sprintf(record_freq, "%d", get_sound_input_cap_rate(((bits&16) ? 16 : 8), stereo));

      retry:

      /* ask the user for settings */
      if (do_dialog(record_dialog, sel) == RECORD_OK) {
	 length = atof(record_length);
	 freq = atof(record_freq);
	 bits = (record_dialog[RECORD_16BIT].flags & D_SELECTED) ? 16 : 8;
	 stereo = (record_dialog[RECORD_STEREO].flags & D_SELECTED) ? TRUE : FALSE;

	 /* make sure we got a valid length */
	 if ((length < 0.1) || (length > 30)) {
	    alert("Please enter a length", "between 0.1 and 30 seconds", NULL, "Understood", NULL, 13, 0);
	    sel = RECORD_LENGTH;
	    goto retry;
	 }

	 /* make sure we got a valid frequency */
	 if ((freq < 4000) || (freq > 50000)) {
	    alert("Please enter a sampling frequency", "between 4000 and 50000", NULL, "Understood", NULL, 13, 0);
	    sel = RECORD_FREQ;
	    goto retry;
	 }

	 /* check that the card likes this frequency */
	 freq2 = get_sound_input_cap_parm(freq, bits, stereo);

	 if (freq2 == 0) {
	    alert("The current input driver does", "not support these settings", NULL, "Drat", NULL, 13, 0);
	    sel = RECORD_16BIT;
	    goto retry;
	 }

	 if (freq2 < 0) {
	    sprintf(record_freq, "%d", -freq2);
	    alert("Invalid sampling frequency", "Suggested alternative:", record_freq, "Understood", NULL, 13, 0);
	    sel = RECORD_FREQ;
	    goto retry;
	 }

	 /* set the sample source */
	 if (record_dialog[RECORD_MIC].flags & D_SELECTED)
	    set_sound_input_source(SOUND_INPUT_MIC);
	 else if (record_dialog[RECORD_LINE].flags & D_SELECTED)
	    set_sound_input_source(SOUND_INPUT_LINE);
	 else if (record_dialog[RECORD_CD].flags & D_SELECTED)
	    set_sound_input_source(SOUND_INPUT_CD);

	 /* draw progress bar */
	 show_mouse(NULL);

	 rect(screen, 95, 175, 545, 273, 255);
	 rectfill(screen, 96, 176, 544, 272, 0);
	 rect(screen, 127, 235, 513, 257, 255);
	 rectfill(screen, 128, 236, 512, 256, 8);
	 textout_centre_ex(screen, font, "Recording...", 320, 200, 255, -1);

	 /* start the input! */
	 buffer_size = start_sound_input(freq, bits, stereo);

	 if (buffer_size <= 0) {
	    alert("Error recording sample", NULL, NULL, "Drat", NULL, 13, 0);
	    destroy_sample(the_sample);
	    the_sample = NULL;
	    show_mouse(screen);
	    return D_REDRAW;
	 }

	 /* round the sample length to an even number of buffer transfers.
	  * Really the recording code ought to handle odd amounts of data 
	  * left over at the end, but it is much easier like this :-)
	  */
	 bytes_per_sample = ((bits==8) ? 1 : sizeof(short)) * ((stereo) ? 2 : 1);
	 sample_len = length * freq;
	 sample_len = MAX((((sample_len * bytes_per_sample) / buffer_size) * buffer_size), buffer_size) / bytes_per_sample;
	 sample_todo = sample_todo_orig = sample_len * bytes_per_sample;

	 if (the_sample)
	    destroy_sample(the_sample);

	 /* create a new sample to hold our data */
	 the_sample = create_sample(bits, stereo, freq, sample_len);
	 sample_data = (char *)the_sample->data;

	 /* discard the first input buffer, to prevent clicks */
	 do {
	 } while (!read_sound_input(sample_data));

	 /* finally, record the sound */
	 while (sample_todo >= buffer_size) {
	    if (read_sound_input(sample_data)) {
	       sample_data += buffer_size;
	       sample_todo -= buffer_size;
	       rectfill(screen, 128, 236, 128+(sample_todo_orig-sample_todo)*384/sample_todo_orig, 256, 1);
	    }
	 }

	 stop_sound_input();

	 show_mouse(screen);
      }

      return D_REDRAW;
   }

   return ret;
}



/* helper for redrawing a specific part of the keyboard object */
void redraw_keyboard(DIALOG *d, int c)
{
   set_clip_rect(screen, d->x+(c-36)*12-6, d->y, d->x+(c-36)*12+18, d->y+d->h);
   object_message(d, MSG_DRAW, 0);
   set_clip_rect(screen, 0, 0, SCREEN_W-1, SCREEN_H-1);
}



/* GUI interface for playing sounds */
int piano_proc(int msg, DIALOG *d, int c)
{
   static char blackkey[12] = 
   { 
      FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, 
      TRUE, FALSE, TRUE, FALSE, TRUE, FALSE 
   };

   int i, l, r;

   switch (msg) {

      case MSG_START:
	 d->d1 = -1;
	 break;

      case MSG_DRAW:
	 /* display the keyboard graphic */
	 for (i=0; i<d->w/12; i++) {
	    if (!blackkey[i%12]) {
	       l = i*12;
	       r = (i+1)*12;
	       if (i > 0 && blackkey[(i-1)%12])
		  l -= 6;
	       if (blackkey[(i+1)%12])
		  r += 6;
	       rectfill(screen, d->x+l+1, d->y+1, d->x+r-1, d->y+d->h-1, (voice[i+36] >= 0) ? 1 : d->bg);
	       rect(screen, d->x+l, d->y, d->x+r, d->y+d->h, d->fg);
	    }
	 }
	 for (i=0; i<d->w/12; i++) {
	    if (blackkey[i%12]) {
	       l = i*12 + 1;
	       r = (i+1)*12 - 1;
	       rectfill(screen, d->x+l, d->y+d->h/4, d->x+r, d->y+d->h, (voice[i+36] >= 0) ? 1 : d->fg);
	    }
	 }
	 break;

      case MSG_CLICK:
	 /* handle mouse clicks (playing sounds from the GUI) */
	 if (!the_sample) {
	    alert("You must read in or record a", "sample before you can play it", NULL, "Sorry. I won't do it again", NULL, 13, 0);
	    return D_O_K;
	 }

	 do {
	    poll_mouse();
	    i = ((mouse_x - d->x) / 12) + 36;

	    if (i != d->d1) {
	       if (d->d1 >= 0) {
		  stop_note(d->d1);
		  redraw_keyboard(d, d->d1);
		  d->d1 = -1;
	       }

	       d->d1 = i;
	       start_note(d->d1, 255);
	       redraw_keyboard(d, d->d1);
	    }
	 } while (mouse_b);

	 if (d->d1 >= 0) {
	    stop_note(d->d1);
	    redraw_keyboard(d, d->d1);
	    d->d1 = -1;
	 }
	 break;

      case MSG_IDLE:
	 /* check for MIDI input */
	 if (midi_waiting()) {
	    i = get_midi();

	    if (i & 0x80) {
	       /* start of a new MIDI command */
	       midi_cmd = i&0xF0;
	       midi_data1 = -1;
	       midi_data2 = -1;
	    }
	    else if (midi_data1 < 0) {
	       /* got the first parameter */
	       midi_data1 = i;
	    }
	    else if (midi_data2 < 0) {
	       /* got the second parameter */
	       midi_data2 = i;

	       if (the_sample) {
		  if ((midi_cmd == 0x80) || ((midi_cmd == 0x90) && (midi_data2 == 0))) {
		     /* note off */
		     stop_note(midi_data1);
		     redraw_keyboard(d, midi_data1);
		  }
		  else if (midi_cmd == 0x90) {
		     /* note on */
		     start_note(midi_data1, midi_data2);
		     redraw_keyboard(d, midi_data1);
		  }
	       }

	       midi_data1 = -1;
	       midi_data2 = -1;
	    }
	 }
	 break;
   }

   return D_O_K;
}



DIALOG the_dialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)              (dp2) (dp3) */
   { d_clear_proc,      0,    0,    0,    0,    0,    8,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { d_ctext_proc,      320,  4,    0,    0,    255,  8,    0,       0,          0,             0,       "Audio input test program for Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR, NULL, NULL },
   { d_ctext_proc,      320,  20,   0,    0,    255,  8,    0,       0,          0,             0,       "By Shawn Hargreaves, " ALLEGRO_DATE_STR, NULL, NULL },
   { d_text_proc,       224,  88,   0,    0,    255,  8,    0,       0,          0,             0,       "MIDI Input:",    NULL, NULL  },
   { d_text_proc,       328,  88,   0,    0,    255,  8,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { d_text_proc,       216,  112,  0,    0,    255,  8,    0,       0,          0,             0,       "Audio Input:",   NULL, NULL  },
   { d_text_proc,       328,  112,  0,    0,    255,  8,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { d_text_proc,       208,  136,  0,    0,    255,  8,    0,       0,          0,             0,       "Audio Output:",  NULL, NULL  },
   { d_text_proc,       328,  136,  0,    0,    255,  8,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { load_proc,         160,  232,  129,  25,   255,  0,    0,       D_EXIT,     0,             0,       "Load Sample",    NULL, NULL  },
   { record_proc,       352,  232,  129,  25,   255,  0,    0,       D_EXIT,     0,             0,       "Record Sample",  NULL, NULL  },
   { d_button_proc,     256,  320,  129,  25,   255,  0,    0,       D_EXIT,     0,             0,       "Quit",           NULL, NULL  },
   { piano_proc,        2,    440,  636,  40,   255,  0,    0,       0,          0,             0,       NULL,             NULL, NULL  },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,             NULL, NULL  }
};



int main(void)
{
   int i;

   for (i=0; i<128; i++)
      voice[i] = -1;

   pitch[127] = 48;
   for (i=126; i>=0; i--)
      pitch[i] = pitch[i+1] / pow(2.0, 1.0/12.0);

   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_mouse();
   install_timer();

   if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) != 0) {
      allegro_message("\nError initialising sound\n%s\n", allegro_error);
      return 1;
   }

   if (install_sound_input(DIGI_AUTODETECT, MIDI_AUTODETECT) != 0) {
      allegro_message("\nError initialising sound input\n%s\n\n"
		      "Try changing the digi_input_card and midi_input_card settings in your\n"
		      "allegro.cfg, in particular disabling (set to zero) one of those drivers.\n",
		      allegro_error);
      return 1;
   }

   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error setting graphics mode\n%s\n", allegro_error);
      return 1;
   }

   set_palette(desktop_palette);

   LOCK_VARIABLE(midi_buffer_head);
   LOCK_VARIABLE(midi_buffer_tail);
   LOCK_VARIABLE(midi_buffer);
   LOCK_FUNCTION(midi_input_callback);

   midi_recorder = midi_input_callback;

   the_dialog[4].dp = (void*)midi_input_driver->name;
   the_dialog[6].dp = (void*)digi_input_driver->name;
   the_dialog[8].dp = (void*)digi_driver->name;

   do_dialog(the_dialog, -1);

   if (the_sample)
      destroy_sample(the_sample);

   return 0;
}

END_OF_MAIN()
