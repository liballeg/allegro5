#ifndef __al_included_allegro5_aintern_wunicode_h
#define __al_included_allegro5_aintern_wunicode_h

#ifdef ALLEGRO_WINDOWS

#ifdef __cplusplus
   extern "C" {
#endif


AL_FUNC(wchar_t *, _al_win_utf16, (const char *s));
AL_FUNC(char *, _al_win_utf8, (const wchar_t *ws));


#ifdef __cplusplus
   }
#endif

#endif

#endif

/* vim: set ts=8 sts=3 sw=3 et: */
