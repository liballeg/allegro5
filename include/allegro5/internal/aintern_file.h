#ifndef __al_included_allegro5_aintern_file_h
#define __al_included_allegro5_aintern_file_h

#ifdef __cplusplus
   extern "C" {
#endif


extern const A5O_FILE_INTERFACE _al_file_interface_stdio;

#define A5O_UNGETC_SIZE 16

struct A5O_FILE
{
   const A5O_FILE_INTERFACE *vtable;
   void *userdata;
   unsigned char ungetc[A5O_UNGETC_SIZE];
   int ungetc_len;
};

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
