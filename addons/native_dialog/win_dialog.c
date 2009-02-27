/* Each of these files implements the same, for different GUI toolkits:
 * 
 * dialog.c - code shared between all platforms
 * gtk_dialog.c - GTK file open dialog
 * osx_dialog.m - OSX file open dialog
 * qt_dialog.cpp  - Qt file open dialog
 * win_dialog.c - Windows file open dialog
 * 
 */

#include "allegro5/allegro5.h"
#include "allegro5/a5_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "allegro5/internal/aintern_memory.h"

#include "allegro5/platform/aintwin.h"


static int next(char *s)
{
   int i = 0;

   while  (s[i])
      i++;

   return i+1;
}

void al_show_native_file_dialog(ALLEGRO_NATIVE_FILE_DIALOG *fd)
{
   OPENFILENAME ofn;
   ALLEGRO_DISPLAY_WIN *display;
   int flags = 0;
   bool ret;
   char buf[4096] = { 0, };
   int i;

   memset(&ofn, 0, sizeof(OPENFILENAME));

   ofn.lStructSize = sizeof(OPENFILENAME);
   display = (ALLEGRO_DISPLAY_WIN*)al_get_current_display();
   ofn.hwndOwner = display->window;
   if (fd->initial_path) {
      strncpy(buf, al_path_to_string(fd->initial_path, '/'), 4096);
   }

   ofn.lpstrFile = buf;
   ofn.nMaxFile = 4096;

   flags |= OFN_NOCHANGEDIR | OFN_EXPLORER;
   if (fd->mode & ALLEGRO_FILECHOOSER_SAVE) {
   	flags |= OFN_OVERWRITEPROMPT;
   }
   else {
   	flags |= (fd->mode & ALLEGRO_FILECHOOSER_FILE_MUST_EXIST) ? OFN_FILEMUSTEXIST : 0;
   }
   flags |= (fd->mode & ALLEGRO_FILECHOOSER_MULTIPLE) ? OFN_ALLOWMULTISELECT : 0;
   flags |= (fd->mode & ALLEGRO_FILECHOOSER_SHOW_HIDDEN) ? 0x10000000 : 0; // NOTE: 0x10000000 is FORCESHOWHIDDEN
   ofn.Flags = flags;

   if (flags & OFN_OVERWRITEPROMPT) {
      ret = GetSaveFileName(&ofn);
   }
   else {
      ret = GetOpenFileName(&ofn);
   }

   if (!ret) {
      DWORD err = GetLastError();
      char buf[1000];
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, buf, 1000, NULL);
      TRACE("al_show_native_file_dialog failed or cancelled: %s\n", buf);
      return;
   }

   if (flags & OFN_ALLOWMULTISELECT) {
      int p;
      char path[1000];

      /* Copy the path portion */
      strncpy(path, buf, 999);
      strcat(path, "/");

      /* Skip path portion */
      i = next(buf);

      /* Count selected */
      fd->count = 0;
      while (1) {
   	   if (buf[i] == 0) {
   	      fd->count++;
	      if (buf[i+1] == 0)
	         break;
           }
	   i++;
      }

      fd->paths = _AL_MALLOC(fd->count * sizeof(void *));
      i = next(buf);
      for (p = 0; p < (int)fd->count; p++) {
      	 fd->paths[p] = al_path_create(path);
	 al_path_concat(fd->paths[p], al_path_create(buf+i));
	 i += next(buf+i);
      }
   }
   else {
   	fd->count = 1;
	fd->paths = _AL_MALLOC(sizeof(void *));
	fd->paths[0] = al_path_create(buf);
   }
}
