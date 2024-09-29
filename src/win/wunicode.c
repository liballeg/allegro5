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

A5O_DEBUG_CHANNEL("wunicode")

/* _al_win_ustr_to_utf16:
 * Convert A5O_USTR to newly-allocated UTF-16 buffer
 */
wchar_t *_al_win_ustr_to_utf16(const A5O_USTR *u)
{
   int wslen;
   wchar_t *ws;
   const char* us = al_cstr(u);
   size_t uslen = al_ustr_size(u);

   if (uslen == 0) {
      ws = al_malloc(sizeof(wchar_t));
      ws[0] = 0;
      return ws;
   }
   wslen = MultiByteToWideChar(CP_UTF8, 0, us, uslen, NULL, 0);
   if (wslen == 0) {
       A5O_ERROR("MultiByteToWideChar failed\n");
       return NULL;
   }
   /* For the NUL at the end. */
   wslen += 1;
   ws = al_malloc(sizeof(wchar_t) * wslen);
   if (!ws) {
       A5O_ERROR("Out of memory\n");
       return NULL;
   }
   if (0 == MultiByteToWideChar(CP_UTF8, 0, us, uslen, ws, wslen)) {
       al_free(ws);
       A5O_ERROR("MultiByteToWideChar failed\n");
       return NULL;
   }
   ws[wslen - 1] = 0;
   return ws;
}

/* _al_win_utf8_to_ansi:
 * Convert UTF-8 to newly-allocated ansi buffer
 */
char* _al_win_ustr_to_ansi(const A5O_USTR *u) {
   int wslen;
   wchar_t *ws;
   int slen;
   char *s;
   const char* us = al_cstr(u);
   size_t uslen = al_ustr_size(u);

   if (uslen == 0) {
      s = al_malloc(sizeof(char));
      s[0] = 0;
      return s;
   }
   wslen = MultiByteToWideChar(CP_UTF8, 0, us, uslen, NULL, 0);
   if (wslen == 0) {
       A5O_ERROR("MultiByteToWideChar failed\n");
       return NULL;
   }
   ws = al_malloc(sizeof(wchar_t) * wslen);
   if (!ws) {
       A5O_ERROR("Out of memory\n");
       return NULL;
   }
   if (0 == MultiByteToWideChar(CP_UTF8, 0, us, uslen, ws, wslen)) {
       al_free(ws);
       A5O_ERROR("MultiByteToWideChar failed\n");
       return NULL;
   }
   slen = WideCharToMultiByte(CP_ACP, 0, ws, wslen, NULL, 0, NULL, NULL);
   if (slen == 0) {
       A5O_ERROR("WideCharToMultiByte failed\n");
       al_free(ws);
       return NULL;
   }
   /* For the NUL at the end. */
   slen += 1;
   s = al_malloc(sizeof(char) * slen);
   if (!s) {
       A5O_ERROR("Out of memory\n");
       al_free(ws);
       return NULL;
   }
   if (0 == WideCharToMultiByte(CP_ACP, 0, ws, wslen, s, slen, NULL, NULL)) {
       al_free(ws);
       al_free(s);
       A5O_ERROR("WideCharToMultiByte failed\n");
       return NULL;
   }
   s[slen - 1] = 0;
   al_free(ws);
   return s;
}

/* _al_win_utf8_to_utf16:
 * Convert UTF-8 to newly-allocated UTF-16 buffer
 */
wchar_t *_al_win_utf8_to_utf16(const char *us)
{
   int wslen;
   wchar_t *ws;

   if (us == NULL) {
      return NULL;
   }
   wslen = MultiByteToWideChar(CP_UTF8, 0, us, -1, NULL, 0);
   if (wslen == 0) {
       A5O_ERROR("MultiByteToWideChar failed\n");
       return NULL;
   }
   ws = al_malloc(sizeof(wchar_t) * wslen);
   if (!ws) {
       A5O_ERROR("Out of memory\n");
       return NULL;
   }
   if (0 == MultiByteToWideChar(CP_UTF8, 0, us, -1, ws, wslen)) {
       al_free(ws);
       A5O_ERROR("MultiByteToWideChar failed\n");
       return NULL;
   }
   return ws;
}

/* _al_win_utf16_to_utf8:
 * Convert UTF-16 to newly-allocated UTF-8 buffer
 */
char *_al_win_utf16_to_utf8(const wchar_t *ws)
{
   int uslen;
   char *us;

   if (ws == NULL) {
      return NULL;
   }
   uslen = WideCharToMultiByte(CP_UTF8, 0, ws, -1, NULL, 0, NULL, NULL);
   if (uslen == 0) {
       A5O_ERROR("WideCharToMultiByte failed\n");
       return NULL;
   }
   us = al_malloc(sizeof(char) * uslen);
   if (!us) {
       A5O_ERROR("Out of memory\n");
       return NULL;
   }
   if (0 == WideCharToMultiByte(CP_UTF8, 0, ws, -1, us, uslen, NULL, NULL)) {
       al_free(us);
       A5O_ERROR("WideCharToMultiByte failed\n");
       return NULL;
   }
   return us;
}
/* _al_win_utf8_to_ansi:
 * Convert UTF-8 to newly-allocated ansi buffer
 */
char* _al_win_utf8_to_ansi(const char *us) {
   int wslen;
   wchar_t *ws;
   int slen;
   char *s;

   if (us == NULL) {
      return NULL;
   }
   wslen = MultiByteToWideChar(CP_UTF8, 0, us, -1, NULL, 0);
   if (wslen == 0) {
       A5O_ERROR("MultiByteToWideChar failed\n");
       return NULL;
   }
   ws = al_malloc(sizeof(wchar_t) * wslen);
   if (!ws) {
       A5O_ERROR("Out of memory\n");
       return NULL;
   }
   if (0 == MultiByteToWideChar(CP_UTF8, 0, us, -1, ws, wslen)) {
       al_free(ws);
       A5O_ERROR("MultiByteToWideChar failed\n");
       return NULL;
   }
   slen = WideCharToMultiByte(CP_ACP, 0, ws, -1, NULL, 0, NULL, NULL);
   if (slen == 0) {
       A5O_ERROR("WideCharToMultiByte failed\n");
       al_free(ws);
       return NULL;
   }
   s = al_malloc(sizeof(char) * slen);
   if (!s) {
       A5O_ERROR("Out of memory\n");
       al_free(ws);
       return NULL;
   }
   if (0 == WideCharToMultiByte(CP_ACP, 0, ws, -1, s, slen, NULL, NULL)) {
       al_free(ws);
       al_free(s);
       A5O_ERROR("WideCharToMultiByte failed\n");
       return NULL;
   }
   al_free(ws);
   return s;
}

/* _al_win_ansi_to_utf8:
 * Convert ansi to newly-allocated UTF-8 buffer
 */
char* _al_win_ansi_to_utf8(const char *s) {
   int wslen;
   wchar_t *ws;
   int uslen;
   char *us;

   if (s == NULL) {
      return NULL;
   }
   wslen = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
   if (wslen == 0) {
      A5O_ERROR("MultiByteToWideChar failed\n");
      return NULL;
   }
   ws = al_malloc(sizeof(wchar_t) * wslen);
   if (!ws) {
      A5O_ERROR("Out of memory\n");
      return NULL;
   }
   if (0 == MultiByteToWideChar(CP_ACP, 0, s, -1, ws, wslen)) {
      al_free(ws);
      A5O_ERROR("MultiByteToWideChar failed\n");
      return NULL;
   }
   uslen = WideCharToMultiByte(CP_UTF8, 0, ws, -1, NULL, 0, NULL, NULL);
   if (uslen == 0) {
      A5O_ERROR("WideCharToMultiByte failed\n");
      al_free(ws);
      return NULL;
   }
   us = al_malloc(sizeof(char) * uslen);
   if (!s) {
      A5O_ERROR("Out of memory\n");
      al_free(ws);
      return NULL;
   }
   if (0 == WideCharToMultiByte(CP_UTF8, 0, ws, -1, us, uslen, NULL, NULL)) {
      al_free(ws);
      al_free(us);
      A5O_ERROR("WideCharToMultiByte failed\n");
      return NULL;
   }
   al_free(ws);
   return us;
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
char *_al_win_copy_utf16_to_utf8(char* us, const wchar_t *ws, size_t uslen)
{
   int rc = WideCharToMultiByte(CP_UTF8, 0, ws, -1, us, uslen, NULL, NULL);
   if (rc == 0) {
       A5O_ERROR("WideCharToMultiByte failed\n");
       return NULL;
   }
   return us;
}

char *_al_win_copy_utf8_to_ansi(char* s, const char *us, size_t slen)
{
    int wslen = MultiByteToWideChar(CP_UTF8, 0, us, -1, NULL, 0);
    if (wslen == 0) {
       A5O_ERROR("MultiByteToWideChar failed\n");
       return NULL;
    }
    wchar_t* ws = al_malloc(wslen * sizeof(wchar_t));
    if (!ws) {
       A5O_ERROR("Out of memory\n");
       return NULL;
    }
    MultiByteToWideChar(CP_UTF8, 0, us, -1, ws, wslen);
    int rc = WideCharToMultiByte(CP_ACP, 0, ws, wslen, s, slen, NULL, NULL);
    al_free(ws);
    if (rc == 0) {
       A5O_ERROR("WideCharToMultiByte failed\n");
       return NULL;
    }
    return s;
}

char *_al_win_copy_ansi_to_utf8(char* us, const char *s, size_t uslen)
{
    int wslen = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
    if (wslen == 0) {
       A5O_ERROR("MultiByteToWideChar failed\n");
       return NULL;
    }
    wchar_t* ws = al_malloc(wslen * sizeof(wchar_t));
    if (!ws) {
       A5O_ERROR("Out of memory\n");
       return NULL;
    }
    MultiByteToWideChar(CP_ACP, 0, s, -1, ws, wslen);
    int rc = WideCharToMultiByte(CP_UTF8, 0, ws, wslen, us, uslen, NULL, NULL);
    al_free(ws);
    if (rc == 0) {
       A5O_ERROR("WideCharToMultiByte failed\n");
       return NULL;
    }
    return us;
}



/* vim: set sts=3 sw=3 et: */
