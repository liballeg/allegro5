#ifndef __al_included_allegro5_allegro_physfs_intern_h
#define __al_included_allegro5_allegro_physfs_intern_h


void _al_set_physfs_fs_interface(void);

ALLEGRO_FILE *_al_file_phys_fopen(const char *filename, const char *mode);


#endif
