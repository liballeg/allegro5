#ifndef __al_included_allegro5_allegro_physfs_intern_h
#define __al_included_allegro5_allegro_physfs_intern_h

#define __FS_PHYS_CWD_MAX 1024

void _al_set_physfs_fs_interface(void);
const ALLEGRO_FILE_INTERFACE *_al_get_phys_vtable(void);
const char * _al_fs_phys_get_path_with_cwd(const char * path, char * buffer);

#endif
