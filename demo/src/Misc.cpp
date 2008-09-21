#include "a5teroids.hpp"
#include <stdio.h>



bool kb_installed = false;
bool joy_installed = false;

/*
 * Return the path to user resources (save states, configuration)
 */
#ifdef __linux__
static char* userResourcePath()
{
   static char path[MAX_PATH];

   char* env = getenv("HOME");

   if (env) {
      strcpy(path, env);
      if (path[strlen(path)-1] != '/') {
         strncat(path, "/.a5teroids/", (sizeof(path)/sizeof(*path))-1);
      }
      else {
         strncat(path, ".a5teroids/", (sizeof(path)/sizeof(*path))-1);
      }
   }
   else {
      strcpy(path, "save/");
   }

   return path;
}
#endif
#ifdef ALLEGRO_MACOSX
#ifndef MAX_PATH 
#define MAX_PATH PATH_MAX
#endif
static char* userResourcePath()
{
   static char path[MAX_PATH];

   char* env = getenv("HOME");

   if (env) {
      strcpy(path, env);
      if (path[strlen(path)-1] != '/') {
         strncat(path, "/.a5teroids/", (sizeof(path)/sizeof(*path))-1);
      }
      else {
         strncat(path, ".a5teroids/", (sizeof(path)/sizeof(*path))-1);
      }
   }
   else {
      strcpy(path, "save/");
   }

   return path;
}
#endif
#if defined(ALLEGRO_MSVC) || defined(ALLEGRO_MINGW32) || defined(ALLEGRO_BCC32)
static char* userResourcePath()
{
   static char path[MAX_PATH];

   int success = SHGetSpecialFolderPath(0, path, CSIDL_APPDATA, false);

   if (success) {
      if (path[strlen(path)-1] != '/') {
         strncat(path, "/a5teroids/", (sizeof(path)/sizeof(*path))-1);	
      }
      else {
         strncat(path, "a5teroids/", (sizeof(path)/sizeof(*path))-1);
      }
   }
   else
      strcpy(path, "save/");

   return path;
}
#endif



const char* getUserResource(const char* fmt, ...) throw (Error)
{
   va_list ap;
   static char name[MAX_PATH];

   // Create the user resource directory
   char userDir[991];
   sprintf(userDir, "%s", userResourcePath());
#ifndef __linux__
   userDir[strlen(userDir)-1] = 0;
#endif
   if (!file_exists(userDir, FA_DIREC, 0)) {
      char command[1000];
      sprintf(command, "mkdir \"%s\"", userDir);
      system(command);
      if (!file_exists(userDir, FA_DIREC, 0)) {
         throw Error("The user resource directory could not be created.\n");
      }
   }

   strcpy(name, userResourcePath());

   va_start(ap, fmt);
   vsnprintf(name+strlen(name), (sizeof(name)/sizeof(*name))-1, fmt, ap);
   return name;
}



/*
* Get the path to the game resources. First checks for a
* MONSTER_DATA environment variable that points to the resources,
* then a system-wide resource directory then the directory
* "data" from the current directory.
*/
#ifdef __linux__
static char* resourcePath()
{
   static char path[MAX_PATH];

   char* env = getenv("A5TEROIDS_DATA");

   if (env) {
      strcpy(path, env);
      if (path[strlen(path)-1] != '/') {
         strncat(path, "/", (sizeof(path)/sizeof(*path))-1);
      }
   }
   else {
     strcpy(path, "data/");
   }

   return path;
}
#else
static char* resourcePath()
{
   static char path[MAX_PATH];

   char* env = getenv("A5TEROIDS_DATA");

   if (env) {
      strcpy(path, env);
      if (path[strlen(path)-1] != '\\' && path[strlen(path)-1] != '/') {
         strncat(path, "/", (sizeof(path)/sizeof(*path))-1);
      }
   }
   else {
      strcpy(path, "data/");
   }

   return path;
}
#endif



const char* getResource(const char* fmt, ...)
{
   va_list ap;
   static char name[MAX_PATH];

   strcpy(name, resourcePath());

   va_start(ap, fmt);
   vsnprintf(name+strlen(name), (sizeof(name)/sizeof(*name))-1, fmt, ap);
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
   al_mixer_destroy(mixer);
   al_voice_destroy(voice);
}

// Returns a random number between lo and hi
float randf(float lo, float hi)
{
   float range = hi - lo;
   int n = rand() % 10000;
   float f = range * n / 10000.0f;
   return lo + f;
}

