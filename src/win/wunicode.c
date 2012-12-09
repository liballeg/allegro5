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

ALLEGRO_DEBUG_CHANNEL("wunicode")


wchar_t *_al_win_utf16(const char *s)
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


char *_al_win_utf8(const wchar_t *ws)
{
   int slen;
   char *s;

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


/* vim: set sts=3 sw=3 et: */
