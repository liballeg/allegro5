#ifndef __al_included_aintern_path_h
#define __al_included_aintern_path_h

struct ALLEGRO_PATH {
   bool free;

   char *drive;
   char *filename;

   int32_t segment_count;
   char **segment;
};

#endif

/* vim: set sts=3 sw=3 et: */
