#include <stdio.h>
#include "cosmic_protector.hpp"
#include "joypad_c.h"

#ifdef A5O_ANDROID
   #include "allegro5/allegro_android.h"
   #define IS_ANDROID   (true)
   #define IS_MSVC      (false)
#elif defined(A5O_MSVC)
   #define IS_ANDROID   (false)
   #define IS_MSVC      (true)
   #define snprintf     _snprintf
#else
   #define IS_ANDROID   (false)
   #define IS_MSVC      (false)
#endif

bool kb_installed = false;
bool joy_installed = false;

/*
 * Return the path to user resources (save states, configuration)
 */

static A5O_PATH* getDir()
{
   A5O_PATH *dir;

   if (IS_ANDROID) {
      /* Path within the APK, not normal filesystem. */
      dir = al_create_path_for_directory("data");
   }
   else {
      dir = al_get_standard_path(A5O_RESOURCES_PATH);
      if (IS_MSVC) {
         /* Hack to cope automatically with MSVC workspaces. */
         const char *last = al_get_path_component(dir, -1);
         if (0 == strcmp(last, "Debug")
            || 0 == strcmp(last, "RelWithDebInfo")
            || 0 == strcmp(last, "Release")
            || 0 == strcmp(last, "Profile")) {
            al_remove_path_component(dir, -1);
         }
      }
      al_append_path_component(dir, "data");
   }
   return dir;
}

const char* getResource(const char* fmt, ...)
{
   va_list ap;
   static char res[512];
   static A5O_PATH *dir;
   static A5O_PATH *path;

   va_start(ap, fmt);
   memset(res, 0, 512);
   snprintf(res, 511, fmt, ap);

   if (!dir)
      dir = getDir();

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
   if (!rm.add(new FontResource(getResource("gfx/large_font.png"))))
      return false;
   if (!rm.add(new FontResource(getResource("gfx/small_font.png"))))
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

   joypad_start();

   return true;
}

bool init(void)
{
   srand(time(NULL));
   if (!al_init()) {
      debug_message("Error initialising Allegro.\n");
      return false;
   }
   al_set_org_name("Allegro");
   al_set_app_name("Cosmic Protector");
   al_init_image_addon();
   al_init_font_addon();
   al_init_acodec_addon();
   al_init_primitives_addon();
#ifdef A5O_ANDROID
   al_android_set_apk_file_interface();
#endif

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
      A5O_AUDIO_STREAM *s = (A5O_AUDIO_STREAM *)rm.getData(i);
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

