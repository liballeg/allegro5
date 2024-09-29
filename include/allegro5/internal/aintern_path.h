#ifndef __al_included_allegro5_aintern_path_h
#define __al_included_allegro5_aintern_path_h

struct A5O_PATH {
   A5O_USTR *drive;
   A5O_USTR *filename;
   _AL_VECTOR segments;    /* vector of A5O_USTR * */
   A5O_USTR *basename;
   A5O_USTR *full_string;
};

#endif

/* vim: set sts=3 sw=3 et: */
