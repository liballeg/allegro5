#ifndef __al_included_internal_aintern_file_h
#define __al_included_internal_aintern_file_h

#ifdef __cplusplus
   extern "C" {
#endif


ALLEGRO_FILE *_al_file_stdio_fopen(const char *path, const char *mode);

extern const ALLEGRO_FILE_INTERFACE _al_file_interface_stdio;


#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
