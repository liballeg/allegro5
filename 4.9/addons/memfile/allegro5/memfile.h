#ifndef ALLEGRO_MEMFILE_H
#define ALLEGRO_MEMFILE_H

#ifdef __cplusplus
extern "C" {
#endif

AL_FUNC(ALLEGRO_FS_ENTRY *, al_open_memfile, (int64_t size, void *mem));

#ifdef __cplusplus
}
#endif

#endif /* ALLEGRO_MEMFILE_H */
