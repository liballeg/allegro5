/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    This file is quite possibly the most entirely pointless piece of
 *    code that I have ever written. I didn't want to include any external
 *    data, you see, so everything has to work entirely from C source,
 *    which means generating all the sounds from code. I don't know why
 *    I'm bothering, because this probably won't sound too good :-)
 */

#include <math.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>

#include "speed.h"

#ifndef M_PI
#define M_PI         3.14159265358979323846
#endif



/* generated sample waveforms */
static ALLEGRO_SAMPLE *zap;
static ALLEGRO_SAMPLE *bang;
static ALLEGRO_SAMPLE *bigbang;
static ALLEGRO_SAMPLE *ping;
static ALLEGRO_SAMPLE *sine;
static ALLEGRO_SAMPLE *square;
static ALLEGRO_SAMPLE *saw;
static ALLEGRO_SAMPLE *bd;
static ALLEGRO_SAMPLE *snare;
static ALLEGRO_SAMPLE *hihat;



#define RAND   (((float)(rand() & 255) / 255.0) - 0.5)



/* format: pitch, delay. Zero pitch = silence */


static int part_1[] =
{
   /* tune A bass */

   /* C */  45, 6, 0, 2,
   /* C */  45, 6, 0, 10,
   /* C */  45, 6, 0, 2,
   /* C */  45, 8, 0, 4,
   /* C */  45, 8, 0, 4,
   /* C */  45, 6, 0, 2,

   /* F */  50, 6, 0, 2,
   /* F */  50, 6, 0, 10,
   /* F */  50, 6, 0, 2,
   /* F */  50, 8, 0, 4,
   /* F */  50, 8, 0, 4,
   /* F */  50, 6, 0, 2,

   /* Ab */ 53, 6, 0, 2,
   /* Ab */ 53, 6, 0, 10,
   /* Ab */ 53, 6, 0, 2,
   /* G */  52, 8, 0, 4,
   /* G */  52, 8, 0, 4,
   /* G */  52, 6, 0, 2,

   /* C */  45, 6, 0, 2,
   /* C */  45, 6, 0, 10,
   /* C */  45, 6, 0, 2,
   /* C */  45, 8, 0, 4,
   /* C */  45, 8, 0, 4,
   /* C */  45, 6, 0, 2,


   /* tune A repeat bass */

   /* C */  45, 6, 0, 2,
   /* C */  45, 6, 0, 10,
   /* C */  45, 6, 0, 2,
   /* C */  45, 8, 0, 4,
   /* C */  45, 8, 0, 4,
   /* C */  45, 6, 0, 2,

   /* F */  50, 6, 0, 2,
   /* F */  50, 6, 0, 10,
   /* F */  50, 6, 0, 2,
   /* F */  50, 8, 0, 4,
   /* F */  50, 8, 0, 4,
   /* F */  50, 6, 0, 2,

   /* Ab */ 53, 6, 0, 2,
   /* Ab */ 53, 6, 0, 10,
   /* Ab */ 53, 6, 0, 2,
   /* G */  52, 8, 0, 4,
   /* G */  52, 8, 0, 4,
   /* G */  52, 6, 0, 2,

   /* C */  45, 6, 0, 2,
   /* C */  45, 6, 0, 10,
   /* C */  45, 6, 0, 2,
   /* C */  45, 8, 0, 4,
   /* C */  45, 8, 0, 4,
   /* C */  45, 6, 0, 2,


   /* tune B bass */

   /* C */  45, 52, 0, 4,
   /* C */  45, 6, 0, 2,

   /* D */  47, 52, 0, 4,
   /* D */  47, 6, 0, 2,

   /* Eb */ 48, 14, 0, 2,
   /* F */  50, 14, 0, 2,
   /* G */  52, 14, 0, 2,
   /* Ab */ 53, 14, 0, 2,

   /* Bb */ 55, 6, 0, 2,
   /* Bb */ 55, 6, 0, 10,
   /* Bb */ 55, 6, 0, 2,
   /* C */  57, 14, 0, 2,
   /* G */  52, 14, 0, 2,


   /* tune B repeat bass */

   /* C */  45, 6, 0, 2,
   /* C */  45, 6, 0, 10,
   /* C */  45, 6, 0, 2,
   /* C */  45, 8, 0, 4,
   /* C */  45, 8, 0, 4,
   /* C */  45, 6, 0, 2,

   /* D */  47, 6, 0, 2,
   /* D */  47, 6, 0, 10,
   /* D */  47, 6, 0, 2,
   /* D */  47, 8, 0, 4,
   /* D */  47, 8, 0, 4,
   /* D */  47, 6, 0, 2,

   /* Eb */ 48, 14, 0, 2,
   /* F */  50, 14, 0, 2,
   /* G */  52, 14, 0, 2,
   /* Ab */ 53, 14, 0, 2,

   /* Bb */ 55, 6, 0, 2,
   /* Bb */ 55, 6, 0, 10,
   /* Bb */ 55, 6, 0, 2,
   /* C */  57, 14, 0, 2,
   /* G */  52, 14, 0, 2,


   0, 0
};



static int part_2[] =
{
   /* tune A harmony */

   /* C */  57, 30, 0, 2,
   /* Eb */ 60, 14, 0, 2,
   /* C */  57, 14, 0, 2,

   /* F */  62, 30, 0, 2,
   /* Ab */ 65, 14, 0, 2,
   /* F */  62, 14, 0, 2,

   /* Ab */ 65, 30, 0, 2,
   /* G */  64, 14, 0, 2,
   /* G */  52, 14, 0, 2,

   /* C */  57, 30, 0, 2,
   /* Eb */ 60, 14, 0, 2,
   /* Eb */ 60, 3,  0, 1,
   /* D */  59, 3,  0, 1,
   /* Db */ 58, 3,  0, 1,
   /* C */  57, 3,  0, 1,


   /* tune A repeat harmony */

   /* C */  57, 3, 0, 1,
   /* Eb */ 60, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* C */  69, 6, 0, 2,
   /* C */  57, 3, 0, 1,
   /* Eb */ 60, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* C */  69, 6, 0, 6,
   /* C */  57, 3, 0, 1,
   /* Eb */ 60, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* C */  69, 6, 0, 2,

   /* C */  57, 3, 0, 1,
   /* F */  62, 3, 0, 1,
   /* Ab */ 65, 3, 0, 1,
   /* C */  69, 6, 0, 2,
   /* C */  57, 3, 0, 1,
   /* F */  62, 3, 0, 1,
   /* Ab */ 65, 3, 0, 1,
   /* C */  69, 6, 0, 6,
   /* C */  57, 3, 0, 1,
   /* F */  62, 3, 0, 1,
   /* Ab */ 65, 3, 0, 1,
   /* C */  69, 6, 0, 2,

   /* C */  57, 3, 0, 1,
   /* Eb */ 60, 3, 0, 1,
   /* Ab */ 65, 3, 0, 1,
   /* C */  69, 6, 0, 2,
   /* C */  57, 3, 0, 1,
   /* Eb */ 60, 3, 0, 1,
   /* Ab */ 65, 3, 0, 1,
   /* C */  69, 6, 0, 6,
   /* C */  57, 3, 0, 1,
   /* D */  59, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* D */  71, 6, 0, 2,

   /* C */  57, 3, 0, 1,
   /* Eb */ 60, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* C */  69, 6, 0, 2,
   /* C */  57, 3, 0, 1,
   /* Eb */ 60, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* C */  69, 6, 0, 6,
   /* C */  57, 3, 0, 1,
   /* Eb */ 60, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* C */  69, 6, 0, 2,


   /* tune B melody */

   /* C */  57, 3, 0, 1,
   /* Eb */ 60, 3, 0, 1,
   /* F */  62, 3, 0, 1,
   /* G */  64, 5, 0, 3,
   /* Ab */ 65, 3, 0, 1,
   /* F */  62, 5, 0, 3,
   /* G */  64, 3, 0, 1,
   /* Eb */ 60, 5, 0, 3,
   /* F */  62, 3, 0, 1,
   /* D */  59, 5, 0, 3,
   /* Eb */ 60, 5, 0, 3,

   /* D */  59, 3, 0, 1,
   /* F# */ 63, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* A */  66, 5, 0, 3,
   /* Bb */ 67, 3, 0, 1,
   /* G */  64, 5, 0, 3,
   /* A */  66, 3, 0, 1,
   /* F# */ 63, 5, 0, 3,
   /* G */  64, 3, 0, 1,
   /* Eb */ 60, 5, 0, 3,
   /* F# */ 63, 2, 0, 1,
   /* Eb */ 60, 2, 0, 1,
   /* D */  59, 1, 0, 1,

   /* C */  57, 3, 0, 1,
   /* Eb */ 60, 3, 0, 1,
   /* F */  62, 3, 0, 1,
   /* G */  64, 5, 0, 3,
   /* Ab */ 65, 3, 0, 1,
   /* F */  62, 5, 0, 3,
   /* G */  64, 3, 0, 1,
   /* Eb */ 60, 5, 0, 3,
   /* F */  62, 3, 0, 1,
   /* D */  59, 5, 0, 3,
   /* Eb */ 60, 5, 0, 3,

   /* D */  59, 3, 0, 1,
   /* E */  61, 3, 0, 1,
   /* F# */ 63, 3, 0, 1,
   /* Ab */ 65, 5, 0, 3,
   /* F# */ 63, 3, 0, 1,
   /* Ab */ 65, 3, 0, 1,
   /* Bb */ 67, 3, 0, 1,
   /* C */  69, 8, 0, 24,


   /* tune B repeat melody */

   /* C */  57, 3, 0, 1,
   /* Eb */ 60, 3, 0, 1,
   /* F */  62, 3, 0, 1,
   /* G */  64, 5, 0, 3,
   /* Ab */ 65, 3, 0, 1,
   /* F */  62, 5, 0, 3,
   /* G */  64, 3, 0, 1,
   /* Eb */ 60, 5, 0, 3,
   /* F */  62, 3, 0, 1,
   /* D */  59, 5, 0, 3,
   /* Eb */ 60, 5, 0, 3,

   /* D */  59, 3, 0, 1,
   /* F# */ 63, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* A */  66, 5, 0, 3,
   /* Bb */ 67, 3, 0, 1,
   /* G */  64, 5, 0, 3,
   /* A */  66, 3, 0, 1,
   /* F# */ 63, 5, 0, 3,
   /* G */  64, 3, 0, 1,
   /* Eb */ 60, 5, 0, 3,
   /* F# */ 63, 2, 0, 1,
   /* Eb */ 60, 2, 0, 1,
   /* D */  59, 1, 0, 1,

   /* C */  57, 3, 0, 1,
   /* Eb */ 60, 3, 0, 1,
   /* F */  62, 3, 0, 1,
   /* G */  64, 5, 0, 3,
   /* Ab */ 65, 3, 0, 1,
   /* F */  62, 5, 0, 3,
   /* G */  64, 3, 0, 1,
   /* Eb */ 60, 5, 0, 3,
   /* F */  62, 3, 0, 1,
   /* D */  59, 5, 0, 3,
   /* Eb */ 60, 5, 0, 3,

   /* D */  59, 3, 0, 1,
   /* E */  61, 3, 0, 1,
   /* F# */ 63, 3, 0, 1,
   /* Ab */ 65, 5, 0, 3,
   /* F# */ 63, 3, 0, 1,
   /* Ab */ 65, 3, 0, 1,
   /* Bb */ 67, 3, 0, 1,
   /* C */  69, 8, 0, 24,


   0, 0
};



static int part_3[] =
{
   /* tune A melody */

   /* G */  64, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* Bb */ 67, 3, 0, 1,
   /* C */  57, 5, 0, 3,
   /* C */  57, 3, 0, 1,
   /* D */  59, 3, 0, 1,
   /* C */  57, 3, 0, 1,
   /* Eb */ 60, 5, 0, 3,
   /* Eb */ 60, 5, 0, 3,
   /* D */  59, 4, 0, 1,
   /* C */  57, 4, 0, 1,
   /* Bb */ 55, 5, 0, 5,

   /* G */  64, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* Bb */ 67, 3, 0, 1,
   /* C */  57, 8, 0, 4,
   /* D */  59, 5, 0, 3,
   /* C */  57, 5, 0, 3,
   /* Eb */ 60, 5, 0, 3,
   /* Eb */ 60, 3, 0, 1,
   /* D */  59, 3, 0, 1,
   /* C */  57, 3, 0, 9,

   /* G */  64, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* Bb */ 67, 3, 0, 1,
   /* C */  57, 5, 0, 3,
   /* D */  59, 5, 0, 3,
   /* C */  57, 5, 0, 3,
   /* Eb */ 60, 5, 0, 3,
   /* Eb */ 60, 3, 0, 1,
   /* D */  59, 3, 0, 1,
   /* C */  57, 3, 0, 1,

   /* G */  64, 5, 0, 3,
   /* G */  64, 5, 0, 3,
   /* Bb */ 67, 5, 0, 3,
   /* C */  57, 8, 0, 32,


   /* tune A repeat melody */

   /* G */  64, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* Bb */ 67, 3, 0, 1,
   /* C */  57, 5, 0, 3,
   /* C */  57, 3, 0, 1,
   /* D */  59, 3, 0, 1,
   /* C */  57, 3, 0, 1,
   /* Eb */ 60, 5, 0, 3,
   /* Eb */ 60, 5, 0, 3,
   /* D */  59, 4, 0, 1,
   /* C */  57, 4, 0, 1,
   /* Bb */ 55, 5, 0, 5,

   /* G */  64, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* Bb */ 67, 3, 0, 1,
   /* C */  57, 8, 0, 4,
   /* D */  59, 5, 0, 3,
   /* C */  57, 5, 0, 3,
   /* Eb */ 60, 5, 0, 3,
   /* Eb */ 60, 3, 0, 1,
   /* D */  59, 3, 0, 1,
   /* C */  57, 3, 0, 9,

   /* G */  64, 3, 0, 1,
   /* G */  64, 3, 0, 1,
   /* Bb */ 67, 3, 0, 1,
   /* C */  57, 5, 0, 3,
   /* D */  59, 5, 0, 3,
   /* C */  57, 5, 0, 3,
   /* Eb */ 60, 5, 0, 3,
   /* Eb */ 60, 3, 0, 1,
   /* D */  59, 3, 0, 1,
   /* C */  57, 3, 0, 1,

   /* G */  64, 5, 0, 3,
   /* G */  64, 5, 0, 3,
   /* Bb */ 67, 5, 0, 3,
   /* C */  57, 8, 0, 32,


   /* tune B harmony */

   /* C */  57, 52, 0, 4,
   /* C */  57, 6, 0, 2,

   /* D */  59, 52, 0, 4,
   /* D */  59, 6, 0, 2,

   /* Eb */ 60, 14, 0, 2,
   /* F */  62, 14, 0, 2,
   /* G */  64, 14, 0, 2,
   /* Ab */ 65, 14, 0, 2,

   /* Bb */ 67, 6, 0, 2,
   /* Bb */ 67, 6, 0, 10,
   /* Bb */ 67, 6, 0, 2,
   /* C */  69, 14, 0, 2,
   /* G */  64, 2,
   /* F# */ 63, 2,
   /* G */  64, 2,
   /* Ab */ 65, 2,
   /* G */  64, 3, 0, 1,
   /* D */  59, 3, 0, 9,


   /* tune B repeat harmony */

   /* C */  57, 11, 0, 1,
   /* D */  59, 4,  0, 8,
   /* C */  57, 11, 0, 1,
   /* D */  59, 5,  0, 3,
   /* C */  57, 7,  0, 1,
   /* D */  59, 3,  0, 9,

   /* D */  59, 11, 0, 1,
   /* Eb */ 60, 4,  0, 8,
   /* D */  59, 11, 0, 1,
   /* Eb */ 60, 5,  0, 3,
   /* D */  59, 7,  0, 1,
   /* Eb */ 60, 3,  0, 9,

   /* Eb */ 60, 11, 0, 1,
   /* F */  62, 5,  0, 7,
   /* G */  64, 11, 0, 1,
   /* Ab */ 65, 10, 0, 2,
   /* Bb */ 67, 7,  0, 9,

   /* Bb */ 67, 14, 0, 2,
   /* Bb */ 67, 6,  0, 2,
   /* C */  69, 14, 0, 2,
   /* G */  64, 2,
   /* F# */ 63, 2,
   /* G */  64, 2,
   /* Ab */ 65, 2,
   /* G */  64, 3, 0, 1,
   /* D */  59, 3, 0, 1,


   0, 0
};



static int part_4[] =
{
   /* tune A drums */

   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 2, 4, 2, 4, 2, 2, 2, 2,


   /* tune A repeat drums */

   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 2, 4, 1, 4, 2, 2, 2, 2,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 2, 4, 1, 4, 2, 2, 2, 2,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 2, 4, 1, 4, 2, 2, 2, 2,
   1, 4, 3, 4, 2, 4, 3, 4,
   1, 4, 3, 4, 2, 4, 3, 4,
   2, 4, 1, 4, 1, 4, 2, 2, 2, 2,
   1, 4, 1, 4, 2, 2, 2, 2, 2, 2, 2, 2,


   /* tune B drums */

   1, 16, 1, 8, 1, 8,
   1, 16, 1, 8, 1, 4, 1, 4,
   1, 16, 1, 8, 1, 8,
   1, 16, 1, 8, 1, 4, 1, 4,
   1, 16, 1, 16, 1, 16, 1, 8, 1, 8,
   1, 8, 1, 8, 1, 4, 1, 4, 1, 4, 1, 4,
   1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2,
   1, 4, 2, 4, 2, 4, 2, 2, 2, 2,


   /* tune B repeat drums */
   1, 8, 2, 8, 1, 12, 1, 4,
   1, 16, 1, 4, 2, 4, 2, 4, 2, 2, 2, 2,
   1, 8, 2, 8, 1, 12, 1, 4,
   1, 16, 1, 4, 2, 4, 2, 4, 2, 2, 2, 2,
   1, 8, 2, 8, 1, 4, 1, 4, 2, 8,
   1, 8, 2, 8, 1, 4, 1, 4, 2, 8,
   1, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 1, 4, 2, 4, 1, 4,
   2, 8, 1, 8, 1, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2,


   0, 0
};



/* music player state */
#define NUM_PARTS    4

static int *part_ptr[NUM_PARTS] =
{
   part_1, part_2, part_3, part_4
};

static int *part_pos[NUM_PARTS];
static int part_time[NUM_PARTS];

static int freq_table[256];

static ALLEGRO_SAMPLE_INSTANCE *part_voice[NUM_PARTS];

static ALLEGRO_TIMER *music_timer;

#define PAN(x)	  (((x) - 128)/128.0)



/* the main music player function */
static void music_player()
{
   int i, note;

   for (i=0; i<NUM_PARTS; i++) {
      if (part_time[i] <= 0) {
	 note = part_pos[i][0];
	 part_time[i] = part_pos[i][1];

	 al_stop_sample_instance(part_voice[i]);

	 if (i == 3) {
	    if (note == 1) {
	       al_set_sample(part_voice[i], bd);
	       al_set_sample_instance_pan(part_voice[i], PAN(128));
	    }
	    else if (note == 2) {
	       al_set_sample(part_voice[i], snare);
	       al_set_sample_instance_pan(part_voice[i], PAN(160));
	    }
	    else {
	       al_set_sample(part_voice[i], hihat);
	       al_set_sample_instance_pan(part_voice[i], PAN(96));
	    }

	    al_play_sample_instance(part_voice[i]);
	 }
	 else {
	    if (note > 0) {
	       al_set_sample_instance_speed(part_voice[i], freq_table[note]/22050.0);
	       al_play_sample_instance(part_voice[i]);
	    }
	 }

	 part_pos[i] += 2;
	 if (!part_pos[i][1])
	    part_pos[i] = part_ptr[i];
      }

      part_time[i]--;
   }
}



/* this code is sick */
static void init_music()
{
   float vol, val;
   char *p;
   int i;

   if (!al_is_audio_installed())
      return;

   /* sine waves (one straight and one with oscillator sync) for the bass */
   sine = create_sample_u8(22050, 64);
   p = (char *)al_get_sample_data(sine);

   for (i=0; i<64; i++) {
      *p = 128 + (sin((float)i * M_PI / 32.0) + sin((float)i * M_PI / 12.0)) * 8.0;
      p++;
   }

   /* square wave for melody #1 */
   square = create_sample_u8(22050, 64);
   p = (char *)al_get_sample_data(square);

   for (i=0; i<64; i++) {
      *p = (i < 32) ? 120 : 136;
      p++;
   }

   /* saw wave for melody #2 */
   saw = create_sample_u8(22050, 64);
   p = (char *)al_get_sample_data(saw);

   for (i=0; i<64; i++) {
      *p = 120 + (i*4 & 255) / 16;
      p++;
   }

   /* bass drum */
   bd = create_sample_u8(22050, 1024);
   p = (char *)al_get_sample_data(bd);

   for (i=0; i<1024; i++) {
      vol = (float)(1024-i) / 16.0;
      *p = 128 + (sin((float)i / 48.0) + sin((float)i / 32.0)) * vol;
      p++;
   }

   /* snare drum */
   snare = create_sample_u8(22050, 3072);
   p = (char *)al_get_sample_data(snare);

   val = 0;

   for (i=0; i<3072; i++) {
      vol = (float)(3072-i) / 24.0;
      val = (val * 0.9) + (RAND * 0.1);
      *p = 128 + val * vol;
      p++;
   }

   /* hihat */
   hihat = create_sample_u8(22050, 1024);
   p = (char *)al_get_sample_data(hihat);

   for (i=0; i<1024; i++) {
      vol = (float)(1024-i) / 192.0;
      *p = 128 + (sin((float)i / 4.2) + RAND) * vol;
      p++;
   }

   /* start up the player */
   for (i=0; i<256; i++)
      freq_table[i] = (int)(350.0 * pow(2.0, (float)i/12.0));

   for (i=0; i<NUM_PARTS; i++) {
      part_pos[i] = part_ptr[i];
      part_time[i] = 0;
   }

   part_voice[0] = al_create_sample_instance(sine);
   part_voice[1] = al_create_sample_instance(square);
   part_voice[2] = al_create_sample_instance(saw);
   part_voice[3] = al_create_sample_instance(bd);

   al_attach_sample_instance_to_mixer(part_voice[0], al_get_default_mixer());
   al_attach_sample_instance_to_mixer(part_voice[1], al_get_default_mixer());
   al_attach_sample_instance_to_mixer(part_voice[2], al_get_default_mixer());
   al_attach_sample_instance_to_mixer(part_voice[3], al_get_default_mixer());

   al_set_sample_instance_playmode(part_voice[0], ALLEGRO_PLAYMODE_LOOP);
   al_set_sample_instance_playmode(part_voice[1], ALLEGRO_PLAYMODE_LOOP);
   al_set_sample_instance_playmode(part_voice[2], ALLEGRO_PLAYMODE_LOOP);
   al_set_sample_instance_playmode(part_voice[3], ALLEGRO_PLAYMODE_ONCE);

   al_set_sample_instance_gain(part_voice[0], 192/255.0);
   al_set_sample_instance_gain(part_voice[1], 192/255.0);
   al_set_sample_instance_gain(part_voice[2], 192/255.0);
   al_set_sample_instance_gain(part_voice[3], 255/255.0);

   al_set_sample_instance_pan(part_voice[0], PAN(128));
   al_set_sample_instance_pan(part_voice[1], PAN(224));
   al_set_sample_instance_pan(part_voice[2], PAN(32));
   al_set_sample_instance_pan(part_voice[3], PAN(128));

   music_timer = al_create_timer(ALLEGRO_BPS_TO_SECS(22));
}



/* timer callback for playing ping samples */
static int ping_vol;
static int ping_freq;
static int ping_count;
static ALLEGRO_TIMER *ping_timer;


static int ping_proc(void)
{
   ping_freq = ping_freq*4/3;

   play_sample(ping, ping_vol, 128, ping_freq, FALSE);

   if (!--ping_count) 
      return FALSE;

   return TRUE;
}



/* thread to keep the music playing and the pings pinging */
static ALLEGRO_THREAD *sound_update_thread;



static void *sound_update_proc(ALLEGRO_THREAD *thread, void *arg)
{
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   (void)arg;

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_timer_event_source(ping_timer));
   if (music_timer)
      al_register_event_source(queue, al_get_timer_event_source(music_timer));

   while (!al_get_thread_should_stop(thread)) {
      if (!al_wait_for_event_timed(queue, &event, 0.25))
	 continue;

      if (event.any.source == (void *)music_timer)
	 music_player();

      if (event.any.source == (void *)ping_timer) {
	 if (!ping_proc())
	    al_stop_timer(ping_timer);
      }
   }

   al_destroy_event_queue(queue);

   return NULL;
}



/* initialises the sound system */
void init_sound()
{
   float f, osc1, osc2, freq1, freq2, vol, val;
   char *p;
   int len;
   int i;

   if (!al_is_audio_installed())
      return;

   /* zap (firing sound) consists of multiple falling saw waves */
   len = 8192;
   zap = create_sample_u8(22050, len);

   p = (char *)al_get_sample_data(zap);

   osc1 = 0;
   freq1 = 0.02;

   osc2 = 0;
   freq2 = 0.025;

   for (i=0; i<len; i++) {
      vol = (float)(len - i) / (float)len * 127;

      *p = 128 + (fmod(osc1, 1) + fmod(osc2, 1) - 1) * vol;

      osc1 += freq1;
      freq1 -= 0.000001;

      osc2 += freq2;
      freq2 -= 0.00000125;

      p++;
   }

   /* bang (explosion) consists of filtered noise */
   len = 8192;
   bang = create_sample_u8(22050, len);

   p = (char *)al_get_sample_data(bang);

   val = 0;

   for (i=0; i<len; i++) {
      vol = (float)(len - i) / (float)len * 255;
      val = (val * 0.75) + (RAND * 0.25);
      *p = 128 + val * vol;
      p++;
   }

   /* big bang (explosion) consists of noise plus rumble */
   len = 24576;
   bigbang = create_sample_u8(11025, len);

   p = (char *)al_get_sample_data(bigbang);

   val = 0;

   osc1 = 0;
   osc2 = 0;

   for (i=0; i<len; i++) {
      vol = (float)(len - i) / (float)len * 128;

      f = 0.5 + ((float)i / (float)len * 0.4);
      val = (val * f) + (RAND * (1-f));

      *p = 128 + (val + (sin(osc1) + sin(osc2)) / 4) * vol;

      osc1 += 0.03;
      osc2 += 0.04;

      p++;
   }

   /* ping consists of two sine waves */
   len = 8192;
   ping = create_sample_u8(22050, len);

   p = (char *)al_get_sample_data(ping);

   osc1 = 0;
   osc2 = 0;

   for (i=0; i<len; i++) {
      vol = (float)(len - i) / (float)len * 31;

      *p = 128 + (sin(osc1) + sin(osc2) - 1) * vol;

      osc1 += 0.2;
      osc2 += 0.3;

      p++;
   }

   ping_timer = al_create_timer(0.3);

   /* set up my lurvely music player :-) */
   if (!no_music) {
      init_music();
      al_start_timer(music_timer);
   }

   sound_update_thread = al_create_thread(sound_update_proc, NULL);
   al_start_thread(sound_update_thread);
}



/* closes down the sound system */
void shutdown_sound()
{
   if (!al_is_audio_installed())
      return;

   al_destroy_thread(sound_update_thread);
   al_destroy_timer(ping_timer);

   al_stop_samples();

   al_destroy_sample(zap);
   al_destroy_sample(bang);
   al_destroy_sample(bigbang);
   al_destroy_sample(ping);

   if (!no_music) {
      al_destroy_timer(music_timer);

      al_destroy_sample_instance(part_voice[0]);
      al_destroy_sample_instance(part_voice[1]);
      al_destroy_sample_instance(part_voice[2]);
      al_destroy_sample_instance(part_voice[3]);

      al_destroy_sample(sine);
      al_destroy_sample(square);
      al_destroy_sample(saw);
      al_destroy_sample(bd);
      al_destroy_sample(snare);
      al_destroy_sample(hihat);
   }
}



/* plays a shoot sound effect */
void sfx_shoot()
{
   if (al_is_audio_installed())
      play_sample(zap, 64, 128, 1000, FALSE);
}



/* plays an alien explosion sound effect */
void sfx_explode_alien()
{
   if (al_is_audio_installed())
      play_sample(bang, 192, 128, 1000, FALSE);
}



/* plays a block explosion sound effect */
void sfx_explode_block()
{
   if (al_is_audio_installed())
      play_sample(bang, 224, 128, 400, FALSE);
}



/* plays a player explosion sound effect */
void sfx_explode_player()
{
   if (al_is_audio_installed())
      play_sample(bigbang, 255, 128, 1000, FALSE);
}



/* plays a ping sound effect */
void sfx_ping(int times)
{
   if (!al_is_audio_installed())
      return;

   if (times) {
      if (times > 1) {
	 ping_vol = 255;
	 ping_freq = 500;
      }
      else {
	 ping_vol = 128;
	 ping_freq = 1000;
      }

      ping_count = times;

      play_sample(ping, ping_vol, 128, ping_freq, FALSE);

      al_start_timer(ping_timer);
   }
   else
      play_sample(ping, 255, 128, 500, FALSE);
}

