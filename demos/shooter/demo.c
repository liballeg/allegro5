#include "demo.h"
#include "data.h"
#include "expl.h"
#include "animsel.h"
#include "display.h"
#include "title.h"
#include "game.h"

/* command line options */
int cheat = FALSE;
int jumpstart = FALSE;

/* our graphics, samples, etc. */
DATAFILE *data;

int max_fps = FALSE;


/* modifies the palette to give us nice colors for the GUI dialogs */
static void set_gui_colors(void)
{
   static RGB black = { 0, 0, 0, 0 };
   static RGB grey = { 48, 48, 48, 0 };
   static RGB white = { 63, 63, 63, 0 };

   set_color(0, &black);
   set_color(16, &black);
   set_color(1, &grey);
   set_color(255, &white);

   gui_fg_color = palette_color[0];
   gui_bg_color = palette_color[1];
}



static void intro_screen(void)
{
   BITMAP *bmp;
   play_sample(data[INTRO_SPL].dat, 255, 128, 1000, FALSE);

   bmp =
       create_sub_bitmap(screen, SCREEN_W / 2 - 160, SCREEN_H / 2 - 100,
                         320, 200);
   play_memory_fli(data[INTRO_ANIM].dat, bmp, FALSE, NULL);
   destroy_bitmap(bmp);

   rest(1000);
   fade_out(1);
}



int main(int argc, char *argv[])
{
   int c, w, h;
   char buf[256], buf2[256];
   ANIMATION_TYPE type;

   for (c = 1; c < argc; c++) {
      if (stricmp(argv[c], "-cheat") == 0)
         cheat = TRUE;

      if (stricmp(argv[c], "-jumpstart") == 0)
         jumpstart = TRUE;
   }

   /* The fonts we are using don't contain the full latin1 charset (not to
    * mention Unicode), so in order to display correctly author names in
    * the credits with 8-bit characters, we will convert them down to 7
    * bits with a custom mapping table. Fortunately, Allegro comes with a
    * default custom mapping table which reduces Latin-1 and Extended-A
    * characters to 7 bits. We don't even need to call set_ucodepage()!
    */
   set_uformat(U_ASCII_CP);
   
   srand(time(NULL));
   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_timer();
   install_mouse();

   if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) != 0) {
      allegro_message("Error initialising sound\n%s\n", allegro_error);
      install_sound(DIGI_NONE, MIDI_NONE, NULL);
   }

   if (install_joystick(JOY_TYPE_AUTODETECT) != 0) {
      allegro_message("Error initialising joystick\n%s\n", allegro_error);
      install_joystick(JOY_TYPE_NONE);
   }

   if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) != 0) {   //GFX_AUTODETECT
      if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
         set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
         allegro_message("Unable to set any graphic mode\n%s\n",
                         allegro_error);
         return 1;
      }
   }

   get_executable_name(buf, sizeof(buf));
   replace_filename(buf2, buf, "demo.dat", sizeof(buf2));
   set_color_conversion(COLORCONV_NONE);
   data = load_datafile(buf2);
   if (!data) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error loading %s\n", buf2);
      exit(1);
   }

   if (!jumpstart) {
      intro_screen();
   }

   clear_bitmap(screen);
   set_gui_colors();
   set_mouse_sprite(NULL);

   if (!gfx_mode_select(&c, &w, &h))
      exit(1);

   if (pick_animation_type(&type) < 0)
      exit(1);

   init_display(c, w, h, type);

   generate_explosions();

   //stop_sample(data[INTRO_SPL].dat);

   clear_bitmap(screen);

   while (title_screen())
      play_game();

   destroy_display();

   destroy_explosions();

   stop_midi();

   set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

   end_title();

   unload_datafile(data);

   allegro_exit();

   return 0;
}

END_OF_MAIN()
