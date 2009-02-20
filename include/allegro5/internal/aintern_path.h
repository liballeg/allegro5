#ifndef __al_included_aintern_path_h
#define __al_included_aintern_path_h

struct ALLEGRO_PATH {
   ALLEGRO_USTR drive;
   ALLEGRO_USTR filename;
   _AL_VECTOR segments;    /* vector of ALLEGRO_USTR */
};

#endif

/* vim: set sts=3 sw=3 et: */
