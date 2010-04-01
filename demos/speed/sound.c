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
#include <allegro.h>

#include "speed.h"

#ifndef M_PI
#define M_PI         3.14159265358979323846
#endif



/* generated sample waveforms */
static SAMPLE *zap;
static SAMPLE *bang;
static SAMPLE *bigbang;
static SAMPLE *ping;
static SAMPLE *sine;
static SAMPLE *square;
static SAMPLE *saw;
static SAMPLE *bd;
static SAMPLE *snare;
static SAMPLE *hihat;



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

static int part_voice[NUM_PARTS];



/* the main music player function */
static void music_player(void)
{
   int i, note;

   for (i=0; i<NUM_PARTS; i++) {
      if (part_time[i] <= 0) {
	 note = part_pos[i][0];
	 part_time[i] = part_pos[i][1];

	 voice_stop(part_voice[i]);

	 if (i == 3) {
	    if (note == 1) {
	       reallocate_voice(part_voice[i], bd);
	       voice_set_pan(part_voice[i], 128);
	    }
	    else if (note == 2) {
	       reallocate_voice(part_voice[i], snare);
	       voice_set_pan(part_voice[i], 160);
	    }
	    else {
	       reallocate_voice(part_voice[i], hihat);
	       voice_set_pan(part_voice[i], 96);
	    }

	    voice_start(part_voice[i]);
	 }
	 else {
	    if (note > 0) {
	       voice_set_frequency(part_voice[i], freq_table[note]);
	       voice_start(part_voice[i]);
	    }
	 }

	 part_pos[i] += 2;
	 if (!part_pos[i][1])
	    part_pos[i] = part_ptr[i];
      }

      part_time[i]--;
   }
}

END_OF_STATIC_FUNCTION(music_player);



/* this code is sick */
static void init_music()
{
   float vol, val;
   char *p;
   int i;

   /* sine waves (one straight and one with oscillator sync) for the bass */
   sine = create_sample(8, FALSE, 22050, 64);
   p = (char *)sine->data;

   for (i=0; i<64; i++) {
      *p = 128 + (sin((float)i * M_PI / 32.0) + sin((float)i * M_PI / 12.0)) * 8.0;
      p++;
   }

   /* square wave for melody #1 */
   square = create_sample(8, FALSE, 22050, 64);
   p = (char *)square->data;

   for (i=0; i<64; i++) {
      *p = (i < 32) ? 120 : 136;
      p++;
   }

   /* saw wave for melody #2 */
   saw = create_sample(8, FALSE, 22050, 64);
   p = (char *)saw->data;

   for (i=0; i<64; i++) {
      *p = 120 + (i*4 & 255) / 16;
      p++;
   }

   /* bass drum */
   bd = create_sample(8, FALSE, 22050, 1024);
   p = (char *)bd->data;

   for (i=0; i<1024; i++) {
      vol = (float)(1024-i) / 16.0;
      *p = 128 + (sin((float)i / 48.0) + sin((float)i / 32.0)) * vol;
      p++;
   }

   /* snare drum */
   snare = create_sample(8, FALSE, 22050, 3072);
   p = (char *)snare->data;

   val = 0;

   for (i=0; i<3072; i++) {
      vol = (float)(3072-i) / 24.0;
      val = (val * 0.9) + (RAND * 0.1);
      *p = 128 + val * vol;
      p++;
   }

   /* hihat */
   hihat = create_sample(8, FALSE, 22050, 1024);
   p = (char *)hihat->data;

   for (i=0; i<1024; i++) {
      vol = (float)(1024-i) / 192.0;
      *p = 128 + (sin((float)i / 4.2) + RAND) * vol;
      p++;
   }

   /* start up the player */
   for (i=0; i<256; i++)
      freq_table[i] = (int)(350.0 * pow(2.0, (float)i/12.0));

   LOCK_VARIABLE(sine);
   LOCK_VARIABLE(square);
   LOCK_VARIABLE(saw);
   LOCK_VARIABLE(bd);
   LOCK_VARIABLE(snare);
   LOCK_VARIABLE(hihat);
   LOCK_VARIABLE(part_1);
   LOCK_VARIABLE(part_2);
   LOCK_VARIABLE(part_3);
   LOCK_VARIABLE(part_4);
   LOCK_VARIABLE(part_ptr);
   LOCK_VARIABLE(part_pos);
   LOCK_VARIABLE(part_time);
   LOCK_VARIABLE(freq_table);
   LOCK_VARIABLE(part_voice);
   LOCK_FUNCTION(music_player);

   for (i=0; i<NUM_PARTS; i++) {
      part_pos[i] = part_ptr[i];
      part_time[i] = 0;
   }

   part_voice[0] = allocate_voice(sine);
   part_voice[1] = allocate_voice(square);
   part_voice[2] = allocate_voice(saw);
   part_voice[3] = allocate_voice(bd);

   voice_set_priority(part_voice[0], 255);
   voice_set_priority(part_voice[1], 255);
   voice_set_priority(part_voice[2], 255);
   voice_set_priority(part_voice[3], 255);

   voice_set_playmode(part_voice[0], PLAYMODE_LOOP);
   voice_set_playmode(part_voice[1], PLAYMODE_LOOP);
   voice_set_playmode(part_voice[2], PLAYMODE_LOOP);
   voice_set_playmode(part_voice[3], PLAYMODE_PLAY);

   voice_set_volume(part_voice[0], 192);
   voice_set_volume(part_voice[1], 192);
   voice_set_volume(part_voice[2], 192);
   voice_set_volume(part_voice[3], 255);

   voice_set_pan(part_voice[0], 128);
   voice_set_pan(part_voice[1], 224);
   voice_set_pan(part_voice[2], 32);
   voice_set_pan(part_voice[3], 128);

   install_int_ex(music_player, BPS_TO_TIMER(22));
}



/* timer callback for playing ping samples */
static int ping_vol;
static int ping_freq;
static int ping_count;


static void ping_proc(void)
{
   ping_freq = ping_freq*4/3;

   play_sample(ping, ping_vol, 128, ping_freq, FALSE);

   if (!--ping_count) 
      remove_int(ping_proc);
}

END_OF_STATIC_FUNCTION(ping_proc);



/* initialises the sound system */
void init_sound()
{
   float f, osc1, osc2, freq1, freq2, vol, val;
   char *p;
   int i;

   /* zap (firing sound) consists of multiple falling saw waves */
   zap = create_sample(8, FALSE, 22050, 8192);

   p = (char *)zap->data;

   osc1 = 0;
   freq1 = 0.02;

   osc2 = 0;
   freq2 = 0.025;

   for (i=0; i<zap->len; i++) {
      vol = (float)(zap->len - i) / (float)zap->len * 127;

      *p = 128 + (fmod(osc1, 1) + fmod(osc2, 1) - 1) * vol;

      osc1 += freq1;
      freq1 -= 0.000001;

      osc2 += freq2;
      freq2 -= 0.00000125;

      p++;
   }

   /* bang (explosion) consists of filtered noise */
   bang = create_sample(8, FALSE, 22050, 8192);

   p = (char *)bang->data;

   val = 0;

   for (i=0; i<bang->len; i++) {
      vol = (float)(bang->len - i) / (float)bang->len * 255;
      val = (val * 0.75) + (RAND * 0.25);
      *p = 128 + val * vol;
      p++;
   }

   /* big bang (explosion) consists of noise plus rumble */
   bigbang = create_sample(8, FALSE, 11025, 24576);

   p = (char *)bigbang->data;

   val = 0;

   osc1 = 0;
   osc2 = 0;

   for (i=0; i<bigbang->len; i++) {
      vol = (float)(bigbang->len - i) / (float)bigbang->len * 128;

      f = 0.5 + ((float)i / (float)bigbang->len * 0.4);
      val = (val * f) + (RAND * (1-f));

      *p = 128 + (val + (sin(osc1) + sin(osc2)) / 4) * vol;

      osc1 += 0.03;
      osc2 += 0.04;

      p++;
   }

   /* ping consists of two sine waves */
   ping = create_sample(8, FALSE, 22050, 8192);

   p = (char *)ping->data;

   osc1 = 0;
   osc2 = 0;

   for (i=0; i<ping->len; i++) {
      vol = (float)(ping->len - i) / (float)ping->len * 31;

      *p = 128 + (sin(osc1) + sin(osc2) - 1) * vol;

      osc1 += 0.2;
      osc2 += 0.3;

      p++;
   }

   /* set up my lurvely music player :-) */
   if (!no_music)
      init_music();
}



/* closes down the sound system */
void shutdown_sound()
{
   destroy_sample(zap);
   destroy_sample(bang);
   destroy_sample(bigbang);
   destroy_sample(ping);

   if (!no_music) {
      remove_int(music_player);

      deallocate_voice(part_voice[0]);
      deallocate_voice(part_voice[1]);
      deallocate_voice(part_voice[2]);
      deallocate_voice(part_voice[3]);

      destroy_sample(sine);
      destroy_sample(square);
      destroy_sample(saw);
      destroy_sample(bd);
      destroy_sample(snare);
      destroy_sample(hihat);
   }

   remove_int(ping_proc);
}



/* plays a shoot sound effect */
void sfx_shoot()
{
   play_sample(zap, 64, 128, 1000, FALSE);
}



/* plays an alien explosion sound effect */
void sfx_explode_alien()
{
   play_sample(bang, 192, 128, 1000, FALSE);
}



/* plays a block explosion sound effect */
void sfx_explode_block()
{
   play_sample(bang, 224, 128, 400, FALSE);
}



/* plays a player explosion sound effect */
void sfx_explode_player()
{
   play_sample(bigbang, 255, 128, 1000, FALSE);
}



/* plays a ping sound effect */
void sfx_ping(int times)
{
   static int virgin = TRUE;

   if (times) {
      if (virgin) {
	 LOCK_VARIABLE(ping);
	 LOCK_VARIABLE(ping_vol);
	 LOCK_VARIABLE(ping_freq);
	 LOCK_VARIABLE(ping_count);
	 LOCK_FUNCTION(ping_proc);

	 virgin = FALSE;
      }

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

      install_int(ping_proc, 300);
   }
   else
      play_sample(ping, 255, 128, 500, FALSE);
}

