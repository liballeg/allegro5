#ifndef __al_included_allegro_aintern_native_dialog_h
#define __al_included_allegro_aintern_native_dialog_h

typedef struct ALLEGRO_NATIVE_DIALOG ALLEGRO_NATIVE_DIALOG;

/* We could use different structs for the different dialogs. But why
 * bother.
 */
struct ALLEGRO_NATIVE_DIALOG
{
   ALLEGRO_USTR *title;
   int flags;

   /* Only used by file chooser. */
   ALLEGRO_PATH *fc_initial_path;
   size_t fc_path_count;
   ALLEGRO_PATH **fc_paths;
   ALLEGRO_USTR *fc_patterns;

   /* Only used by message box. */
   ALLEGRO_USTR *mb_heading;
   ALLEGRO_USTR *mb_text;
   ALLEGRO_USTR *mb_buttons;
   int mb_pressed_button;

   /* Only used by text log. */
   ALLEGRO_THREAD *tl_thread;
   ALLEGRO_COND *tl_text_cond;
   ALLEGRO_MUTEX *tl_text_mutex;
   ALLEGRO_USTR *tl_pending_text;
   bool tl_init_error;
   bool tl_done;
   bool tl_have_pending;
   ALLEGRO_EVENT_SOURCE tl_events;
   void *tl_textview;

   /* Only used by platform implementations. */
   bool is_active;
   void *window;
   void *async_queue;
};

extern bool _al_init_native_dialog_addon(void);
extern void _al_shutdown_native_dialog_addon(void);
extern bool _al_show_native_file_dialog(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd);
extern int _al_show_native_message_box(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd);
extern bool _al_open_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog);
extern void _al_close_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog);
extern void _al_append_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog);

#endif
