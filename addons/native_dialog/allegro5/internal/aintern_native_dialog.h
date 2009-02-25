#ifndef __al_included_a5_aintern_native_dialog_h
#define __al_included_a5_aintern_native_dialog_h

struct ALLEGRO_NATIVE_FILE_DIALOG
{
   ALLEGRO_PATH *initial_path;
   ALLEGRO_USTR *title;
   ALLEGRO_USTR *patterns;
   ALLEGRO_PATH **paths;
   int mode;
   size_t count;
};

#endif
