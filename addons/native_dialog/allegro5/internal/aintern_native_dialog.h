#ifndef __al_included_allegro_aintern_native_dialog_h
#define __al_included_allegro_aintern_native_dialog_h

/* We could use different structs for the different dialogs. But why
 * bother.
 */
struct ALLEGRO_NATIVE_DIALOG
{
   ALLEGRO_USTR *title;
   ALLEGRO_USTR *text;
   int mode;

   /* Only used by file chooser. */
   ALLEGRO_PATH *initial_path;
   size_t count;
   ALLEGRO_PATH **paths;
   ALLEGRO_USTR *patterns;

   /* Only used by message box. */
   ALLEGRO_USTR *heading;
   ALLEGRO_USTR *buttons;
   int pressed_button;

   /* Only used by text log. */
   ALLEGRO_THREAD *thread;
   ALLEGRO_COND *text_cond;
   ALLEGRO_MUTEX *text_mutex;
   bool done;
   int new_line;
   ALLEGRO_EVENT_SOURCE events;

   /* Only used by platform implementations. */
   bool is_active;
   ALLEGRO_COND *cond;
   void *window;
   void *textview;
};

extern int _al_show_native_message_box(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd);
extern void _al_open_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog);
extern void _al_close_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog);
extern void _al_append_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog);

#endif
