#ifndef __al_included_a5_aintern_native_dialog_h
#define __al_included_a5_aintern_native_dialog_h

struct ALLEGRO_NATIVE_DIALOG
{
   ALLEGRO_PATH *initial_path;
   ALLEGRO_USTR *title;
   ALLEGRO_USTR *heading;
   ALLEGRO_USTR *text;
   ALLEGRO_USTR *patterns;
   ALLEGRO_USTR *buttons;
   ALLEGRO_PATH **paths;

   int mode;
   size_t count;
   int pressed_button;

   /* Only used by platform implementations. */
   bool is_active;
   ALLEGRO_COND *cond;
};

extern int _al_show_native_message_box(ALLEGRO_NATIVE_DIALOG *fd);

#endif
