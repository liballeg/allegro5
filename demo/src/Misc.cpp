#include "a5teroids.hpp"

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
#else
static char* userResourcePath()
{
   static char path[MAX_PATH];

   bool success = SHGetSpecialFolderPath(0, path, CSIDL_APPDATA, false);

   if (success) {
      if (path[strlen(path)-1] != '/') {
         strncat(path, "/a5teroids/", (sizeof(path)/sizeof(*path))-1);	
      }
      else {
         strncat(path, "a5teroids/", (sizeof(path)/sizeof(*path))-1);
      }
   }
   else {
      strcpy(path, "save/");
   }

   return path;
}
#endif



const char* getUserResource(const char* fmt, ...) throw (Error)
{
   va_list ap;
   static char name[MAX_PATH];

   // Create the user resource directory
   char userDir[1000];
   sprintf(userDir, "%s", userResourcePath());
#ifndef __linux__
   userDir[strlen(userDir)-1] = 0;
#endif
   if (!file_exists(userDir, FA_DIREC, 0)) {
      char command[1000];
      snprintf(command, (sizeof(command)/sizeof(*command))-1, "mkdir \"%s\"", userDir);
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
   a5font_destroy_font((A5FONT_FONT *)f);
}


bool loadResources(void)
{
   ResourceManager& rm = ResourceManager::getInstance();

   if (!rm.add(new DisplayResource())) return false;
   if (!rm.add(new Player(), false)) return false;
   if (!rm.add(new Input())) return false;

   // Load fonts
   A5FONT_FONT *myfont = a5font_load_font(getResource("gfx/large_font.tga"), NULL);
   if (!myfont) return false;
   GenericResource *res = new GenericResource(myfont, my_destroy_font);
   if (!rm.add(res, false)) return false;
   myfont = a5font_load_font(getResource("gfx/small_font.tga"), NULL);
   if (!myfont) return false;
   res = new GenericResource(myfont, my_destroy_font);
   if (!rm.add(res, false)) return false;

   for (int i = 0; BMP_NAMES[i]; i++) {
      if (!rm.add(new BitmapResource(getResource(BMP_NAMES[i]))))
         return false;
   }

   for (int i = 0; SAMPLE_NAMES[i]; i++) {
      if (!rm.add(new SampleResource(getResource(SAMPLE_NAMES[i]))))
         return false;
   }

   return true;
}

bool init(void)
{
   srand(time(NULL));

   al_init();

   install_sound(DIGI_AUTODETECT, MIDI_NONE, 0);
   
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);

   // Read configuration file
   /*
   Configuration& cfg = Configuration::getInstance();
   if (!cfg.read()) {
      debug_message("Error reading configuration file.\n");
   }
   */

   if (!loadResources()) {
      debug_message("Error loading resources.");
      return false;
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

