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
 *      CoreAudio MIDI driver routines for MacOS X.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
#error something is wrong with the makefile
#endif                


static int ca_detect(int);
static int ca_init(int, int);
static void ca_exit(int);
static int ca_mixer_volume(int);
static void ca_raw_midi(int);


static AUGraph graph;
static AudioUnit synth_unit;
static int command, data_pos, data_buffer[2];
static char driver_desc[256];


NoteAllocator osx_note_allocator = NULL;


MIDI_DRIVER midi_core_audio =
{
   MIDI_CORE_AUDIO,         /* driver ID code */
   empty_string,            /* driver name */
   empty_string,            /* description string */
   "CoreAudio",             /* ASCII format name string */
   16,                      /* available voices */
   0,                       /* voice number offset */
   16,                      /* maximum voices we can support */
   0,                       /* default number of voices to use */
   10, 10,                  /* reserved voice range */
   ca_detect,               /* AL_METHOD(int,  detect, (int input)); */
   ca_init,                 /* AL_METHOD(int,  init, (int input, int voices)); */
   ca_exit,                 /* AL_METHOD(void, exit, (int input)); */
   ca_mixer_volume,         /* AL_METHOD(int,  mixer_volume, (int volume)); */
   ca_raw_midi,             /* AL_METHOD(void, raw_midi, (int data)); */
   _dummy_load_patches,     /* AL_METHOD(int,  load_patches, (AL_CONST char *patches, AL_CONST char *drums)); */
   _dummy_adjust_patches,   /* AL_METHOD(void, adjust_patches, (AL_CONST char *patches, AL_CONST char *drums)); */
   _dummy_key_on,           /* AL_METHOD(void, key_on, (int inst, int note, int bend, int vol, int pan)); */
   _dummy_noop1,            /* AL_METHOD(void, key_off, (int voice)); */
   _dummy_noop2,            /* AL_METHOD(void, set_volume, (int voice, int vol)); */
   _dummy_noop3,            /* AL_METHOD(void, set_pitch, (int voice, int note, int bend)); */
   _dummy_noop2,            /* AL_METHOD(void, set_pan, (int voice, int pan)); */
   _dummy_noop2,            /* AL_METHOD(void, set_vibrato, (int voice, int amount)); */
};



static int ca_detect(int input)
{
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return FALSE;
   }
   if (floor(NSAppKitVersionNumber) <= NSAppKitVersionNumber10_1)
      return FALSE;
   return TRUE;
}



static int ca_init(int input, int voices)
{
   char tmp[128];
   ComponentDescription desc;
   AUNode synth_node, output_node;

   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return -1;
   }
   if (floor(NSAppKitVersionNumber) <= NSAppKitVersionNumber10_1) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("MacOS X.2 or newer required by this driver"));
      return -1;
   }
   
   NewAUGraph(&graph);
   
   desc.componentType = kAudioUnitType_MusicDevice;
   desc.componentSubType = kAudioUnitSubType_DLSSynth;
   desc.componentManufacturer = kAudioUnitManufacturer_Apple;
   desc.componentFlags = 0;
   desc.componentFlagsMask = 0;
   AUGraphNewNode(graph, &desc, 0, NULL, &synth_node);
   
   desc.componentType = kAudioUnitType_Output;
   desc.componentSubType = kAudioUnitSubType_DefaultOutput;
   desc.componentManufacturer = kAudioUnitManufacturer_Apple;
   desc.componentFlags = 0;        
   desc.componentFlagsMask = 0;   
   AUGraphNewNode(graph, &desc, 0, NULL, &output_node);

   AUGraphConnectNodeInput(graph, synth_node, 0, output_node, 0);

   AUGraphOpen(graph);
   AUGraphInitialize(graph);

   AUGraphGetNodeInfo(graph, synth_node, NULL, NULL, NULL, &synth_unit);

   AUGraphStart(graph);
   
   uszprintf(driver_desc, sizeof(driver_desc),  uconvert_ascii("CoreAudio MIDI software synthesizer", tmp));
   midi_core_audio.desc = driver_desc;

   command = -1;

   return 0;
}



static void ca_exit(int input)
{
   if (input)
      return;
   AUGraphStop(graph);
   AUGraphUninitialize(graph);
   AUGraphClose(graph);
   DisposeAUGraph(graph);
}



static int ca_mixer_volume(int volume)
{
   float value = (float)volume / 255.0;

   return AudioUnitSetParameter(synth_unit, kAudioUnitParameterUnit_LinearGain, kAudioUnitScope_Output, 0, value, 0);
}



static void ca_raw_midi(int data)
{
   if (command == -1) {
      data_buffer[0] = data_buffer[1] = 0;
      data_pos = 0;
      command = data;
      return;
   }
   data_buffer[data_pos++] = data;
   if (((data_pos == 1) && (((command >> 4) == 0xC) || ((command >> 4) == 0xD))) || (data_pos == 2)) {
      MusicDeviceMIDIEvent(synth_unit, command, data_buffer[0], data_buffer[1], 0);
      command = -1;
   }
}
