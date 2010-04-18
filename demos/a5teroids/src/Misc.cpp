#include "a5teroids.hpp"
#include <stdio.h>

#include <allegro5/fshook.h>
#include <allegro5/path.h>

bool kb_installed = false;
bool joy_installed = false;

/*
 * Return the path to user resources (save states, configuration)
 */

#ifdef ALLEGRO_MACOSX
#ifndef MAX_PATH 
#define MAX_PATH PATH_MAX
#endif
#endif

#ifdef ALLEGRO_MSVC
   #define snprintf _snprintf
#endif

const char* getResource(const char* fmt, ...)
{
   va_list ap;
   static char res[512];
   static ALLEGRO_PATH *dir;
   ALLEGRO_PATH *path;

   va_start(ap, fmt);
   memset(res, 0, 512);
   snprintf(res, 511, fmt, ap);

   if (dir)
      al_destroy_path(dir);
   dir = al_get_standard_path(ALLEGRO_PROGRAM_PATH);
   al_append_path_component(dir, "data");
   path = al_create_path(res);
   al_join_paths(dir, path);
   al_destroy_path(path);

   return al_path_cstr(dir, '/');
}


static void my_destroy_font(void *f)
{
   al_destroy_font((ALLEGRO_FONT *)f);
}


bool loadResources(void)
{
   ResourceManager& rm = ResourceManager::getInstance();

   if (!rm.add(new DisplayResource())) {
   	printf("Failed to create display.\n");
   	return false;
   }

   /* For some reason dsound needs a window... */
   if (!al_install_audio()) {
   	printf("Failed to install audio.\n");
   	return false;
   }

   if (!rm.add(new Player(), false)) {
   	printf("Failed to create player.\n");
   	return false;
   }
   if (!rm.add(new Input())) {
   	printf("Failed initializing input.\n");
	return false;
   }

   // Load fonts
   ALLEGRO_FONT *myfont = al_load_font(getResource("gfx/large_font.tga"), 0, 0);
   if (!myfont) {
      debug_message("Failed to load %s\n", getResource("gfx/large_font.tga"));
      return false;
   }
   GenericResource *res = new GenericResource(myfont, my_destroy_font);
   if (!rm.add(res, false))
      return false;
   myfont = al_load_font(getResource("gfx/small_font.tga"), 0, 0);
   if (!myfont) {
      printf("Failed to load %s\n", getResource("gfx/small_font.tga"));
      return false;
   }
   res = new GenericResource(myfont, my_destroy_font);
   if (!rm.add(res, false))
      return false;

   for (int i = 0; BMP_NAMES[i]; i++) {
      if (!rm.add(new BitmapResource(getResource(BMP_NAMES[i])))) {
         printf("Failed to load %s\n", getResource(BMP_NAMES[i]));
         return false;
      }
   }

   for (int i = 0; SAMPLE_NAMES[i]; i++) {
      if (!rm.add(new SampleResource(getResource(SAMPLE_NAMES[i])))) {
         printf("Failed to load %s\n", getResource(SAMPLE_NAMES[i]));
         return false;
      }
   }

   for (int i = 0; STREAM_NAMES[i]; i++) {
      if (!rm.add(new StreamResource(getResource(STREAM_NAMES[i])))) {
         printf("Failed to load %s\n", getResource(STREAM_NAMES[i]));
         return false;
      }
   }

   return true;
}

bool init(void)
{
   srand(time(NULL));

   al_init();
   al_init_image_addon();
   al_init_font_addon();
   al_init_acodec_addon();

   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);

   // Read configuration file
   /*
   Configuration& cfg = Configuration::getInstance();
   if (!cfg.read()) {
      debug_message("Error reading configuration file.\n");
   }
   */
   
   if (!loadResources()) {
      debug_message("Error loading resources.\n");
      return false;
   }

   voice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_2);
   mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
   al_attach_mixer_to_voice(mixer, voice);
   
   ResourceManager& rm = ResourceManager::getInstance();

   for (int i = RES_SAMPLE_START; i < RES_SAMPLE_END; i++) {
      ALLEGRO_SAMPLE_INSTANCE *s = (ALLEGRO_SAMPLE_INSTANCE *)rm.getData(i);
      al_attach_sample_instance_to_mixer(s, mixer);
   }

   for (int i = RES_STREAM_START; i < RES_STREAM_END; i++) {
      ALLEGRO_AUDIO_STREAM *s = (ALLEGRO_AUDIO_STREAM *)rm.getData(i);
      al_attach_audio_stream_to_mixer(s, mixer);
      al_set_audio_stream_playing(s, false);
   }

   return true;
}

void done(void)
{
   /*
   Configuration& cfg = Configuration::getInstance();
   cfg.write();
   cfg.destroy();
   */

   // Free resources
   ResourceManager::getInstance().destroy();
   al_destroy_mixer(mixer);
   al_destroy_voice(voice);
}

// Returns a random number between lo and hi
float randf(float lo, float hi)
{
   float range = hi - lo;
   int n = rand() % 10000;
   float f = range * n / 10000.0f;
   return lo + f;
}

