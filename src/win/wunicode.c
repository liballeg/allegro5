/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Windows "UNICODE" helper routines.
 *
 *      See LICENSE.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_wunicode.h"

#include <windows.h>

ALLEGRO_DEBUG_CHANNEL("wunicode")

/* _al_win_utf8_to_utf16:
 * Convert UTF-8 to newly-allocated UTF-16 buffer
 */
wchar_t *_al_win_utf8_to_utf16(const char *s)
{
   int wslen;
   wchar_t *ws;

   wslen = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
   if (wslen == 0) {
       ALLEGRO_ERROR("MultiByteToWideChar failed\n");
       return NULL;
   }
   ws = al_malloc(sizeof(wchar_t) * wslen);
   if (!ws) {
       ALLEGRO_ERROR("Out of memory\n");
       return NULL;
   }
   if (0 == MultiByteToWideChar(CP_UTF8, 0, s, -1, ws, wslen)) {
       al_free(ws);
       ALLEGRO_ERROR("MultiByteToWideChar failed\n");
       return NULL;
   }
   return ws;
}

/* _al_win_utf16_to_utf8:
 * Convert UTF-16 to newly-allocated UTF-8 buffer
 */
char *_al_win_utf16_to_utf8(const wchar_t *ws)
{
   int slen;
   char *s;

   if (ws == NULL) {
      return NULL;
   }
   slen = WideCharToMultiByte(CP_UTF8, 0, ws, -1, NULL, 0, NULL, NULL);
   if (slen == 0) {
       ALLEGRO_ERROR("WideCharToMultiByte failed\n");
       return NULL;
   }
   s = al_malloc(sizeof(char) * slen);
   if (!s) {
       ALLEGRO_ERROR("Out of memory\n");
       return NULL;
   }
   if (0 == WideCharToMultiByte(CP_UTF8, 0, ws, -1, s, slen, NULL, NULL)) {
       al_free(s);
       ALLEGRO_ERROR("WideCharToMultiByte failed\n");
       return NULL;
   }
   return s;
}
/* _al_win_utf8_to_ansi:
 * Convert UTF-8 to newly-allocated ansi buffer
 */
char* _al_win_utf8_to_ansi(const char *u) {
   int wslen;
   wchar_t *ws;
   int slen;
   char *s;

   if (u == NULL) {
      return NULL;
   }
   wslen = MultiByteToWideChar(CP_UTF8, 0, u, -1, NULL, 0);
   if (wslen == 0) {
       ALLEGRO_ERROR("MultiByteToWideChar failed\n");
       return NULL;
   }
   ws = al_malloc(sizeof(wchar_t) * wslen);
   if (!ws) {
       ALLEGRO_ERROR("Out of memory\n");
       return NULL;
   }
   if (0 == MultiByteToWideChar(CP_UTF8, 0, u, -1, ws, wslen)) {
       al_free(ws);
       ALLEGRO_ERROR("MultiByteToWideChar failed\n");
       return NULL;
   }
   slen = WideCharToMultiByte(CP_ACP, 0, ws, -1, NULL, 0, NULL, NULL);
   if (slen == 0) {
       ALLEGRO_ERROR("WideCharToMultiByte failed\n");
       al_free(ws);
       return NULL;
   }
   s = al_malloc(sizeof(char) * slen);
   if (!s) {
       ALLEGRO_ERROR("Out of memory\n");
       al_free(ws);
       return NULL;
   }
   if (0 == WideCharToMultiByte(CP_ACP, 0, ws, -1, s, slen, NULL, NULL)) {
       al_free(ws);
       al_free(s);
       ALLEGRO_ERROR("WideCharToMultiByte failed\n");
       return NULL;
   }
   al_free(ws);
   return s;
}

/* _al_win_ansi_to_utf8:
 * Convert UTF-8 to newly-allocated ansi buffer
 */
char* _al_win_ansi_to_utf8(const char *u) {
   int wslen;
   wchar_t *ws;
   int slen;
   char *s;

   if (u == NULL) {
      return NULL;
   }
   wslen = MultiByteToWideChar(CP_ACP, 0, u, -1, NULL, 0);
   if (wslen == 0) {
      ALLEGRO_ERROR("MultiByteToWideChar failed\n");
      return NULL;
   }
   ws = al_malloc(sizeof(wchar_t) * wslen);
   if (!ws) {
      ALLEGRO_ERROR("Out of memory\n");
      return NULL;
   }
   if (0 == MultiByteToWideChar(CP_ACP, 0, u, -1, ws, wslen)) {
      al_free(ws);
      ALLEGRO_ERROR("MultiByteToWideChar failed\n");
      return NULL;
   }
   slen = WideCharToMultiByte(CP_UTF8, 0, ws, -1, NULL, 0, NULL, NULL);
   if (slen == 0) {
      ALLEGRO_ERROR("WideCharToMultiByte failed\n");
      al_free(ws);
      return NULL;
   }
   s = al_malloc(sizeof(char) * slen);
   if (!s) {
      ALLEGRO_ERROR("Out of memory\n");
      al_free(ws);
      return NULL;
   }
   if (0 == WideCharToMultiByte(CP_UTF8, 0, ws, -1, s, slen, NULL, NULL)) {
      al_free(ws);
      al_free(s);
      ALLEGRO_ERROR("WideCharToMultiByte failed\n");
      return NULL;
   }
   al_free(ws);
   return s;
}

/* _al_win_copy_utf16_to_utf8: Copy string and convert to UTF-8.
 * This takes a string and copies a UTF-8 version of it to
 * a buffer of fixed size.
 * If successful, the string will be zero-terminated and is 
 * the return value of the function.
 * If unsuccesful, NULL will be returned.
 * If the representation would overflow the buffer, nothing
 * is copied and the return value is NULL.
 */ 
char *_al_win_copy_utf16_to_utf8(char* u, const wchar_t *ws, size_t size)
{
   int rc = WideCharToMultiByte(CP_UTF8, 0, ws, -1, u, (int) size, NULL, NULL);
   if (rc == 0) {
       ALLEGRO_ERROR("WideCharToMultiByte failed\n");
       return NULL;
   }
   return u;
}
char *_al_win_copy_utf8_to_ansi(char* s, const char *u, size_t size)
{
   int slen = MultiByteToWideChar(CP_UTF8, 0, u, -1, NULL, 0);
    if (slen == 0) {
       ALLEGRO_ERROR("MultiByteToWideChar failed\n");
       return NULL;
    }
    wchar_t* ws = al_malloc(slen * sizeof(wchar_t));
    if (!ws) {
       ALLEGRO_ERROR("Out of memory\n");
       return NULL;
    }
    MultiByteToWideChar(CP_UTF8, 0, u, -1, ws, slen);
    int rc = WideCharToMultiByte(CP_ACP, 0, ws, slen, s, (int) size, NULL, NULL);
    al_free(ws);
    if (rc == 0) {
       ALLEGRO_ERROR("WideCharToMultiByte failed\n");
       return NULL;
    }
    return s;
}

char *_al_win_copy_ansi_to_utf8(char* u, const char *s, size_t size)
{
    int slen = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
    if (slen == 0) {
       ALLEGRO_ERROR("MultiByteToWideChar failed\n");
       return NULL;
    }
    wchar_t* ws = al_malloc(slen * sizeof(wchar_t));
    if (!ws) {
       ALLEGRO_ERROR("Out of memory\n");
       return NULL;
    }
    MultiByteToWideChar(CP_ACP, 0, s, -1, ws, slen);
    int rc = WideCharToMultiByte(CP_UTF8, 0, ws, slen, u, (int) size, NULL, NULL);
    al_free(ws);
    if (rc == 0) {
       ALLEGRO_ERROR("WideCharToMultiByte failed\n");
       return NULL;
    }
    return u;
}



/* vim: set sts=3 sw=3 et: */
