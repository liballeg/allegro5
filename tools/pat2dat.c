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
 *      MIDI patch (for DIGMID) grabbing utility for the Allegro library.
 *
 *      By Shawn Hargreaves.
 *
 *      SoundFont loader based on code provided by George Foot.
 *
 *      See readme.txt for copyright information.
 */


#define ALLEGRO_USE_CONSOLE

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "datedit.h"



static DATAFILE *datafile = NULL;

static int err = 0;

static int opt_compression = 0;
static int opt_overwrite = FALSE;
static int opt_8bit = FALSE;
static int opt_verbose = FALSE;
static int opt_veryverbose = FALSE;
static int opt_bank = 0;
static int opt_drum_bank = 0;
static char *opt_datafile = NULL;
static char *opt_configfile = NULL;
static char *opt_soundfont = NULL;

#define MAX_FILES    256

static char *opt_namelist[MAX_FILES];
static int opt_numnames = 0;

static int need_patches[128], need_drums[128];



static char *patch_names[] = 
{
   "Acoustic Grand",                   "Bright Acoustic",
   "Electric Grand",                   "Honky-Tonk",
   "Electric Piano 1",                 "Electric Piano 2",
   "Harpsichord",                      "Clav",
   "Celesta",                          "Glockenspiel",
   "Music Box",                        "Vibraphone",
   "Marimba",                          "Xylophone",
   "Tubular Bells",                    "Dulcimer",
   "Drawbar Organ",                    "Percussive Organ",
   "Rock Organ",                       "Church Organ",
   "Reed Organ",                       "Accoridan",
   "Harmonica",                        "Tango Accordian",
   "Acoustic Guitar (nylon)",          "Acoustic Guitar (steel)",
   "Electric Guitar (jazz)",           "Electric Guitar (clean)",
   "Electric Guitar (muted)",          "Overdriven Guitar",
   "Distortion Guitar",                "Guitar Harmonics",
   "Acoustic Bass",                    "Electric Bass (finger)",
   "Electric Bass (pick)",             "Fretless Bass",
   "Slap Bass 1",                      "Slap Bass 2",
   "Synth Bass 1",                     "Synth Bass 2",
   "Violin",                           "Viola",
   "Cello",                            "Contrabass",
   "Tremolo Strings",                  "Pizzicato Strings",
   "Orchestral Strings",               "Timpani",
   "String Ensemble 1",                "String Ensemble 2",
   "SynthStrings 1",                   "SynthStrings 2",
   "Choir Aahs",                       "Voice Oohs",
   "Synth Voice",                      "Orchestra Hit",
   "Trumpet",                          "Trombone",
   "Tuba",                             "Muted Trumpet",
   "French Horn",                      "Brass Section",
   "SynthBrass 1",                     "SynthBrass 2",
   "Soprano Sax",                      "Alto Sax",
   "Tenor Sax",                        "Baritone Sax",
   "Oboe",                             "English Horn",
   "Bassoon",                          "Clarinet",
   "Piccolo",                          "Flute",
   "Recorder",                         "Pan Flute",
   "Blown Bottle",                     "Skakuhachi",
   "Whistle",                          "Ocarina",
   "Lead 1 (square)",                  "Lead 2 (sawtooth)",
   "Lead 3 (calliope)",                "Lead 4 (chiff)",
   "Lead 5 (charang)",                 "Lead 6 (voice)",
   "Lead 7 (fifths)",                  "Lead 8 (bass+lead)",
   "Pad 1 (new age)",                  "Pad 2 (warm)",
   "Pad 3 (polysynth)",                "Pad 4 (choir)",
   "Pad 5 (bowed)",                    "Pad 6 (metallic)",
   "Pad 7 (halo)",                     "Pad 8 (sweep)",
   "FX 1 (rain)",                      "FX 2 (soundtrack)",
   "FX 3 (crystal)",                   "FX 4 (atmosphere)",
   "FX 5 (brightness)",                "FX 6 (goblins)",
   "FX 7 (echoes)",                    "FX 8 (sci-fi)",
   "Sitar",                            "Banjo",
   "Shamisen",                         "Koto",
   "Kalimba",                          "Bagpipe",
   "Fiddle",                           "Shanai",
   "Tinkle Bell",                      "Agogo",
   "Steel Drums",                      "Woodblock",
   "Taiko Drum",                       "Melodic Tom",
   "Synth Drum",                       "Reverse Cymbal",
   "Guitar Fret Noise",                "Breath Noise",
   "Seashore",                         "Bird Tweet",
   "Telephone ring",                   "Helicopter",
   "Applause",                         "Gunshot"
};



static char *drum_names[] = 
{
   "Acoustic Bass Drum",               "Bass Drum 1",
   "Side Stick",                       "Acoustic Snare",
   "Hand Clap",                        "Electric Snare",
   "Low Floor Tom",                    "Closed Hi-Hat",
   "High Floor Tom",                   "Pedal Hi-Hat",
   "Low Tom",                          "Open Hi-Hat",
   "Low-Mid Tom",                      "Hi-Mid Tom",
   "Crash Cymbal 1",                   "High Tom",
   "Ride Cymbal 1",                    "Chinese Cymbal",
   "Ride Bell",                        "Tambourine",
   "Splash Cymbal",                    "Cowbell",
   "Crash Cymbal 2",                   "Vibraslap",
   "Ride Cymbal 2",                    "Hi Bongo",
   "Low Bongo",                        "Mute Hi Conga",
   "Open Hi Conga",                    "Low Conga",
   "High Timbale",                     "Low Timbale",
   "High Agogo",                       "Low Agogo",
   "Cabasa",                           "Maracas",
   "Short Whistle",                    "Long Whistle",
   "Short Guiro",                      "Long Guiro",
   "Claves",                           "Hi Wood Block",
   "Low Wood Block",                   "Mute Cuica",
   "Open Cuica",                       "Mute Triangle",
   "Open Triangle"
};



static char *short_patch_names[] = 
{
   "acpiano",     "brpiano",     "synpiano",    "honktonk",
   "epiano1",     "epiano2",     "hrpsi",       "clavi",
   "celeste",     "glock",       "musicbox",    "vibes",
   "marimba",     "xylo",        "tubebell",    "dulcimer",
   "drawbar",     "percorg",     "rockorg",     "church",
   "reedorg",     "tango",       "harmonca",    "concert",
   "nylongtr",    "steelgtr",    "jazzgtr",     "cleangtr",
   "mutegtr",     "drivegtr",    "distgtr",     "harmgtr",
   "acbass",      "fngrbass",    "pickbass",    "fretless",
   "slapbas1",    "slapbas2",    "synbass1",    "synbass2",
   "violin",      "viola",       "cello",       "contra",
   "tremstr",     "pizz",        "harp",        "timp",
   "strens1",     "strens2",     "synstr1",     "synstr2",
   "choirah",     "choiroh",     "synchoir",    "orchhit",
   "trumpet",     "trombone",    "tuba",        "mutetrpt",
   "horn",        "brassens",    "synbras1",    "synbras2",
   "sopsax",      "altosax",     "tenorsax",    "barisax",
   "oboe",        "englhorn",    "bassoon",     "clarinet",
   "piccolo",     "flute",       "recorder",    "panflute",
   "bottle",      "shaku",       "whistle",     "ocarina",
   "square",      "saw",         "calliope",    "chiff",
   "charang",     "voice",       "fifths",      "basslead",
   "newage",      "warm",        "polysyn",     "choir",
   "bowed",       "metal",       "halo",        "sweep",
   "rain",        "sndtrack",    "crystal",     "atmos",
   "bright",      "goblins",     "echoes",      "scifi",
   "sitar",       "banjo",       "shamisen",    "koto",
   "kalimba",     "bagpipes",    "fiddle",      "shannai",
   "tinkle",      "agogo",       "steel",       "woodblck",
   "taiko",       "toms",        "syndrum",     "reverse",
   "fret",        "blow",        "seashore",    "birds",
   "phone",       "copter",      "applause",    "gunshot"
};



static char *short_drum_names[] = 
{
   "kick1",       "kick2",       "rimshot",     "snare1",
   "handclap",    "snare2",      "tomflrlo",    "hihatcl",
   "tomflrhi",    "hihatpd",     "tomlow",      "hihatop",
   "tommidlo",    "tommidhi",    "crash1",      "tomhi",
   "ride1",       "chinese",     "ridebell",    "tambrine",
   "splash",      "cowbell",     "crash2",      "vibrslap",
   "ride2",       "bongohi",     "bongolo",     "congamt",
   "congahi",     "congalo",     "timbalhi",    "timballo",
   "agogohi",     "agogolo",     "cabasa",      "maracas",
   "whistle1",    "whistle2",    "guiro1",      "guiro2",
   "claves",      "woodhi",      "woodlo",      "cuica1",
   "cuica2",      "triangl1",    "triangl2"
};



/* display help on the command syntax */
static void usage(void)
{
   printf("\nPatch grabber for the Allegro DIGMID driver " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR "\n");
   printf("By Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n");
   printf("Usage: pat2dat [options] filename.dat [samplelocation] [tunes.mid]\n\n");
   printf("Supported input formats:\n");
   printf("\tGUS patch: <location> should point to the default.cfg index file\n");
   printf("\tSoundFont: <location> should point to a SoundFont .sf2 file\n\n");
   printf("Options:\n");
   printf("\t'-b x' grab specified instrument bank\n");
   printf("\t'-c' compress objects\n");
   printf("\t'-d x' grab specified percussion kit\n");
   printf("\t'-o' overwrite existing datafile\n");
   printf("\t'-v' select verbose mode\n");
   printf("\t'-vv' select very verbose mode\n");
   printf("\t'-8' reduce sample data to 8 bits\n");
}



/* unused callback for datedit.c */
int datedit_ask(AL_CONST char *fmt, ...) { return 0; }

/* unused callback for datedit.c */
int datedit_select(AL_CONST char *(*list_getter)(int index, int *list_size), AL_CONST char *fmt, ...) { return 0; }


/* callback for outputting messages */
void datedit_msg(AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   printf("%s\n", buf);
}



/* callback for starting a 2-part message output */
void datedit_startmsg(AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   printf("%s", buf);
   fflush(stdout);
}



/* callback for ending a 2-part message output */
void datedit_endmsg(AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   printf("%s\n", buf);
}



/* callback for printing errors */
void datedit_error(AL_CONST char *fmt, ...)
{
   va_list args;
   char buf[1024];

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);

   fprintf(stderr, "%s\n", buf);

   err = 1;
}



/* deal with the strange variable length MIDI number format */
static unsigned long parse_var_len(unsigned char **data)
{
   unsigned long val = **data & 0x7F;

   while (**data & 0x80) {
      (*data)++;
      val <<= 7;
      val += (**data & 0x7F);
   }

   (*data)++;
   return val;
}



/* scans through a MIDI file to find which patches it needs */
static void scan_patches(MIDI *midi, char *patches, char *drums)
{
   unsigned char *p, *end;
   unsigned char running_status, event;
   long l;
   int c;

   for (c=0; c<128; c++)                        /* initialise to unused */
      patches[c] = drums[c] = FALSE;

   for (c=0; c<MIDI_TRACKS; c++) {              /* for each track... */
      p = midi->track[c].data;
      end = p + midi->track[c].len;
      running_status = 0;

      while (p < end) {                         /* work through data stream */
	 event = *p; 
	 if (event & 0x80) {                    /* regular message */
	    p++;
	    if ((event != 0xF0) && (event != 0xF7) && (event != 0xFF))
	       running_status = event;
	 }
	 else                                   /* use running status */
	    event = running_status; 

	 switch (event>>4) {

	    case 0x0C:                          /* program change! */
	       patches[*p] = TRUE;
	       p++;
	       break;

	    case 0x09:                          /* note on, is it a drum? */
	       if ((event & 0x0F) == 9)
		  drums[*p] = TRUE;
	       p += 2;
	       break;

	    case 0x08:                          /* note off */
	    case 0x0A:                          /* note aftertouch */
	    case 0x0B:                          /* control change */
	    case 0x0E:                          /* pitch bend */
	       p += 2;
	       break;

	    case 0x0D:                          /* channel aftertouch */
	       p += 1;
	       break;

	    case 0x0F:                          /* special event */
	       switch (event) {
		  case 0xF0:                    /* sysex */
		  case 0xF7: 
		     l = parse_var_len(&p);
		     p += l;
		     break;

		  case 0xF2:                    /* song position */
		     p += 2;
		     break;

		  case 0xF3:                    /* song select */
		     p++;
		     break;

		  case 0xFF:                    /* meta-event */
		     p++;
		     l = parse_var_len(&p);
		     p += l;
		     break;

		  default:
		     /* the other special events don't have any data bytes,
			so we don't need to bother skipping past them */
		     break;
	       }
	       break;

	    default:
	       /* something has gone badly wrong if we ever get to here */
	       break;
	 }

	 if (p < end)                           /* skip time offset */
	    parse_var_len(&p);
      }
   }
}



/* reads a MIDI file to see what patches it requires */
static int read_midi(AL_CONST char *filename, int attrib, void *param)
{
   char patches[128], drums[128];
   char fname[256];
   MIDI *midi;
   int c;

   strcpy(fname, filename);
   fix_filename_case(fname);

   printf("Scanning %s...\n", fname);

   midi = load_midi(fname);
   if (!midi) {
      fprintf(stderr, "Error reading %s\n", fname);
      err = 1;
      return -1;
   }

   scan_patches(midi, patches, drums);

   if (opt_veryverbose) {
      printf("\n\tUses patches:\n");

      for (c=0; c<128; c++)
	 if (patches[c])
	    printf("\t\t%c Instrument #%-6d(%s)\n", (need_patches[c] ? ' ' : '*'), c+1, patch_names[c]);

      for (c=0; c<128; c++)
	 if (drums[c])
	    printf("\t\t%c Percussion #%-6d(%s)\n", (need_drums[c] ? ' ' : '*'), c+1, (((c >= 35) && (c <= 81)) ? drum_names[c-35] : "unknown"));

      printf("\n");
   }

   for (c=0; c<128; c++) {
      if (patches[c])
	 need_patches[c] = TRUE;

      if (drums[c])
	 need_drums[c] = TRUE;
   }

   destroy_midi(midi);
   return 0;
}



/* scratch buffer for generating new patch files */
static unsigned char *mem = NULL;
static int mem_size = 0;
static int mem_alloced = 0;



/* writes a byte to the memory buffer */
static void mem_write8(int val)
{
   if (mem_size >= mem_alloced) {
      mem_alloced += 4096;
      mem = _AL_REALLOC(mem, mem_alloced);
   }

   mem[mem_size] = val;
   mem_size++;
}



/* writes a word to the memory buffer (little endian) */
static void mem_write16(int val)
{
   mem_write8(val & 0xFF);
   mem_write8((val >> 8) & 0xFF);
}



/* writes a long to the memory buffer (little endian) */
static void mem_write32(int val)
{
   mem_write8(val & 0xFF);
   mem_write8((val >> 8) & 0xFF);
   mem_write8((val >> 16) & 0xFF);
   mem_write8((val >> 24) & 0xFF);
}



/* alters data already written to the memory buffer (little endian) */
static void mem_modify32(int pos, int val)
{
   mem[pos] = val & 0xFF;
   mem[pos+1] = (val >> 8) & 0xFF;
   mem[pos+2] = (val >> 16) & 0xFF;
   mem[pos+3] = (val >> 24) & 0xFF;
}



/* writes a block of data the memory buffer */
static void mem_write_block(void *data, int size)
{
   if (mem_size+size > mem_alloced) {
      mem_alloced = (mem_alloced + size + 4095) & ~4095;
      mem = _AL_REALLOC(mem, mem_alloced);
   }

   memcpy(mem+mem_size, data, size);
   mem_size += size;
}



/* writes from a file to the memory buffer */
static void mem_write_file(PACKFILE *f, int size)
{
   if (mem_size+size > mem_alloced) {
      mem_alloced = (mem_alloced + size + 4095) & ~4095;
      mem = _AL_REALLOC(mem, mem_alloced);
   }

   pack_fread(mem+mem_size, size, f);
   mem_size += size;
}



/* imports data from a GUS patch file */
static DATAFILE *grab_patch(int type, AL_CONST char *filename, DATAFILE_PROPERTY **prop, int depth)
{
   PACKFILE *f;
   int64_t sz = file_size_ex(filename);
   char buf[256];
   int inst, layer, sample, i;
   int data_size, data_size_pos;
   int num_instruments, instrument_size, instrument_size_pos;
   int num_layers, layer_size, layer_size_pos;
   int num_samples, sample_size;
   int loop_start, loop_end;
   int flags;
   int odd_len;

   if (sz <= 0)
      return NULL;

   f = pack_fopen(filename, F_READ);
   if (!f)
      return NULL; 

   if (!opt_8bit) {
      /* raw copy of the disk file */
      mem = _AL_MALLOC(sz);

      pack_fread(mem, sz, f);
      pack_fclose(f);

      if (errno) {
	 _AL_FREE(mem);
	 return NULL;
      }

      if (opt_verbose) {
	 memcpy(buf, mem+22, 60);

	 for (i=0; i<60; i++)
	    if (buf[i] < ' ')
	       break;

	 if (i) {
	    buf[i] = 0;
	    printf("%s\n\n", buf);
	 }
      }

      return datedit_construct(type, mem, sz, prop);
   }

   /* have to parse the file to adjust the sample format */
   mem = NULL;
   mem_size = 0;
   mem_alloced = 0;

   /* check patch header */
   pack_fread(buf, 22, f);
   if (memcmp(buf, "GF1PATCH110\0ID#000002\0", 22)) {
      pack_fclose(f);
      return NULL;
   }

   mem_write_block(buf, 22);

   /* description */
   pack_fread(buf, 60, f);
   mem_write_block(buf, 60);

   if (opt_verbose) {
      for (i=0; i<60; i++)
	 if (buf[i] < ' ')
	    break;

      if (i) {
	 buf[i] = 0;
	 printf("%s\n", buf);
      }
   }

   /* how many instruments? */
   num_instruments = pack_getc(f);
   mem_write8(num_instruments);

   mem_write_file(f, 6);

   data_size = pack_igetl(f);
   data_size_pos = mem_size;
   mem_write32(data_size);

   mem_write_file(f, 36);

   for (inst=0; inst<num_instruments; inst++) {
      mem_write_file(f, 18);

      instrument_size = pack_igetl(f);
      instrument_size_pos = mem_size;
      mem_write32(instrument_size);

      num_layers = pack_getc(f);
      mem_write8(num_layers);

      mem_write_file(f, 40);

      for (layer=0; layer<num_layers; layer++) {
	 mem_write_file(f, 2);

	 layer_size = pack_igetl(f);
	 layer_size_pos = mem_size;
	 mem_write32(layer_size);

	 num_samples = pack_getc(f);
	 mem_write8(num_samples);

	 mem_write_file(f, 40);

	 for (sample=0; sample<num_samples; sample++) {
	    mem_write_file(f, 8);

	    sample_size = pack_igetl(f); 
	    loop_start = pack_igetl(f); 
	    loop_end = pack_igetl(f); 

	    pack_fread(buf, 35, f);

	    flags = pack_getc(f); 

	    if (flags & 1) {
	       /* adjust 16 bit sample parameters */
	       data_size -= sample_size/2;
	       instrument_size -= sample_size/2;
	       layer_size -= sample_size/2;

	       if (sample_size & 1) {
		  data_size--;
		  instrument_size--;
		  layer_size--;
		  odd_len = TRUE;
	       }
	       else
		  odd_len = FALSE;

	       mem_modify32(data_size_pos, data_size);
	       mem_modify32(instrument_size_pos, instrument_size);
	       mem_modify32(layer_size_pos, layer_size);

	       sample_size /= 2;
	       loop_start /= 2;
	       loop_end /= 2;
	    }
	    else
	       odd_len = FALSE;

	    mem_write32(sample_size);
	    mem_write32(loop_start);
	    mem_write32(loop_end);
	    mem_write_block(buf, 35);

	    /* change flags to 8 bit */
	    mem_write8((flags & ~1) | 2);

	    mem_write_file(f, 40);

	    if (flags & 1) {
	       /* reduce 16 bit sample data */
	       if (flags & 2) {
		  for (i=0; i<sample_size; i++)
		     mem_write8(pack_igetw(f) >> 8);
	       }
	       else {
		  for (i=0; i<sample_size; i++)
		     mem_write8((pack_igetw(f) >> 8) ^ 0x80);
	       }
	       if (odd_len)
		  pack_getc(f);
	    }
	    else {
	       /* copy 8 bit waveform */
	       if (flags & 2) {
		  for (i=0; i<sample_size; i++)
		     mem_write8(pack_getc(f));
	       }
	       else {
		  for (i=0; i<sample_size; i++)
		     mem_write8(pack_getc(f) ^ 0x80);
	       }
	    }
	 }
      }
   }

   pack_fclose(f);

   if (errno) {
      _AL_FREE(mem);
      return NULL;
   }

   return datedit_construct(type, mem, mem_size, prop);
}



/* splits a string into component parts */
static int parse_string(char *buf, char *argv[])
{
   int c = 0;

   while ((*buf) && (c<16)) {
      while ((*buf == ' ') || (*buf == '\t') || (*buf == '='))
	 buf++;

      if (*buf == '#')
	 return c;

      if (*buf)
	 argv[c++] = buf;

      while ((*buf) && (*buf != ' ') && (*buf != '\t') && (*buf != '='))
	 buf++;

      if (*buf) {
	 *buf = 0;
	 buf++;
      }
   }

   return c;
}



/* inserts all the required patches into the datafile */
static void add_gus_patches(void)
{
   char dir[256], file[256], buf[256], filename[256];
   DATEDIT_GRAB_PARAMETERS params;
   PACKFILE *f;
   DATAFILE *d;
   char *argv[16];
   int argc;
   int patchnum;
   int flag_num;
   int drum_mode = FALSE;
   int override_mode = FALSE;
   int drum_start = 0;
   int i, j;

   if (opt_configfile) {
      if (!file_exists(opt_configfile, FA_RDONLY | FA_ARCH, NULL)) {
	 fprintf(stderr, "Error: %s not found\n", opt_configfile);
	 err = 1;
	 return;
      }
      strcpy(dir, opt_configfile);
      *get_filename(dir) = 0;
      strcpy(file, get_filename(opt_configfile));
   }
   else {
      if (!_digmid_find_patches(dir, sizeof dir, file, sizeof file)) {
	 fprintf(stderr, "\nError: MIDI patch set not found!\n\n");
	 fprintf(stderr, "You should have a directory containing a collection of GUS format .pat\n");
	 fprintf(stderr, "files and a default.cfg index file, or a SoundFont 2 .sf2 sample bank.\n");
	 fprintf(stderr, "Specify your default.cfg or .sf2 file on the pat2dat command line.\n");
	 err = 1;
	 return;
      }
   }

   strcpy(buf, dir);
   strcat(buf, file);
   fix_filename_case(buf);
   for (i=0; buf[i]; i++)
      if (buf[i] == '\\')
	 buf[i] = '/';

   params.datafile = NULL;  /* only with absolute filenames */
   params.filename = buf;
   params.name = "default_cfg";
   params.type = DAT_DATA;
   /* params.x = */
   /* params.y = */
   /* params.w = */
   /* params.h = */
   /* params.colordepth = */
   params.relative = FALSE;  /* required (see above) */

   printf("Grabbing %s\n", buf);
   d = datedit_grabnew(datafile, &params);
   if (!d) {
      errno = err = 1;
      return;
   }
   else
      datafile = d;

   if (opt_verbose)
      printf("\n");

   f = pack_fopen(buf, F_READ);
   if (!f) {
      fprintf(stderr, "Error reading %s\n", buf);
      err = 1;
      return;
   }

   while (pack_fgets(buf, 255, f) != 0) {
      argc = parse_string(buf, argv);

      if (argc > 0) {
	 /* is first word all digits? */
	 flag_num = TRUE;
	 for (i=0; i<(int)strlen(argv[0]); i++) {
	    if ((!uisdigit(argv[0][i])) && (argv[0][i] != '-')) {
	       flag_num = FALSE;
	       break;
	    }
	 }

	 if ((flag_num) && (argc >= 2)) {
	    if (stricmp(argv[1], "begin_multipatch") == 0) {
	       /* start the block of percussion instruments */
	       drum_start = atoi(argv[0])-1;
	       drum_mode = TRUE;
	    }
	    else if (stricmp(argv[1], "override_patch") == 0) {
	       /* ignore patch overrides */
	       override_mode = TRUE;
	    }
	    else if (!override_mode) {
	       /* must be a patch number */
	       patchnum = atoi(argv[0]);

	       if (!drum_mode)
		  patchnum--;

	       if ((patchnum >= 0) && (patchnum < 128) &&
		   (((drum_mode) && (need_drums[patchnum])) ||
		    ((!drum_mode) && (need_patches[patchnum])))) {

		  if (drum_mode)
		     patchnum += drum_start;

		  /* grab the sample */
		  strcpy(filename, dir);
		  strcat(filename, argv[1]);
		  if ((*get_extension(filename) == 0) && (!strchr(filename, '#')))
		     strcat(filename, ".pat");

		  fix_filename_case(argv[1]);
		  fix_filename_case(filename);
		  for (j=0; filename[j]; j++)
		     if (filename[j] == '\\')
			filename[j] = '/';

		  for (j=0; datafile[j].type != DAT_END; j++)
		     if (stricmp(get_datafile_property(datafile+j, DAT_NAME), argv[1]) == 0)
			break;

		  if (datafile[j].type == DAT_END) {
		     params.filename = filename;
		     params.name = argv[1];
		     params.type = DAT_PATCH;
		     printf("Grabbing %s\n", filename);
		     d = datedit_grabnew(datafile, &params);
		  }
		  else
		     d = datafile;

		  if (!d) {
		     errno = err = 1;
		     pack_fclose(f);
		     return;
		  }
		  else {
		     datafile = d;

		     if (drum_mode)
			need_drums[patchnum-drum_start] = FALSE;
		     else
			need_patches[patchnum] = FALSE;
		  }
	       }
	    }
	 }
	 else {
	    /* handle other keywords */
	    if (stricmp(argv[0], "end_multipatch") == 0) {
	       drum_mode = FALSE;
	       override_mode = FALSE;
	    }
	 }
      }
   }

   pack_fclose(f);
}



/* SoundFont chunk format and ID values */
typedef struct RIFF_CHUNK
{
   int size;
   int id;
   int type;
   int end;
} RIFF_CHUNK;


#define CID(a,b,c,d)    (((d)<<24)+((c)<<16)+((b)<<8)+((a)))


#define CID_RIFF  CID('R','I','F','F')
#define CID_LIST  CID('L','I','S','T')
#define CID_INFO  CID('I','N','F','O')
#define CID_sdta  CID('s','d','t','a')
#define CID_snam  CID('s','n','a','m')
#define CID_smpl  CID('s','m','p','l')
#define CID_pdta  CID('p','d','t','a')
#define CID_phdr  CID('p','h','d','r')
#define CID_pbag  CID('p','b','a','g')
#define CID_pmod  CID('p','m','o','d')
#define CID_pgen  CID('p','g','e','n')
#define CID_inst  CID('i','n','s','t')
#define CID_ibag  CID('i','b','a','g')
#define CID_imod  CID('i','m','o','d')
#define CID_igen  CID('i','g','e','n')
#define CID_shdr  CID('s','h','d','r')
#define CID_ifil  CID('i','f','i','l')
#define CID_isng  CID('i','s','n','g')
#define CID_irom  CID('i','r','o','m')
#define CID_iver  CID('i','v','e','r')
#define CID_INAM  CID('I','N','A','M')
#define CID_IPRD  CID('I','P','R','D')
#define CID_ICOP  CID('I','C','O','P')
#define CID_sfbk  CID('s','f','b','k')
#define CID_ICRD  CID('I','C','R','D')
#define CID_IENG  CID('I','E','N','G')
#define CID_ICMT  CID('I','C','M','T')
#define CID_ISFT  CID('I','S','F','T')



/* SoundFont generator types */
#define SFGEN_startAddrsOffset         0
#define SFGEN_endAddrsOffset           1
#define SFGEN_startloopAddrsOffset     2
#define SFGEN_endloopAddrsOffset       3
#define SFGEN_startAddrsCoarseOffset   4
#define SFGEN_modLfoToPitch            5
#define SFGEN_vibLfoToPitch            6
#define SFGEN_modEnvToPitch            7
#define SFGEN_initialFilterFc          8
#define SFGEN_initialFilterQ           9
#define SFGEN_modLfoToFilterFc         10
#define SFGEN_modEnvToFilterFc         11
#define SFGEN_endAddrsCoarseOffset     12
#define SFGEN_modLfoToVolume           13
#define SFGEN_unused1                  14
#define SFGEN_chorusEffectsSend        15
#define SFGEN_reverbEffectsSend        16
#define SFGEN_pan                      17
#define SFGEN_unused2                  18
#define SFGEN_unused3                  19
#define SFGEN_unused4                  20
#define SFGEN_delayModLFO              21
#define SFGEN_freqModLFO               22
#define SFGEN_delayVibLFO              23
#define SFGEN_freqVibLFO               24
#define SFGEN_delayModEnv              25
#define SFGEN_attackModEnv             26
#define SFGEN_holdModEnv               27
#define SFGEN_decayModEnv              28
#define SFGEN_sustainModEnv            29
#define SFGEN_releaseModEnv            30
#define SFGEN_keynumToModEnvHold       31
#define SFGEN_keynumToModEnvDecay      32
#define SFGEN_delayVolEnv              33
#define SFGEN_attackVolEnv             34
#define SFGEN_holdVolEnv               35
#define SFGEN_decayVolEnv              36
#define SFGEN_sustainVolEnv            37
#define SFGEN_releaseVolEnv            38
#define SFGEN_keynumToVolEnvHold       39
#define SFGEN_keynumToVolEnvDecay      40
#define SFGEN_instrument               41
#define SFGEN_reserved1                42
#define SFGEN_keyRange                 43
#define SFGEN_velRange                 44
#define SFGEN_startloopAddrsCoarse     45
#define SFGEN_keynum                   46
#define SFGEN_velocity                 47
#define SFGEN_initialAttenuation       48
#define SFGEN_reserved2                49
#define SFGEN_endloopAddrsCoarse       50
#define SFGEN_coarseTune               51
#define SFGEN_fineTune                 52
#define SFGEN_sampleID                 53
#define SFGEN_sampleModes              54
#define SFGEN_reserved3                55
#define SFGEN_scaleTuning              56
#define SFGEN_exclusiveClass           57
#define SFGEN_overridingRootKey        58
#define SFGEN_unused5                  59
#define SFGEN_endOper                  60



/* SoundFont sample data */
static short *sf_sample_data = NULL;
static int sf_sample_data_size = 0;



/* SoundFont preset headers */
typedef struct sfPresetHeader
{
   char achPresetName[20];
   unsigned short wPreset;
   unsigned short wBank;
   unsigned short wPresetBagNdx;
   unsigned long dwLibrary;
   unsigned long dwGenre;
   unsigned long dwMorphology;
} sfPresetHeader;


static sfPresetHeader *sf_presets = NULL;
static int sf_num_presets = 0;



/* SoundFont preset indexes */
typedef struct sfPresetBag
{
   unsigned short wGenNdx;
   unsigned short wModNdx;
} sfPresetBag;


static sfPresetBag *sf_preset_indexes = NULL;
static int sf_num_preset_indexes = 0;



/* SoundFont preset generators */
typedef struct rangesType
{
   unsigned char byLo;
   unsigned char byHi;
} rangesType;


typedef union genAmountType
{
   rangesType ranges;
   short shAmount;
   unsigned short wAmount;
} genAmountType;


typedef struct sfGenList
{
   unsigned short sfGenOper;
   genAmountType genAmount;
} sfGenList;


static sfGenList *sf_preset_generators = NULL;
static int sf_num_preset_generators = 0;



/* SoundFont instrument headers */
typedef struct sfInst
{
   char achInstName[20];
   unsigned short wInstBagNdx;
} sfInst;


static sfInst *sf_instruments = NULL;
static int sf_num_instruments = 0;



/* SoundFont instrument indexes */
typedef struct sfInstBag
{
   unsigned short wInstGenNdx;
   unsigned short wInstModNdx;
} sfInstBag;


static sfInstBag *sf_instrument_indexes = NULL;
static int sf_num_instrument_indexes = 0;



/* SoundFont instrument generators */
static sfGenList *sf_instrument_generators = NULL;
static int sf_num_instrument_generators = 0;



/* SoundFont sample headers */
typedef struct sfSample
{
   char achSampleName[20];
   unsigned long dwStart;
   unsigned long dwEnd;
   unsigned long dwStartloop;
   unsigned long dwEndloop;
   unsigned long dwSampleRate;
   unsigned char byOriginalKey;
   signed char chCorrection;
   unsigned short wSampleLink;
   unsigned short sfSampleType;
} sfSample;


static sfSample *sf_samples = NULL;
static int sf_num_samples = 0;



/* list of the layers waiting to be dealt with */
typedef struct EMPTY_WHITE_ROOM
{
   sfSample *sample;
   sfGenList *igen;
   sfGenList *pgen;
   int igen_count;
   int pgen_count;
   float volume;
} EMPTY_WHITE_ROOM;

#define MAX_WAITING  64

static EMPTY_WHITE_ROOM waiting_list[MAX_WAITING];

static int waiting_list_count;



/* SoundFont parameters for the current sample */
static int sf_start, sf_end;
static int sf_loop_start, sf_loop_end;
static int sf_key, sf_tune;
static int sf_pan;
static int sf_keyscale;
static int sf_keymin, sf_keymax;
static int sf_sustain_mod_env;
static int sf_mod_env_to_pitch;
static int sf_delay_vol_env;
static int sf_attack_vol_env;
static int sf_hold_vol_env;
static int sf_decay_vol_env;
static int sf_release_vol_env;
static int sf_sustain_level;
static int sf_mode;



/* interprets a SoundFont generator object */
static void apply_generator(sfGenList *g, int preset)
{
   switch (g->sfGenOper) {

      case SFGEN_startAddrsOffset:
	 sf_start += g->genAmount.shAmount;
	 break;

      case SFGEN_endAddrsOffset:
	 sf_end += g->genAmount.shAmount;
	 break;

      case SFGEN_startloopAddrsOffset:
	 sf_loop_start += g->genAmount.shAmount;
	 break;

      case SFGEN_endloopAddrsOffset:
	 sf_loop_end += g->genAmount.shAmount;
	 break;

      case SFGEN_startAddrsCoarseOffset:
	 sf_start += (int)g->genAmount.shAmount * 32768;
	 break;

      case SFGEN_endAddrsCoarseOffset:
	 sf_end += (int)g->genAmount.shAmount * 32768;
	 break;

      case SFGEN_startloopAddrsCoarse:
	 sf_loop_start += (int)g->genAmount.shAmount * 32768;
	 break;

      case SFGEN_endloopAddrsCoarse:
	 sf_loop_end += (int)g->genAmount.shAmount * 32768;
	 break;

      case SFGEN_modEnvToPitch:
	 if (preset)
	    sf_mod_env_to_pitch += g->genAmount.shAmount;
	 else
	    sf_mod_env_to_pitch = g->genAmount.shAmount;
	 break;

      case SFGEN_sustainModEnv:
	 if (preset)
	    sf_sustain_mod_env += g->genAmount.shAmount;
	 else
	    sf_sustain_mod_env = g->genAmount.shAmount;
	 break;

      case SFGEN_delayVolEnv:
	 if (preset)
	    sf_delay_vol_env += g->genAmount.shAmount;
	 else
	    sf_delay_vol_env = g->genAmount.shAmount;
	 break;

      case SFGEN_attackVolEnv:
	 if (preset)
	    sf_attack_vol_env += g->genAmount.shAmount;
	 else
	    sf_attack_vol_env = g->genAmount.shAmount;
	 break;

      case SFGEN_holdVolEnv:
	 if (preset)
	    sf_hold_vol_env += g->genAmount.shAmount;
	 else
	    sf_hold_vol_env = g->genAmount.shAmount;
	 break;

      case SFGEN_decayVolEnv:
	 if (preset)
	    sf_decay_vol_env += g->genAmount.shAmount;
	 else
	    sf_decay_vol_env = g->genAmount.shAmount;
	 break;

      case SFGEN_sustainVolEnv:
	 if (preset)
	    sf_sustain_level += g->genAmount.shAmount;
	 else
	    sf_sustain_level = g->genAmount.shAmount;
	 break;

      case SFGEN_releaseVolEnv:
	 if (preset)
	    sf_release_vol_env += g->genAmount.shAmount;
	 else
	    sf_release_vol_env = g->genAmount.shAmount;
	 break;

      case SFGEN_pan:
	 if (preset)
	    sf_pan += g->genAmount.shAmount;
	 else
	    sf_pan = g->genAmount.shAmount;
	 break;

      case SFGEN_keyRange:
	 sf_keymin = g->genAmount.ranges.byLo;
	 sf_keymax = g->genAmount.ranges.byHi;
	 break;

      case SFGEN_coarseTune:
	 sf_tune += (int)g->genAmount.shAmount * 100;
	 break;

      case SFGEN_fineTune:
	 sf_tune += g->genAmount.shAmount;
	 break;

      case SFGEN_sampleModes:
	 sf_mode = g->genAmount.wAmount;
	 break;

      case SFGEN_scaleTuning:
	 if (preset)
	    sf_keyscale += g->genAmount.shAmount;
	 else
	    sf_keyscale = g->genAmount.shAmount;
	 break;

      case SFGEN_overridingRootKey:
	 sf_key = g->genAmount.shAmount;
	 break;
   }
}



/* converts AWE32 (MIDI) pitches to GUS (frequency) format */
static int key2freq(int note, int cents)
{
   return pow(2.0, (float)(note*100+cents)/1200.0) * 8175.800781;
}



/* converts the strange AWE32 timecent values to milliseconds */
static int timecent2msec(int t)
{
   return pow(2.0, (float)t/1200.0) * 1000.0;
}



/* converts milliseconds to the even stranger floating point GUS format */
static int msec2gus(int t, int r)
{
   static int vexp[4] = { 1, 8, 64, 512 };

   int e, m;

   t = t * 32 / r;

   if (t <= 0)
      return 0x3F;

   for (e=3; e>=0; e--) {
      m = (vexp[e] * 16 + t/2) / t;

      if ((m > 0) && (m < 64))
	 return ((e << 6) | m);
   }

   return 0xC1;
}



/* copies data from the waiting list into a GUS .pat struct */
static void grab_soundfont_sample(char *name)
{
   static char msg[] = "Converted by Allegro pat2dat v" ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR;
   sfSample *sample;
   sfGenList *igen;
   sfGenList *pgen;
   float vol, total_vol;
   int igen_count;
   int pgen_count;
   int length;
   int min_freq, max_freq;
   int root_freq;
   int decay;
   int sustain;
   int release;
   int flags;
   int i, n;

   mem = NULL;
   mem_size = 0;
   mem_alloced = 0;

   mem_write_block("GF1PATCH110\0ID#000002\0", 22);

   mem_write_block(msg, sizeof(msg));     /* copyright message */
   for (i=0; i<60-(int)sizeof(msg); i++)
      mem_write8(0);

   mem_write8(1);                         /* number of instruments */
   mem_write8(14);                        /* number of voices */
   mem_write8(0);                         /* number of channels */
   mem_write16(waiting_list_count);       /* number of waveforms */
   mem_write16(127);                      /* master volume */
   mem_write32(0);                        /* data size (wrong!) */

   for (i=0; i<36; i++)                   /* reserved */
      mem_write8(0);

   mem_write16(0);                        /* instrument number */

   for (i=0; name[i]; i++)                /* instrument name */
      mem_write8(name[i]);

   while (i < 16) {                       /* pad instrument name */
      mem_write8(0);
      i++;
   }

   mem_write32(0);                        /* instrument size (wrong!) */
   mem_write8(1);                         /* number of layers */

   for (i=0; i<40; i++)                   /* reserved */
      mem_write8(0);

   mem_write8(0);                         /* layer duplicate */
   mem_write8(0);                         /* layer number */
   mem_write32(0);                        /* layer size (wrong!) */
   mem_write8(waiting_list_count);        /* number of samples */

   for (i=0; i<40; i++)                   /* reserved */
      mem_write8(0);

   /* bodge alert!!! I don't know how to make the volume parameters come
    * out right. If I ignore them, some things are very wrong (eg. with
    * the Emu 2MB bank the pads have overloud 'tinkle' layers, and the
    * square lead is way too loud). But if I process the attenuation
    * parameter the way I think it should be done, other things go wrong
    * (the church organ and honkytonk piano come out very quiet). So I
    * look at the volume setting and then normalise the results, to get
    * the differential between each layer but still keep a uniform overall
    * volume for the patch. Totally incorrect, but it usually seems to do 
    * the right thing...
    */

   total_vol = 0;

   for (n=0; n<waiting_list_count; n++) {
      int v = 0;
      int keymin = 0;
      int keymax = 127;

      /* look for volume and keyrange generators */
      for (i=0; i<waiting_list[n].igen_count; i++) {
	 if (waiting_list[n].igen[i].sfGenOper == SFGEN_initialAttenuation) {
	    v = waiting_list[n].igen[i].genAmount.shAmount;
	 }
	 else if (waiting_list[n].igen[i].sfGenOper == SFGEN_keyRange) {
	    keymin = waiting_list[n].igen[i].genAmount.ranges.byLo;
	    keymax = waiting_list[n].igen[i].genAmount.ranges.byHi;
	 }
      }

      for (i=0; i<waiting_list[n].pgen_count; i++) {
	 if (waiting_list[n].pgen[i].sfGenOper == SFGEN_initialAttenuation) {
	    v += waiting_list[n].pgen[i].genAmount.shAmount;
	 }
	 else if (waiting_list[n].pgen[i].sfGenOper == SFGEN_keyRange) {
	    keymin = waiting_list[n].pgen[i].genAmount.ranges.byLo;
	    keymax = waiting_list[n].pgen[i].genAmount.ranges.byHi;
	 }
      }

      /* convert centibels to scaling factor (I _think_ this is right :-) */
      vol = 1.0;
      while (v-- > 0)
	 vol /= pow(10, 0.005);

      waiting_list[n].volume = vol;

      if ((keymin <= 60) && (keymax >= 60))
	 total_vol += vol;
   }

   /* normalise the layer volumes so they sum to unity */
   if (total_vol > 0) {
      for (n=0; n<waiting_list_count; n++)
	 waiting_list[n].volume = CLAMP(0.2, waiting_list[n].volume/total_vol, 1.0);
   }

   /* for each sample... */
   for (n=0; n<waiting_list_count; n++) {
      sample = waiting_list[n].sample;
      igen = waiting_list[n].igen;
      pgen = waiting_list[n].pgen;
      igen_count = waiting_list[n].igen_count;
      pgen_count = waiting_list[n].pgen_count;
      vol = waiting_list[n].volume;

      /* set default generator values */
      sf_start = sample->dwStart;
      sf_end = sample->dwEnd;
      sf_loop_start = sample->dwStartloop;
      sf_loop_end = sample->dwEndloop;
      sf_key = sample->byOriginalKey;
      sf_tune = sample->chCorrection;
      sf_sustain_mod_env = 0;
      sf_mod_env_to_pitch = 0;
      sf_delay_vol_env = -12000;
      sf_attack_vol_env = -12000;
      sf_hold_vol_env = -12000;
      sf_decay_vol_env = -12000;
      sf_release_vol_env = -12000;
      sf_sustain_level = 0;
      sf_pan = 0;
      sf_keyscale = 100;
      sf_keymin = 0;
      sf_keymax = 127;
      sf_mode = 0;

      /* process the lists of generator data */
      for (i=0; i<igen_count; i++)
	 apply_generator(&igen[i], FALSE);

      for (i=0; i<pgen_count; i++)
	 apply_generator(&pgen[i], TRUE);

      /* convert SoundFont values into some more useful formats */
      length = sf_end - sf_start;

      sf_loop_start = CLAMP(0, sf_loop_start-sf_start, sf_end);
      sf_loop_end = CLAMP(0, sf_loop_end-sf_start, sf_end);

      sf_pan = CLAMP(0, sf_pan*16/1000+7, 15);
      sf_keyscale = CLAMP(0, sf_keyscale*1024/100, 2048);

      sf_tune += sf_mod_env_to_pitch * CLAMP(0, 1000-sf_sustain_mod_env, 1000) / 1000;

      min_freq = key2freq(sf_keymin, 0);
      max_freq = key2freq(sf_keymax+1, 0) - 1;
      root_freq = key2freq(sf_key, -sf_tune);

      sustain = CLAMP(0, (1000-sf_sustain_level)*255/1000, 255);

      decay = timecent2msec(sf_delay_vol_env) +
	      timecent2msec(sf_attack_vol_env) +
	      timecent2msec(sf_hold_vol_env) +
	      timecent2msec(sf_decay_vol_env);

      release = timecent2msec(sf_release_vol_env);

      /* The output from this code is almost certainly not a 'correct'
       * .pat file. There are a lot of things I don't know about the
       * format, which have been filled in by guesses or values copied
       * from the Gravis files. And I have no idea what I'm supposed to
       * put in all the size fields, which are currently set to zero :-)
       *
       * But, the results are good enough for DIGMID to understand, and
       * the CONVERT program also seems quite happy to accept them, so
       * it is at least mostly correct...
       */

      mem_write8('s');                    /* sample name */
      mem_write8('a');
      mem_write8('m');
      mem_write8('p');
      mem_write8('0'+(n+1)/10);
      mem_write8('0'+(n+1)%10);
      mem_write8(0);

      mem_write8(0);                      /* fractions */

      if (opt_8bit) {
	 mem_write32(length);             /* waveform size */
	 mem_write32(sf_loop_start);      /* loop start */
	 mem_write32(sf_loop_end);        /* loop end */
      }
      else {
	 mem_write32(length*2);           /* waveform size */
	 mem_write32(sf_loop_start*2);    /* loop start */
	 mem_write32(sf_loop_end*2);      /* loop end */
      }

      mem_write16(sample->dwSampleRate);  /* sample freq */

      mem_write32(min_freq);              /* low freq */
      mem_write32(max_freq);              /* high freq */
      mem_write32(root_freq);             /* root frequency */

      mem_write16(512);                   /* finetune */
      mem_write8(sf_pan);                 /* balance */

      mem_write8(0x3F);                   /* envelope rates */
      mem_write8(0x3F);
      mem_write8(msec2gus(decay, 256-sustain));
      mem_write8(0x3F);
      mem_write8(msec2gus(release, 256));
      mem_write8(0x3F);

      mem_write8(255);                    /* envelope offsets */
      mem_write8(255);
      mem_write8(sustain);
      mem_write8(255);
      mem_write8(0);
      mem_write8(0);

      mem_write8(0x3C);                   /* tremolo sweep */
      mem_write8(0x80);                   /* tremolo rate */
      mem_write8(0);                      /* tremolo depth */

      mem_write8(0x3C);                   /* vibrato sweep */
      mem_write8(0x80);                   /* vibrato rate */
      mem_write8(0);                      /* vibrato depth */

      flags = (32 | 64);                  /* enable sustain and envelope */

      if (opt_8bit)
	 flags |= 2;                      /* signed waveform */
      else
	 flags |= 1;                      /* 16-bit waveform */

      if (sf_mode & 1)
	 flags |= 4;                      /* looped sample */

      mem_write8(flags);                  /* write sample mode */

      mem_write16(60);                    /* scale frequency */
      mem_write16(sf_keyscale);           /* scale factor */

      for (i=0; i<36; i++)                /* reserved */
	 mem_write8(0);

      if (opt_8bit) {                     /* sample waveform */
	 for (i=0; i<length; i++)
	    mem_write8((int)((sf_sample_data[sample->dwStart+i] >> 8) * vol) ^ 0x80);
      }
      else {
	 for (i=0; i<length; i++)
	    mem_write16(sf_sample_data[sample->dwStart+i] * vol);
      }
   }

   datafile = datedit_insert(datafile, NULL, name, DAT_PATCH, mem, mem_size);
}



/* converts loaded SoundFont data */
static int grab_soundfont(int num, int drum, char *name)
{
   sfPresetHeader *pheader;
   sfPresetBag *pindex;
   sfGenList *pgen;
   sfInst *iheader;
   sfInstBag *iindex;
   sfGenList *igen;
   sfSample *sample;
   int pindex_count;
   int pgen_count;
   int iindex_count;
   int igen_count;
   int pnum, inum, lnum;
   int wanted_patch, wanted_bank;
   int keymin, keymax;
   int waiting_room_full;
   int i;
   char *s;

   if (drum) {
      wanted_patch = opt_drum_bank;
      wanted_bank = 128;
      keymin = num;
      keymax = num;
   }
   else {
      wanted_patch = num;
      wanted_bank = opt_bank;
      keymin = 0;
      keymax = 127;
   }

   /* search for the desired preset */
   for (pnum=0; pnum<sf_num_presets; pnum++) {
      pheader = &sf_presets[pnum];

      if ((pheader->wPreset == wanted_patch) && (pheader->wBank == wanted_bank)) {
	 /* find what substructures it uses */
	 pindex = &sf_preset_indexes[pheader->wPresetBagNdx];
	 pindex_count = pheader[1].wPresetBagNdx - pheader[0].wPresetBagNdx;

	 if (pindex_count < 1)
	    return FALSE;

	 /* prettify the preset name */
	 s = pheader->achPresetName;

	 i = strlen(s)-1;
	 while ((i >= 0) && (uisspace(s[i]))) {
	    s[i] = 0;
	    i--;
	 }

	 printf("Grabbing %s -> %s\n", s, name);

	 waiting_list_count = 0;
	 waiting_room_full = FALSE;

	 /* for each layer in this preset */
	 for (inum=0; inum<pindex_count; inum++) {
	    pgen = &sf_preset_generators[pindex[inum].wGenNdx];
	    pgen_count = pindex[inum+1].wGenNdx - pindex[inum].wGenNdx;

	    /* find what instrument we should use */
	    if ((pgen_count > 0) && 
		(pgen[pgen_count-1].sfGenOper == SFGEN_instrument)) {

	       if (pgen[0].sfGenOper == SFGEN_keyRange)
		  if ((pgen[0].genAmount.ranges.byHi < keymin) ||
		      (pgen[0].genAmount.ranges.byLo > keymax))
		     continue;

	       iheader = &sf_instruments[pgen[pgen_count-1].genAmount.wAmount];

	       iindex = &sf_instrument_indexes[iheader->wInstBagNdx];
	       iindex_count = iheader[1].wInstBagNdx - iheader[0].wInstBagNdx;

	       /* for each layer in this instrument */
	       for (lnum=0; lnum<iindex_count; lnum++) {
		  igen = &sf_instrument_generators[iindex[lnum].wInstGenNdx];
		  igen_count = iindex[lnum+1].wInstGenNdx - iindex[lnum].wInstGenNdx;

		  /* find what sample we should use */
		  if ((igen_count > 0) && 
		      (igen[igen_count-1].sfGenOper == SFGEN_sampleID)) {

		     if (igen[0].sfGenOper == SFGEN_keyRange)
			if ((igen[0].genAmount.ranges.byHi < keymin) ||
			    (igen[0].genAmount.ranges.byLo > keymax))
			   continue;

		     sample = &sf_samples[igen[igen_count-1].genAmount.wAmount];

		     /* don't bother with left channel or linked samples */
		     if (sample->sfSampleType & 12)
			continue;

		     /* prettify the sample name */
		     s = sample->achSampleName;

		     i = strlen(s)-1;
		     while ((i >= 0) && (uisspace(s[i]))) {
			s[i] = 0;
			i--;
		     }

		     if (sample->sfSampleType & 0x8000) {
			printf("Error: this SoundFont uses the AWE32 ROM data in sample %s\n", s);
			if (opt_veryverbose)
			   printf("\n");
			return FALSE;
		     }

		     /* add this sample to the waiting list */
		     if (waiting_list_count < MAX_WAITING) {
			if (opt_veryverbose)
			   printf("  - sample %s\n", s);

			waiting_list[waiting_list_count].sample = sample;
			waiting_list[waiting_list_count].igen = igen;
			waiting_list[waiting_list_count].pgen = pgen;
			waiting_list[waiting_list_count].igen_count = igen_count;
			waiting_list[waiting_list_count].pgen_count = pgen_count;
			waiting_list[waiting_list_count].volume = 1.0;
			waiting_list_count++;
		     }
		     else 
			waiting_room_full = TRUE;
		  }
	       }
	    }
	 }

	 if (waiting_room_full)
	    printf("Warning: too many layers in this instrument!\n");

	 if (waiting_list_count > 0)
	    grab_soundfont_sample(name);
	 else
	    printf("Strange... no valid layers found!\n");

	 if (opt_veryverbose)
	    printf("\n");

	 return (waiting_list_count > 0);
      }
   }

   return FALSE;
}



/* reads a byte from the input file */
static int get8(FILE *f)
{
   return getc(f);
}



/* reads a word from the input file (little endian) */
static int get16(FILE *f)
{
   int b1, b2;

   b1 = get8(f);
   b2 = get8(f);

   return ((b2 << 8) | b1);
}



/* reads a long from the input file (little endian) */
static int get32(FILE *f)
{
   int b1, b2, b3, b4;

   b1 = get8(f);
   b2 = get8(f);
   b3 = get8(f);
   b4 = get8(f);

   return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}



/* calculates the file offset for the end of a chunk */
static void calc_end(RIFF_CHUNK *chunk, FILE *f)
{
   chunk->end = ftell(f) + chunk->size + (chunk->size & 1);
}



/* reads and displays a SoundFont text/copyright message */
static void print_sf_string(FILE *f, char *title)
{
   char buf[256];
   char ch;
   int i = 0;

   do {
      ch = get8(f);
      buf[i++] = ch;
   } while ((ch) && (i < 256));

   if (i & 1)
      get8(f);

   if (opt_verbose)
      printf("%-12s%s\n", title, buf);
}



/* error handling macro */
#define BAD_SF()                                            \
{                                                           \
   fprintf(stderr, "Error: bad SoundFont structure\n");     \
   err = 1;                                                 \
   goto getout;                                             \
}



/* inserts all the required patches into the datafile */
static void add_soundfont_patches(void)
{
   RIFF_CHUNK file, chunk, subchunk;
   DATEDIT_GRAB_PARAMETERS params;
   DATAFILE *d;
   FILE *f;
   time_t now;
   char buf[256] = "XXXXXX";
   char tm[80];
   int i;

   #ifdef ALLEGRO_HAVE_MKSTEMP

      int tmp_fd;

      tmp_fd = mkstemp(buf);
      close(tmp_fd);

   #else

      tmpnam(buf);

   #endif
   
   printf("Generating index file\n");

   f = fopen(buf, F_WRITE);
   if (!f) {
      fprintf(stderr, "Error writing temporary file\n");
      err = 1;
      return;
   }

   time(&now);
   strcpy(tm, asctime(localtime(&now)));
   for (i=0; tm[i]; i++)
      if ((tm[i] == '\r') || (tm[i] == '\n'))
	 tm[i] = 0;

   fprintf(f, "# Patch index for the Allegro DIGMID driver\n");
   fprintf(f, "# Produced by pat2dat v" ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR "\n");
   fprintf(f, "# Date: %s\n\n", tm);

   fprintf(f, "0 %s\n", short_patch_names[0]);

   for (i=0; i<128; i++)
      fprintf(f, "%d %s\n", i+1, short_patch_names[i]);

   fprintf(f, "\n129-256 begin_multipatch default blank\n");

   for (i=35; i<82; i++)
      fprintf(f, "\t%d %s\n", i, short_drum_names[i-35]);

   fprintf(f, "end_multipatch\n");

   fclose(f);

   params.datafile = NULL;  /* only with absolute filenames */
   params.filename = buf;
   params.name = "default_cfg";
   params.type = DAT_DATA;
   /* params.x = */
   /* params.y = */
   /* params.w = */
   /* params.h = */
   /* params.colordepth = */
   params.relative = FALSE;  /* required (see above) */

   d = datedit_grabnew(datafile, &params);

   delete_file(buf);

   if (!d) {
      errno = err = 1;
      return;
   }
   else
      datafile = d;

   if (opt_verbose)
      printf("\nReading %s\n\n", opt_soundfont);
   else
      printf("Reading %s\n", opt_soundfont);

   f = fopen(opt_soundfont, "rb");
   if (!f) {
      fprintf(stderr, "Error opening file\n");
      err = 1;
      return;
   }

   file.id = get32(f);
   if (file.id != CID_RIFF) {
      fprintf(stderr, "Error: bad SoundFont header\n");
      err = 1;
      goto getout;
   }

   file.size = get32(f);
   calc_end(&file, f);
   file.type = get32(f);
   if (file.type != CID_sfbk) {
      fprintf(stderr, "Error: bad SoundFont header\n");
      err = 1;
      goto getout;
   }

   while (ftell(f) < file.end) {
      chunk.id = get32(f);
      chunk.size = get32(f);
      calc_end(&chunk, f);

      switch (chunk.id) {

	 case CID_LIST: 
	    /* a list of other chunks */
	    chunk.type = get32(f);

	    while (ftell(f) < chunk.end) {
	       subchunk.id = get32(f);
	       subchunk.size = get32(f);
	       calc_end(&subchunk, f);

	       switch (chunk.type) {

		  case CID_INFO: 
		     /* information block */
		     switch (subchunk.id) {

			case CID_ifil:
			   if (get16(f) < 2) {
			      fprintf(stderr, "Error: this is a SoundFont 1.x file, and I only understand version 2 (.sf2)\n");
			      err = 1;
			      goto getout;
			   }
			   get16(f);
			   break;

			case CID_INAM:
			   print_sf_string(f, "Bank name:");
			   break;

			case CID_irom:
			   print_sf_string(f, "ROM name:");
			   break;

			case CID_ICRD: 
			   print_sf_string(f, "Date:");
			   break;

			case CID_IENG: 
			   print_sf_string(f, "Made by:");
			   break;

			case CID_IPRD: 
			   print_sf_string(f, "Target:");
			   break;

			case CID_ICOP: 
			   print_sf_string(f, "Copyright:");
			   break;

			case CID_ISFT: 
			   print_sf_string(f, "Tools:");
			   break;
		     }

		     /* skip unknown chunks and extra data */
		     fseek(f, subchunk.end, SEEK_SET);
		     break;

		  case CID_pdta: 
		     /* preset, instrument and sample header data */
		     switch (subchunk.id) {

			case CID_phdr: 
			   /* preset headers */
			   sf_num_presets = subchunk.size/38;

			   if ((sf_num_presets*38 != subchunk.size) ||
			       (sf_num_presets < 2) || (sf_presets))
			      BAD_SF();

			   sf_presets = malloc(sizeof(sfPresetHeader) * sf_num_presets);

			   for (i=0; i<sf_num_presets; i++) {
			      fread(sf_presets[i].achPresetName, 20, 1, f);
			      sf_presets[i].wPreset = get16(f);
			      sf_presets[i].wBank = get16(f);
			      sf_presets[i].wPresetBagNdx = get16(f);
			      sf_presets[i].dwLibrary = get32(f);
			      sf_presets[i].dwGenre = get32(f);
			      sf_presets[i].dwMorphology = get32(f);
			   }
			   break;

			case CID_pbag: 
			   /* preset index list */
			   sf_num_preset_indexes = subchunk.size/4;

			   if ((sf_num_preset_indexes*4 != subchunk.size) || 
			       (sf_preset_indexes))
			      BAD_SF();

			   sf_preset_indexes = malloc(sizeof(sfPresetBag) * sf_num_preset_indexes);

			   for (i=0; i<sf_num_preset_indexes; i++) {
			      sf_preset_indexes[i].wGenNdx = get16(f);
			      sf_preset_indexes[i].wModNdx = get16(f);
			   }
			   break;

			case CID_pgen:
			   /* preset generator list */
			   sf_num_preset_generators = subchunk.size/4;

			   if ((sf_num_preset_generators*4 != subchunk.size) || 
			       (sf_preset_generators))
			      BAD_SF();

			   sf_preset_generators = malloc(sizeof(sfGenList) * sf_num_preset_generators);

			   for (i=0; i<sf_num_preset_generators; i++) {
			      sf_preset_generators[i].sfGenOper = get16(f);
			      sf_preset_generators[i].genAmount.wAmount = get16(f);
			   }
			   break;

			case CID_inst: 
			   /* instrument names and indices */
			   sf_num_instruments = subchunk.size/22;

			   if ((sf_num_instruments*22 != subchunk.size) ||
			       (sf_num_instruments < 2) || (sf_instruments))
			      BAD_SF();

			   sf_instruments = malloc(sizeof(sfInst) * sf_num_instruments);

			   for (i=0; i<sf_num_instruments; i++) {
			      fread(sf_instruments[i].achInstName, 20, 1, f);
			      sf_instruments[i].wInstBagNdx = get16(f);
			   }
			   break;

			case CID_ibag: 
			   /* instrument index list */
			   sf_num_instrument_indexes = subchunk.size/4;

			   if ((sf_num_instrument_indexes*4 != subchunk.size) ||
			       (sf_instrument_indexes))
			      BAD_SF();

			   sf_instrument_indexes = malloc(sizeof(sfInstBag) * sf_num_instrument_indexes);

			   for (i=0; i<sf_num_instrument_indexes; i++) {
			      sf_instrument_indexes[i].wInstGenNdx = get16(f);
			      sf_instrument_indexes[i].wInstModNdx = get16(f);
			   }
			   break;

			case CID_igen: 
			   /* instrument generator list */
			   sf_num_instrument_generators = subchunk.size/4;

			   if ((sf_num_instrument_generators*4 != subchunk.size) ||
			       (sf_instrument_generators))
			      BAD_SF();

			   sf_instrument_generators = malloc(sizeof(sfGenList) * sf_num_instrument_generators);

			   for (i=0; i<sf_num_instrument_generators; i++) {
			      sf_instrument_generators[i].sfGenOper = get16(f);
			      sf_instrument_generators[i].genAmount.wAmount = get16(f);
			   }
			   break;

			case CID_shdr:
			   /* sample headers */
			   sf_num_samples = subchunk.size/46;

			   if ((sf_num_samples*46 != subchunk.size) ||
			       (sf_num_samples < 2) || (sf_samples))
			      BAD_SF();

			   sf_samples = malloc(sizeof(sfSample) * sf_num_samples);

			   for (i=0; i<sf_num_samples; i++) {
			      fread(sf_samples[i].achSampleName, 20, 1, f);
			      sf_samples[i].dwStart = get32(f);
			      sf_samples[i].dwEnd = get32(f);
			      sf_samples[i].dwStartloop = get32(f);
			      sf_samples[i].dwEndloop = get32(f);
			      sf_samples[i].dwSampleRate = get32(f);
			      sf_samples[i].byOriginalKey = get8(f);
			      sf_samples[i].chCorrection = get8(f);
			      sf_samples[i].wSampleLink = get16(f);
			      sf_samples[i].sfSampleType = get16(f);
			   }
			   break;
		     }

		     /* skip unknown chunks and extra data */
		     fseek(f, subchunk.end, SEEK_SET);
		     break;

		  case CID_sdta: 
		     /* sample data block */
		     switch (subchunk.id) {

			case CID_smpl: 
			   /* sample waveform (all in one) */
			   if (sf_sample_data)
			      BAD_SF();

			   sf_sample_data_size = subchunk.size / 2;
			   sf_sample_data = malloc(sizeof(short) * sf_sample_data_size);

			   for (i=0; i<sf_sample_data_size; i++)
			      sf_sample_data[i] = get16(f);

			   break;
		     }

		     /* skip unknown chunks and extra data */
		     fseek(f, subchunk.end, SEEK_SET);
		     break;

		  default: 
		     /* unrecognised chunk */
		     fseek(f, chunk.end, SEEK_SET);
		     break;
	       }
	    }
	    break;

	 default:
	    /* not a list so we're not interested */
	    fseek(f, chunk.end, SEEK_SET);
	    break;
      }

      if (feof(f))
	 BAD_SF();
   }

   getout:

   if (f)
      fclose(f);

   /* convert SoundFont to .pat format, and add it to the output datafile */
   if (!err) {
      if ((!sf_sample_data) || (!sf_presets) || 
	  (!sf_preset_indexes) || (!sf_preset_generators) || 
	  (!sf_instruments) || (!sf_instrument_indexes) || 
	  (!sf_instrument_generators) || (!sf_samples))
	 BAD_SF();

      if (opt_verbose)
	 printf("\n");

      for (i=0; (i<128) && (!err); i++) {
	 if (need_patches[i]) {
	    if (grab_soundfont(i, FALSE, short_patch_names[i]))
	       need_patches[i] = FALSE;
	 }
      }

      for (i=35; (i<82) && (!err); i++) {
	 if (need_drums[i]) {
	    if (grab_soundfont(i, TRUE, short_drum_names[i-35]))
	       need_drums[i] = FALSE;
	 }
      }
   }

   /* oh, how polite I am... */
   if (sf_sample_data) {
      free(sf_sample_data);
      sf_sample_data = NULL;
   }

   if (sf_presets) {
      free(sf_presets);
      sf_presets = NULL;
   }

   if (sf_preset_indexes) {
      free(sf_preset_indexes);
      sf_preset_indexes = NULL;
   }

   if (sf_preset_generators) {
      free(sf_preset_generators);
      sf_preset_generators = NULL;
   }

   if (sf_instruments) {
      free(sf_instruments);
      sf_instruments = NULL;
   }

   if (sf_instrument_indexes) {
      free(sf_instrument_indexes);
      sf_instrument_indexes = NULL;
   }

   if (sf_instrument_generators) {
      free(sf_instrument_generators);
      sf_instrument_generators = NULL;
   }

   if (sf_samples) {
      free(sf_samples);
      sf_samples = NULL;
   }
}



int main(int argc, char *argv[])
{
   int c;

   if (install_allegro(SYSTEM_NONE, &errno, atexit) != 0)
       return 1;
   datedit_init();

   for (c=0; c<128; c++)
      need_patches[c] = need_drums[c] = FALSE;

   for (c=0; datedit_grabber_info[c]->type != DAT_END; c++) {
      if (datedit_grabber_info[c]->type == DAT_PATCH) {
	 datedit_grabber_info[c]->grab = grab_patch;
	 break;
      }
   }

   if (datedit_grabber_info[c]->type == DAT_END) {
      fprintf(stderr, "Argh, patch object not registered!\n");
      return 1;
   }

   for (c=1; c<argc; c++) {
      if (argv[c][0] == '-') {
	 switch (utolower(argv[c][1])) {

	    case 'b':
	       if (c >= argc-1) {
		  usage();
		  return 1;
	       }
	       opt_bank = atoi(argv[++c]);
	       break;

	    case 'c':
	       opt_compression = 1; 
	       break;

	    case 'd':
	       if (c >= argc-1) {
		  usage();
		  return 1;
	       }
	       opt_drum_bank = atoi(argv[++c]);
	       break;

	    case 'o':
	       opt_overwrite = TRUE; 
	       break;

	    case 'v':
	       opt_verbose = TRUE;
	       if (utolower(argv[c][2]) == 'v')
		  opt_veryverbose = TRUE;
	       break;

	    case '8':
	       opt_8bit = TRUE; 
	       break;

	    default:
	       printf("Unknown option '%s'\n", argv[c]);
	       return 1;
	 }
      }
      else {
	 if (stricmp(get_extension(argv[c]), "dat") == 0) {
	    if (opt_datafile) {
	       usage();
	       return 1;
	    }
	    opt_datafile = argv[c];
	 }
	 else if (stricmp(get_extension(argv[c]), "mid") == 0) {
	    if (opt_numnames < MAX_FILES)
	       opt_namelist[opt_numnames++] = argv[c];
	 }
	 else if (stricmp(get_extension(argv[c]), "cfg") == 0) {
	    if ((opt_configfile) || (opt_soundfont)) {
	       usage();
	       return 1;
	    }
	    opt_configfile = argv[c];
	 }
	 else if (stricmp(get_extension(argv[c]), "sf2") == 0) {
	    if ((opt_configfile) || (opt_soundfont)) {
	       usage();
	       return 1;
	    }
	    opt_soundfont = argv[c];
	 }
	 else {
	    printf("Unknown file type '.%s'\n", get_extension(argv[c]));
	    return 1;
	 }
      }
   }

   if (!opt_datafile) {
      usage();
      return 1;
   }

   if (!opt_overwrite) {
      if (file_exists(opt_datafile, FA_RDONLY | FA_ARCH, NULL)) {
	 printf("%s already exists: use -o to overwrite\n", opt_datafile);
	 return 1;
      }
   }

   if (opt_numnames == 0) {
      printf("No MIDI files specified: including entire patch set\n");

      for (c=0; c<128; c++)
	 need_patches[c] = TRUE;

      for (c=35; c<82; c++)
	 need_drums[c] = TRUE;
   }
   else {
      need_patches[0] = TRUE;    /* always load the piano */

      for (c=0; c<opt_numnames; c++) {
	 if (for_each_file_ex(opt_namelist[c], 0, ~(FA_ARCH | FA_RDONLY), read_midi, NULL) <= 0) {
	    fprintf(stderr, "Error: %s not found\n", opt_namelist[c]);
	    err = 1;
	    break;
	 }
      }
   }

   if (!err) {
      if (opt_verbose) {
	 if (!opt_veryverbose)
	    printf("\n");

	 printf("Required patches:\n\n");

	 for (c=0; c<128; c++)
	    if (need_patches[c])
	       printf("\tInstrument #%-6d(%s)\n", c+1, patch_names[c]);

	 for (c=0; c<128; c++)
	    if (need_drums[c])
	       printf("\tPercussion #%-6d(%s)\n", c+1, (((c >= 35) && (c <= 81)) ? drum_names[c-35] : "unknown"));

	 printf("\n");
      }

      datafile = _AL_MALLOC(sizeof(DATAFILE));
      datafile->dat = NULL;
      datafile->type = DAT_END;
      datafile->size = 0;
      datafile->prop = NULL;

      if (opt_soundfont)
	 add_soundfont_patches();
      else
	 add_gus_patches();

      if (!err) {
	 DATEDIT_SAVE_DATAFILE_OPTIONS options;

	 options.pack = opt_compression;
	 options.strip = 1;
	 options.sort = 1;
	 options.verbose = (opt_veryverbose || (opt_verbose && opt_compression));
	 options.write_msg = TRUE;
	 options.backup = FALSE;

	 if (!datedit_save_datafile(datafile, opt_datafile, NULL, &options, NULL))
	    err = 1;
      }

      unload_datafile(datafile);

      if (!err) {
	 int ok = TRUE;

	 for (c=0; c<128; c++) {
	    if (need_patches[c]) {
	       if (ok) {
		  printf("\n");
		  ok = FALSE;
	       }
	       printf("Warning: can't find instrument #%d (%s)\n", c+1, patch_names[c]);
	    }
	 }

	 for (c=0; c<128; c++) {
	    if (need_drums[c]) {
	       if (ok) {
		  printf("\n");
		  ok = FALSE;
	       }
	       printf("Warning: can't find percussion #%d (%s)\n", c+1, (((c >= 35) && (c <= 81)) ? drum_names[c-35] : "unknown"));
	    }
	 }
      }

      if ((!err) && (opt_verbose)) {
	 printf("\nEither specify this file with the \"patches=\" line in your allegro.cfg, or\n");
	 printf("rename it to patches.dat and put it in the same directory as your program.\n");
      }
   }

   return err;
}

END_OF_MAIN()
