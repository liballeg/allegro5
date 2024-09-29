#ifndef __al_included_allegro5_allegro_physfs_intern_h
#define __al_included_allegro5_allegro_physfs_intern_h

void _al_set_physfs_fs_interface(void);
const A5O_FILE_INTERFACE *_al_get_phys_vtable(void);
A5O_USTR *_al_physfs_process_path(const char *path);

#endif
