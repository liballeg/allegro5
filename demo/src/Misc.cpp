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

const char* getUserResource(const char* fmt, ...) throw (Error)
{
   AL_PATH path;
   va_list ap;
   static char name[MAX_PATH];

   // Create the user resource directory
   char userDir[991];
   al_get_path(AL_USER_DATA_PATH, userDir, 991);
   al_path_init(&path, userDir);

#ifdef ALLEGRO_WINDOWS
   al_path_append(&path, "a5teroids");
#else
   al_path_append(&path, ".a5teroids");
#endif

   al_path_to_string(&path, userDir, 991, ALLEGRO_NATIVE_PATH_SEP);
   al_path_free(&path);

   if (!al_fs_exists(userDir)) {
      if(al_fs_mkdir(userDir) == -1) {
         throw Error("The user resource directory could not be created.\n");
      }
   }

   ustrcpy(name, userDir);

   va_start(ap, fmt);
   uvszprintf(name+strlen(name), MAX_PATH-1, fmt, ap);
   return name;
}



/*
* Get the path to the game resources. First checks for a
* MONSTER_DATA environment variable that points to the resources,
* then a system-wide resource directory then the directory
* "data" from the current directory.
*/
static char* resourcePath()
{
   AL_PATH paths;
   static char path[MAX_PATH];
   char sep[2] = { ALLEGRO_NATIVE_PATH_SEP, '\0' };
   char* env = getenv("A5TEROIDS_DATA");

   if (env) {
      al_path_init(&paths, env);
      al_path_to_string(&paths, path, PATH_MAX, ALLEGRO_NATIVE_PATH_SEP);

      if(!al_fs_exists(path)) {
         al_path_free(&paths);

         al_get_path(AL_SYSTEM_DATA_PATH, path, MAX_PATH);
         al_path_init(&paths, path);
         al_path_append(&paths, "a5teroids");
         al_path_to_string(&paths, path, PATH_MAX, ALLEGRO_NATIVE_PATH_SEP);

         if(!al_fs_exists(path)) {
            al_path_free(&paths);
            al_path_init(&paths, "data/");
         }
      }
   }
   else {
      al_get_path(AL_SYSTEM_DATA_PATH, path, MAX_PATH);
      al_path_init(&paths, path);
      al_path_append(&paths, "a5teroids");
      al_path_to_string(&paths, path, PATH_MAX, ALLEGRO_NATIVE_PATH_SEP);

      if(!al_fs_exists(path)) {
         al_path_free(&paths);
         al_path_init(&paths, "data/");
      }
   }

   al_path_to_string(&paths, path, MAX_PATH, ALLEGRO_NATIVE_PATH_SEP);
   al_path_free(&paths);

   if (path[ustrlen(path)-1] != '/') {
      strncat(path, sep, MAX_PATH-1);
   }

   return path;
}



const char* getResource(const char* fmt, ...)
{
   va_list ap;
   static char name[MAX_PATH];

   ustrcpy(name, resourcePath());

   va_start(ap, fmt);
   uvszprintf(name+ustrlen(name), MAX_PATH-1, fmt, ap);
   return name;
}


static void my_destroy_font(void *f)
{
   al_font_destroy_font((ALLEGRO_FONT *)f);
}


bool loadResources(void)
{
   ResourceManager& rm = ResourceManager::getInstance();

   if (!rm.add(new DisplayResource())) {
   	printf("Failed to create display.\n");
   	return false;
   }

   /* For some reason dsound needs a window... */
   al_audio_init(ALLEGRO_AUDIO_DRIVER_AUTODETECT);

   if (!rm.add(new Player(), false)) {
   	printf("Failed to create player.\n");
   	return false;
   }
   if (!rm.add(new Input())) {
   	printf("Failed initializing input.\n");
	return false;
   }

   // Load fonts
   ALLEGRO_FONT *myfont = al_font_load_font(getResource("gfx/large_font.tga"), NULL);
   if (!myfont) {
      debug_message("Failed to load %s\n", getResource("gfx/large_font.tga"));
      return false;
   }
   GenericResource *res = new GenericResource(myfont, my_destroy_font);
   if (!rm.add(res, false)) return false;
   myfont = al_font_load_font(getResource("gfx/small_font.tga"), NULL);
   if (!myfont) {
      printf("Failed to load %s\n", getResource("gfx/small_font.tga"));
      return false;
   }
   res = new GenericResource(myfont, my_destroy_font);
   if (!rm.add(res, false)) return false;

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

   return true;
}

bool init(void)
{
   srand(time(NULL));

   al_init();
   al_iio_init();
   al_font_init();

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

   voice = al_voice_create(44100, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_2);
   mixer = al_mixer_create(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
   al_voice_attach_mixer(voice, mixer);
   
   ResourceManager& rm = ResourceManager::getInstance();

   for (int i = RES_SAMPLE_START; i < RES_SAMPLE_END; i++) {
      ALLEGRO_SAMPLE *s = (ALLEGRO_SAMPLE *)rm.getData(i);
      al_mixer_attach_sample(mixer, s);
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
}

// Returns a random number between lo and hi
float randf(float lo, float hi)
{
   float range = hi - lo;
   int n = rand() % 10000;
   float f = range * n / 10000.0f;
   return lo + f;
}

