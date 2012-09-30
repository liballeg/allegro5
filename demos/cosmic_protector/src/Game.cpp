#include "cosmic_protector.hpp"
#include <stdio.h>

#include <allegro5/fshook.h>
#include <allegro5/path.h>

bool kb_installed = false;
bool joy_installed = false;

/*
 * Return the path to user resources (save states, configuration)
 */

#ifdef ALLEGRO_MSVC
   #define snprintf _snprintf
#endif

const char* getResource(const char* fmt, ...)
{
   va_list ap;
   static char res[512];
   static ALLEGRO_PATH *dir;
   static ALLEGRO_PATH *path;

   va_start(ap, fmt);
   memset(res, 0, 512);
   snprintf(res, 511, fmt, ap);

   if (!dir) {
      dir = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
#ifdef ALLEGRO_MSVC
      {
         /* Hack to cope automatically with MSVC workspaces. */
         const char *last = al_get_path_component(dir, -1);
         if (0 == strcmp(last, "Debug")
            || 0 == strcmp(last, "RelWithDebInfo")
            || 0 == strcmp(last, "Release")
            || 0 == strcmp(last, "Profile")) {
            al_remove_path_component(dir, -1);
         }
      }
#endif
      al_append_path_component(dir, "data");
   }

   if (path)
      al_destroy_path(path);

   path = al_create_path(res);
   al_rebase_path(dir, path);
   return al_path_cstr(path, '/');
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
      /* Continue anyway. */
   }
   else {
      al_reserve_samples(16);
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
   if (!rm.add(new FontResource(getResource("gfx/large_font.tga"))))
      return false;
   if (!rm.add(new FontResource(getResource("gfx/small_font.tga"))))
      return false;

   for (int i = 0; BMP_NAMES[i]; i++) {
      if (!rm.add(new BitmapResource(getResource(BMP_NAMES[i])))) {
         printf("Failed to load %s\n", getResource(BMP_NAMES[i]));
         return false;
      }
   }

   for (int i = 0; SAMPLE_NAMES[i]; i++) {
      if (!rm.add(new SampleResource(getResource(SAMPLE_NAMES[i])))) {
         /* Continue anyway. */
      }
   }

   for (int i = 0; STREAM_NAMES[i]; i++) {
      if (!rm.add(new StreamResource(getResource(STREAM_NAMES[i])))) {
         /* Continue anyway. */
      }
   }

   return true;
}

bool init(void)
{
   srand(time(NULL));

   if (!al_init()) {
      debug_message("Error initialising Allegro.\n");
      return false;
   }

   al_init_image_addon();
   al_init_font_addon();
   al_init_acodec_addon();

   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);

   if (!loadResources()) {
      debug_message("Error loading resources.\n");
      return false;
   }

   return true;
}

void done(void)
{
   // Free resources
   al_stop_samples();
   ResourceManager& rm = ResourceManager::getInstance();
   for (int i = RES_STREAM_START; i < RES_STREAM_END; i++) {
      ALLEGRO_AUDIO_STREAM *s = (ALLEGRO_AUDIO_STREAM *)rm.getData(i);
      if (s)
         al_set_audio_stream_playing(s, false);
   }

   ResourceManager::getInstance().destroy();
}

// Returns a random number between lo and hi
float randf(float lo, float hi)
{
   float range = hi - lo;
   int n = rand() % 10000;
   float f = range * n / 10000.0f;
   return lo + f;
}

