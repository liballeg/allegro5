#ifndef __al_included_internal_aintern_file_h
#define __al_included_internal_aintern_file_h

#ifdef __cplusplus
   extern "C" {
#endif


typedef struct ALLEGRO_FILE_INTERFACE ALLEGRO_FILE_INTERFACE;

struct ALLEGRO_FILE
{
   const ALLEGRO_FILE_INTERFACE *vtable;
};

struct ALLEGRO_FILE_INTERFACE
{
   AL_METHOD(ALLEGRO_FILE*, fi_fopen, (const char *path, const char *mode));
   AL_METHOD(void,    fi_fclose, (ALLEGRO_FILE *handle));
   AL_METHOD(size_t,  fi_fread, (ALLEGRO_FILE *f, void *ptr, size_t size));
   AL_METHOD(size_t,  fi_fwrite, (ALLEGRO_FILE *f, const void *ptr, size_t size));
   AL_METHOD(bool,    fi_fflush, (ALLEGRO_FILE *f));
   AL_METHOD(int64_t, fi_ftell, (ALLEGRO_FILE *f));
   AL_METHOD(bool,    fi_fseek, (ALLEGRO_FILE *f, int64_t offset, int whence));
   AL_METHOD(bool,    fi_feof, (ALLEGRO_FILE *f));
   AL_METHOD(bool,    fi_ferror, (ALLEGRO_FILE *f));
   AL_METHOD(int,     fi_fungetc, (ALLEGRO_FILE *f, int c));
   AL_METHOD(off_t,   fi_fsize, (ALLEGRO_FILE *f));
};


extern const ALLEGRO_FILE_INTERFACE _al_file_interface_stdio;


#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
