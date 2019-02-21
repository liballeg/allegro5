/* Each of these files implements the same, for different GUI toolkits:
 *
 * dialog.c - code shared between all platforms
 * gtk_dialog.c - GTK file open dialog
 * osx_dialog.m - OSX file open dialog
 * qt_dialog.cpp  - Qt file open dialog
 * win_dialog.c - Windows file open dialog
 *
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_native_dialog.h"

#include "allegro5/platform/aintwin.h"
#include "allegro5/internal/aintern_wunicode.h"

#include "allegro5/allegro_windows.h"

#include <string.h>

/* We use RichEdit by default. */
#include <richedit.h>
#include <shlobj.h> // for folder selector

ALLEGRO_DEBUG_CHANNEL("win_dialog")

#define WM_SHOW_POPUP (WM_APP + 42)
#define WM_HIDE_MENU (WM_APP + 43)
#define WM_SHOW_MENU (WM_APP + 44)

/* Reference count for shared resources. */
static int wlog_count = 0;

/* Handle of RichEdit module */
static void *wlog_rich_edit_module = 0;

/* Name of the edit control. Depend on system resources. */
static TCHAR* wlog_edit_control = TEXT("EDIT");

/* True if output support unicode */
static bool wlog_unicode = false;

static ALLEGRO_MUTEX* global_mutex;

static ALLEGRO_COND* wm_size_cond;
static bool got_wm_size_event = false;

/* For Unicode support, define some helper functions
 * which work with either UTF-16 or ANSI code pages
 * depending on whether UNICODE is defined or not.
 */ 

/* tcreate_path:
 * Create path from TCHARs
 */
static ALLEGRO_PATH* _tcreate_path(const TCHAR* ts) 
{
   char* tmp = _twin_tchar_to_utf8(ts);
   ALLEGRO_PATH* path = al_create_path(tmp);
   al_free(tmp);
   return path;
}
/* tcreate_path:
 * Create directory path from TCHARs
 */
static ALLEGRO_PATH* _tcreate_path_for_directory(const TCHAR* ts) 
{
   char* tmp = _twin_tchar_to_utf8(ts);
   ALLEGRO_PATH* path = al_create_path_for_directory(tmp);
   al_free(tmp);
   return path;
}

bool _al_init_native_dialog_addon(void)
{
   global_mutex = al_create_mutex();
   wm_size_cond = al_create_cond();
   if (!global_mutex || !wm_size_cond) {
      al_destroy_mutex(global_mutex);
      al_destroy_cond(wm_size_cond);
      return false;
   }
   return true;
}

void _al_shutdown_native_dialog_addon(void)
{
   al_destroy_mutex(global_mutex);
   al_destroy_cond(wm_size_cond);
   global_mutex = NULL;
   wm_size_cond = NULL;
}


static bool select_folder(ALLEGRO_DISPLAY_WIN *win_display,
   ALLEGRO_NATIVE_DIALOG *fd)
{
   BROWSEINFO folderinfo;
   LPCITEMIDLIST pidl;
   /* Selected path */
   TCHAR buf[MAX_PATH] = TEXT("");
   /* Display name */
   TCHAR dbuf[MAX_PATH] = TEXT("");

   folderinfo.hwndOwner = win_display ? win_display->window : NULL;
   folderinfo.pidlRoot = NULL;
   folderinfo.pszDisplayName = dbuf;
   folderinfo.lpszTitle = _twin_ustr_to_tchar(fd->title);
   folderinfo.ulFlags = 0;
   folderinfo.lpfn = NULL;

   pidl = SHBrowseForFolder(&folderinfo);

   al_free((void*) folderinfo.lpszTitle);

   if (pidl) {
      SHGetPathFromIDList(pidl, buf);
      fd->fc_path_count = 1;
      fd->fc_paths = al_malloc(sizeof(void *));
      fd->fc_paths[0] = _tcreate_path(buf);
      return true;
   }
   return false;
}

static ALLEGRO_USTR *create_filter_string(const ALLEGRO_USTR *patterns)
{
   ALLEGRO_USTR *filter = al_ustr_new("");
   bool filter_all = false;
   int start, end;

   /* FIXME: Move all this filter parsing stuff into a common file. */
   if (0 == strcmp(al_cstr(patterns), "*.*")) {
      filter_all = true;
   }
   else {
      al_ustr_append_cstr(filter, "All Supported Files");
      al_ustr_append_chr(filter, '\0');
      start = al_ustr_size(filter);
      al_ustr_append(filter, patterns);

      /* Remove all instances of "*.*", which will be added separately. */
      for (;;) {
         int pos = al_ustr_find_cstr(filter, start, "*.*;");
         if (pos == -1)
            break;
         if (pos == start || al_ustr_get(filter, pos - 1) == ';') {
            filter_all = true;
            al_ustr_remove_range(filter, pos, pos + 4);
            start = pos;
         }
         else {
            start = pos + 4;
         }
      }
      while (al_ustr_has_suffix_cstr(filter, ";*.*")) {
         filter_all = true;
         end = al_ustr_size(filter);
         al_ustr_remove_range(filter, end - 4, end);
      }

      al_ustr_append_chr(filter, '\0');
   }

   if (filter_all) {
      al_ustr_append_cstr(filter, "All Files");
      al_ustr_append_chr(filter, '\0');
      al_ustr_append_cstr(filter, "*.*");
      al_ustr_append_chr(filter, '\0');
   }

   al_ustr_append_chr(filter, '\0');
   return filter;
}

static int skip_nul_terminated_string(TCHAR *s)
{
   int i = 0;

   while (s[i])
      i++;

   return i+1;
}

bool _al_show_native_file_dialog(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd)
{
   OPENFILENAME ofn;
   ALLEGRO_DISPLAY_WIN *win_display;
   int flags = 0;
   bool ret;
   TCHAR buf[4096];
   const int BUFSIZE = sizeof(buf) / sizeof(TCHAR);
   TCHAR* wfilter = NULL;
   TCHAR* wpath = NULL;
   ALLEGRO_USTR *filter_string = NULL;
   ALLEGRO_PATH* initial_dir_path = NULL;

   buf[0] = '\0';

   win_display = (ALLEGRO_DISPLAY_WIN *)display;

   if (fd->flags & ALLEGRO_FILECHOOSER_FOLDER) {
      return select_folder(win_display, fd);
   }

   /* Selecting a file. */
   memset(&ofn, 0, sizeof(OPENFILENAME));
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = win_display ? win_display->window : NULL;

   /* Create filter string. */
   if (fd->fc_patterns) {
      filter_string = create_filter_string(fd->fc_patterns);
      wfilter = _twin_ustr_to_tchar(filter_string);
      ofn.lpstrFilter = wfilter;
   }
   else {
      /* List all files by default. */
      ofn.lpstrFilter = TEXT("All Files\0*.*\0\0");
   }

   /* Provide buffer for file chosen by dialog. */
   ofn.lpstrFile = buf;
   ofn.nMaxFile = BUFSIZE;

   /* Initialize file name buffer and starting directory. */
   if (fd->fc_initial_path) {
      bool is_dir;
      const ALLEGRO_USTR *path = al_path_ustr(fd->fc_initial_path, ALLEGRO_NATIVE_PATH_SEP);

      if (al_filename_exists(al_cstr(path))) {
         ALLEGRO_FS_ENTRY *fs = al_create_fs_entry(al_cstr(path));
         is_dir = al_get_fs_entry_mode(fs) & ALLEGRO_FILEMODE_ISDIR;
         al_destroy_fs_entry(fs);
      }
      else {
         is_dir = false;
      }

      if (is_dir) {
         wpath = _twin_ustr_to_tchar(path);
      }
      else {
         //al_ustr_encode_utf16(path, buf, sizeof(buf));
         /* Extract the directory from the path. */
         initial_dir_path = al_clone_path(fd->fc_initial_path);
         if (initial_dir_path) {
            al_set_path_filename(initial_dir_path, NULL);
            wpath = _twin_utf8_to_tchar(al_path_cstr(initial_dir_path, ALLEGRO_NATIVE_PATH_SEP));
         }
      }
      ofn.lpstrInitialDir = wpath;
   }

   if (fd->title)
      ofn.lpstrTitle = _twin_ustr_to_tchar(fd->title);

   flags |= OFN_NOCHANGEDIR | OFN_EXPLORER;
   if (fd->flags & ALLEGRO_FILECHOOSER_SAVE) {
      flags |= OFN_OVERWRITEPROMPT;
   }
   else {
      flags |= (fd->flags & ALLEGRO_FILECHOOSER_FILE_MUST_EXIST) ? OFN_FILEMUSTEXIST : 0;
   }
   flags |= (fd->flags & ALLEGRO_FILECHOOSER_MULTIPLE) ? OFN_ALLOWMULTISELECT : 0;
   flags |= (fd->flags & ALLEGRO_FILECHOOSER_SHOW_HIDDEN) ? 0x10000000 : 0; // NOTE: 0x10000000 is FORCESHOWHIDDEN
   ofn.Flags = flags;

   if (flags & OFN_OVERWRITEPROMPT) {
      ret = GetSaveFileName(&ofn);
   }
   else {
      ret = GetOpenFileName(&ofn);
   }

   if (initial_dir_path) {
      al_destroy_path(initial_dir_path);
   }
   al_free((void*) ofn.lpstrTitle);
   al_free(wfilter);
   al_free(wpath);
   al_ustr_free(filter_string);

   if (!ret) {
      DWORD err = GetLastError();
      if (err != ERROR_SUCCESS) {
         ALLEGRO_ERROR("al_show_native_file_dialog failed: %s\n", _al_win_error(err));
      }
      return false;
   }

   if (flags & OFN_ALLOWMULTISELECT) {
      int i = 0;
      /* Count number of file names in buf. */
      fd->fc_path_count = 0;
      while (1) {
         int j = skip_nul_terminated_string(buf + i);
         if (j <= 1)
            break;
         fd->fc_path_count++;
         i += j;
      }
   }
   else {
      fd->fc_path_count = 1;
   }

   if (fd->fc_path_count == 1) {
      fd->fc_paths = al_malloc(sizeof(void *));
      fd->fc_paths[0] = _tcreate_path(buf);
   }
   else {
      int i, p;
      /* If multiple files were selected, the first string in buf is the
       * directory name, followed by each of the file names terminated by NUL.
       */
      fd->fc_path_count -= 1;
      fd->fc_paths = al_malloc(fd->fc_path_count * sizeof(void *));
      i = skip_nul_terminated_string(buf);
      for (p = 0; p < (int)fd->fc_path_count; p++) {
         fd->fc_paths[p] = _tcreate_path_for_directory(buf);
         char* tmp = _twin_tchar_to_utf8(buf + i);
         al_set_path_filename(fd->fc_paths[p], tmp);
         al_free(tmp);
         i += skip_nul_terminated_string(buf+i);
      }
   }

   return true;
}

int _al_show_native_message_box(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd)
{
   UINT type = MB_SETFOREGROUND;
   int result;
   TCHAR *text, *title;

   /* Note: the message box code cannot assume that Allegro is installed. */

   if (fd->flags & ALLEGRO_MESSAGEBOX_QUESTION)
      type |= MB_ICONQUESTION;
   else if (fd->flags & ALLEGRO_MESSAGEBOX_WARN)
      type |= MB_ICONWARNING;
   else if (fd->flags & ALLEGRO_MESSAGEBOX_ERROR) 
      type |= MB_ICONERROR;
   else 
      type |= MB_ICONINFORMATION;

   if (fd->flags & ALLEGRO_MESSAGEBOX_YES_NO)
      type |= MB_YESNO;
   else if (fd->flags & ALLEGRO_MESSAGEBOX_OK_CANCEL)
      type |= MB_OKCANCEL;

   /* heading + text are combined together */

   if (al_ustr_size(fd->mb_heading)) 
      al_ustr_append_cstr(fd->mb_heading, "\n\n");

   al_ustr_append(fd->mb_heading, fd->mb_text);

   text = _twin_ustr_to_tchar(fd->mb_heading);
   if (!text) {
      return 0;
   }
   title = _twin_ustr_to_tchar(fd->title);
   if (!title) {
      al_free(text);
      return 0;
   }
   result = MessageBox(al_get_win_window_handle(display),
      text, title, type);

   al_free(text);
   al_free(title);

   if (result == IDYES || result == IDOK)
      return 1;
   else
      return 2;
}


/* Emit close event. */
static void wlog_emit_close_event(ALLEGRO_NATIVE_DIALOG *textlog, bool keypress)
{
   ALLEGRO_EVENT event;
   event.user.type = ALLEGRO_EVENT_NATIVE_DIALOG_CLOSE;
   event.user.timestamp = al_get_time();
   event.user.data1 = (intptr_t)textlog;
   event.user.data2 = (intptr_t)keypress;
   al_emit_user_event(&textlog->tl_events, &event, NULL);
}

/* convert_crlf:
 * Alter this string to change every LF to CRLF
 */
static ALLEGRO_USTR* convert_crlf(ALLEGRO_USTR* s) {
   int pos = 0;
   int ch, prev;
   while ((pos = al_ustr_find_chr(s, pos, '\n')) >= 0) {
      prev = pos;
      ch = al_ustr_prev_get(s, &prev);
      if (ch == -2) {
         return s;
      } else if (ch != '\r') {
         al_ustr_insert_chr(s, pos, '\r');
         al_ustr_next(s, &pos);
      }
      al_ustr_next(s, &pos);
   }
   return s;
}
/* General function to output log message. */
static void wlog_do_append_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   int index;
   index = GetWindowTextLength(textlog->tl_textview);
   SendMessage(textlog->tl_textview, EM_SETSEL, (WPARAM)index, (LPARAM)index);
   convert_crlf(textlog->tl_pending_text);
   TCHAR* buf = _twin_utf8_to_tchar(al_cstr(textlog->tl_pending_text));   
   SendMessage(textlog->tl_textview, EM_REPLACESEL, 0, (LPARAM) buf);
   al_free(buf);
   al_ustr_truncate(textlog->tl_pending_text, 0);
   textlog->tl_have_pending = false;

   SendMessage(textlog->tl_textview, WM_VSCROLL, SB_BOTTOM, 0);
}

/* Text log window callback. */
static LRESULT CALLBACK wlog_text_log_callback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   CREATESTRUCT* create_struct;

   ALLEGRO_NATIVE_DIALOG* textlog = (ALLEGRO_NATIVE_DIALOG*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

   switch (uMsg) {
      case WM_CREATE:
         /* Set user data for window, so we will be able to retrieve text log structure any time */
         create_struct = (CREATESTRUCT*)lParam;
         SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)create_struct->lpCreateParams);
         break;

      case WM_CLOSE:
         if (textlog->is_active) {
            if (!(textlog->flags & ALLEGRO_TEXTLOG_NO_CLOSE)) {
               wlog_emit_close_event(textlog, false);
            }
            return 0;
         }
         break;

      case WM_DESTROY:
         PostQuitMessage(0);
         break;

      case WM_KEYDOWN:
         if (wParam == VK_ESCAPE) {
            wlog_emit_close_event(textlog, true);
         }

         break;

      case WM_MOVE:
         InvalidateRect(hWnd, NULL, FALSE);
         break;

      case WM_SIZE:
         /* Match text log size to parent client area */
         if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) {
            RECT client_rect;
            GetClientRect(hWnd, &client_rect);
            MoveWindow(textlog->tl_textview, client_rect.left, client_rect.top,
               client_rect.right - client_rect.left, client_rect.bottom - client_rect.top, TRUE);
         }
         break;

      case WM_USER:
         al_lock_mutex(textlog->tl_text_mutex);
         wlog_do_append_native_text_log(textlog);
         al_unlock_mutex(textlog->tl_text_mutex);
         break;
   }

   return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


/* We hold textlog->tl_text_mutex. */
static bool open_native_text_log_inner(ALLEGRO_NATIVE_DIALOG *textlog)
{
   LPCTSTR font_name;
   HWND hWnd;
   HWND hLog;
   RECT client_rect;
   HFONT hFont;
   MSG msg;
   BOOL ret;
   TCHAR* title;

   /* Create text log window. */
   title = _twin_ustr_to_tchar(textlog->title);
   hWnd = CreateWindow(TEXT("Allegro Text Log"), title,
      WS_CAPTION | WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU,
      CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL,
      (HINSTANCE)GetModuleHandle(NULL), textlog);
   al_free(title);
   if (!hWnd) {
      ALLEGRO_ERROR("CreateWindow failed\n");
      return false;
   }

   /* Get client area of the log window. */
   GetClientRect(hWnd, &client_rect);

   /* Create edit control. */
   hLog = CreateWindow(wlog_edit_control, NULL,
      WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL | ES_READONLY,
      client_rect.left, client_rect.top, client_rect.right - client_rect.left, client_rect.bottom - client_rect.top,
      hWnd, NULL, (HINSTANCE)GetModuleHandle(NULL), NULL);
   if (!hLog) {
      ALLEGRO_ERROR("CreateWindow failed\n");
      DestroyWindow(hWnd);
      return false;
   }

   /* Enable double-buffering. */
   SetWindowLong(hLog, GWL_EXSTYLE, GetWindowLong(hLog, GWL_EXSTYLE) | 0x02000000L/*WS_EX_COMPOSITED*/);

   /* Select font name. */
   if (textlog->flags & ALLEGRO_TEXTLOG_MONOSPACE)
      font_name = TEXT("Courier New");
   else
      font_name = TEXT("Arial");

   /* Create font and set font. */
   hFont = CreateFont(-11, 0, 0, 0, FW_LIGHT, 0, 0,
      0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
      FF_MODERN | FIXED_PITCH, font_name);

   /* Assign font to the text log. */
   if (hFont) {
      SendMessage(hLog, WM_SETFONT, (WPARAM)hFont, 0);
   }

   /* We are ready to show our window. */
   ShowWindow(hWnd, SW_NORMAL);

   /* Save handles for future use. */
   textlog->window    = hWnd;
   textlog->tl_textview  = hLog;
   textlog->is_active = true;

   /* Now notify al_show_native_textlog that the text log is ready. */
   textlog->tl_done = true;
   al_signal_cond(textlog->tl_text_cond);
   al_unlock_mutex(textlog->tl_text_mutex);

   /* Process messages. */
   while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
      if (ret != -1 && msg.message != WM_QUIT) {
         /* Intercept child window key down messages. Needed to track
          * hit of ESCAPE key while text log have focus. */
         if (msg.hwnd != textlog->window && msg.message == WM_KEYDOWN) {
            PostMessage(textlog->window, WM_KEYDOWN, msg.wParam, msg.lParam);
         }
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
      else
         break;
   }

   /* Close window. Should be already closed, this is just sanity. */
   if (IsWindow(textlog->window)) {
      DestroyWindow(textlog->window);
   }

   /* Release font. We don't want to leave any garbage. */
   DeleteObject(hFont);

   /* Notify everyone that we're gone. */
   al_lock_mutex(textlog->tl_text_mutex);
   textlog->tl_done = true;
   al_signal_cond(textlog->tl_text_cond);

   return true;
}

bool _al_open_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   WNDCLASS text_log_class;
   bool rc;

   al_lock_mutex(textlog->tl_text_mutex);

   /* Note: the class registration and rich edit module loading are not
    * implemented in a thread-safe manner (pretty unlikely).
    */

   /* Prepare text log class info. */
   if (wlog_count == 0) {
      ALLEGRO_DEBUG("Register text log class\n");

      memset(&text_log_class, 0, sizeof(text_log_class));
      text_log_class.hInstance      = (HINSTANCE)GetModuleHandle(NULL);
      text_log_class.lpszClassName  = TEXT("Allegro Text Log");
      text_log_class.lpfnWndProc    = wlog_text_log_callback;
      text_log_class.hIcon          = NULL;
      text_log_class.hCursor        = NULL;
      text_log_class.lpszMenuName   = NULL;
      text_log_class.hbrBackground  = (HBRUSH)GetStockObject(GRAY_BRUSH);

      if (RegisterClass(&text_log_class) == 0) {
         /* Failure, window class is a basis and we do not have one. */
         ALLEGRO_ERROR("RegisterClass failed\n");
         al_unlock_mutex(textlog->tl_text_mutex);
         return false;
      }
   }

   /* Load RichEdit control. */
   if (wlog_count == 0) {
      ALLEGRO_DEBUG("Load rich edit module\n");
      ALLEGRO_ASSERT(!wlog_rich_edit_module);

      if ((wlog_rich_edit_module = _al_open_library("msftedit.dll"))) {
         /* 4.1 and emulation of 3.0, 2.0, 1.0 */
         wlog_edit_control = TEXT("RICHEDIT50W"); /*MSFTEDIT_CLASS*/
         wlog_unicode      = true;
      }
      else if ((wlog_rich_edit_module = _al_open_library("riched20.dll"))) {
         /* 3.0, 2.0 */
         wlog_edit_control = TEXT("RichEdit20W"); /*RICHEDIT_CLASS*/
         wlog_unicode      = true;
      }
      else if ((wlog_rich_edit_module = _al_open_library("riched32.dll"))) {
         /* 1.0 */
         wlog_edit_control = TEXT("RichEdit"); /*RICHEDIT_CLASS*/
         wlog_unicode      = false;
      }
      else {
         wlog_edit_control = TEXT("EDIT");
         wlog_unicode      = false;
      }
   }

   wlog_count++;
   ALLEGRO_DEBUG("wlog_count = %d\n", wlog_count);

   rc = open_native_text_log_inner(textlog);

   wlog_count--;
   ALLEGRO_DEBUG("wlog_count = %d\n", wlog_count);

   /* Release RichEdit module. */
   if (wlog_count == 0 && wlog_rich_edit_module) {
      ALLEGRO_DEBUG("Unload rich edit module\n");
      _al_close_library(wlog_rich_edit_module);
      wlog_rich_edit_module = NULL;
   }

   /* Unregister window class. */
   if (wlog_count == 0) {
      ALLEGRO_DEBUG("Unregister text log class\n");
      UnregisterClass(TEXT("Allegro Text Log"), (HINSTANCE)GetModuleHandle(NULL));
   }

   al_unlock_mutex(textlog->tl_text_mutex);

   return rc;
}

void _al_close_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   /* Just deactivate text log and send bye, bye message. */
   if (IsWindow(textlog->window)) {
      textlog->is_active = false;
      PostMessage(textlog->window, WM_CLOSE, 0, 0);
   }
}

void _al_append_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   if (textlog->tl_have_pending)
      return;
   textlog->tl_have_pending = true;

   /* Post text as user message. This guarantees that the whole processing will
    * take place on text log thread.
    */
   if (IsWindow(textlog->window)) {
      PostMessage(textlog->window, WM_USER, (WPARAM)textlog, 0);
   }
}

static bool menu_callback(ALLEGRO_DISPLAY *display, UINT msg, WPARAM wParam, LPARAM lParam,
                             LPARAM* result, void *userdata)
{
   (void) userdata;
   *result = 0;

   if (msg == WM_COMMAND && lParam == 0) {
      const int id = LOWORD(wParam);
      _AL_MENU_ID *menu_id = _al_find_parent_menu_by_id(display, id);
      if (menu_id) {
         int index;
         if (_al_find_menu_item_unique(menu_id->menu, id, NULL, &index)) {
            if (al_get_menu_item_flags(menu_id->menu, -index) & ALLEGRO_MENU_ITEM_CHECKBOX) {
               /* Toggle the checkbox, since Windows doesn't do that automatically. */
               al_toggle_menu_item_flags(menu_id->menu, -index, ALLEGRO_MENU_ITEM_CHECKED);
            }
         }
      }
      _al_emit_menu_event(display, id);
      return true;
   }
   else if (msg == WM_SYSCOMMAND) {
      if ((wParam & 0xfff0) == SC_KEYMENU && al_get_display_menu(display) != NULL) {
         /* Allow the ALT key to open the menu
          * XXX: do we even want to do this? Should it be optional?
          */
         DefWindowProc(al_get_win_window_handle(display), msg, wParam, lParam);
         return true;
      }
   }
   else if (msg == WM_SHOW_POPUP) {
      ALLEGRO_MENU *menu = (ALLEGRO_MENU *) lParam;
      HWND hwnd = al_get_win_window_handle(display);
      POINT pos;
      GetCursorPos(&pos);      
      SetForegroundWindow(hwnd);
      TrackPopupMenuEx((HMENU) menu->extra1, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, pos.x, pos.y, hwnd, NULL);

      return true;
   }
   else if (msg == WM_HIDE_MENU) {
      SetMenu(al_get_win_window_handle(display), NULL);
      return true;
   }
   else if (msg == WM_SHOW_MENU) {
      ALLEGRO_MENU *menu = (ALLEGRO_MENU *) lParam;
      SetMenu(al_get_win_window_handle(display), (HMENU) menu->extra1);
      return true;
   }
   else if (msg == WM_SIZE) {
      ALLEGRO_DEBUG("Got the WM_SIZE event.\n");
      got_wm_size_event = true;
      al_signal_cond(wm_size_cond);
   }
   else if (msg == WM_MENUSELECT) {
      /* XXX: could use this as a way to indicate the popup menu was canceled */
   }

   return false;
}

static void init_menu_info(MENUITEMINFO *info, ALLEGRO_MENU_ITEM *menu)
{
   memset(info, 0, sizeof(*info));

   info->cbSize = sizeof(*info);   
   info->fMask = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_STRING | MIIM_CHECKMARKS;
   info->wID = menu->unique_id;

   if (!menu->caption) {
      info->fType = MFT_SEPARATOR;
   }
   else {
      info->fType = MFT_STRING;
      info->dwTypeData = _twin_ustr_to_tchar(menu->caption);
      info->cch = al_ustr_size(menu->caption);
   }

   if (menu->flags & ALLEGRO_MENU_ITEM_CHECKED) {
      info->fState |= MFS_CHECKED;
   }

   if (menu->flags & ALLEGRO_MENU_ITEM_DISABLED) {
      info->fState |= MFS_DISABLED;
   }

   if (menu->icon) {
      /* convert ALLEGRO_BITMAP to HBITMAP (could be moved into a public function) */
      const int h = al_get_bitmap_height(menu->icon), w = al_get_bitmap_width(menu->icon);
      HDC hdc;
      HBITMAP hbmp;
      BITMAPINFO bi;
      uint8_t *data = NULL;
      ALLEGRO_LOCKED_REGION *lock;

      ZeroMemory(&bi, sizeof(BITMAPINFO));
      bi.bmiHeader.biSize = sizeof(BITMAPINFO);
      bi.bmiHeader.biWidth = w;
      bi.bmiHeader.biHeight = -h;
      bi.bmiHeader.biPlanes = 1;
      bi.bmiHeader.biBitCount = 32;
      bi.bmiHeader.biCompression = BI_RGB;

      hdc = GetDC(menu->parent->display ? al_get_win_window_handle(menu->parent->display) : NULL);
      
      hbmp = CreateDIBSection(hdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, (void **)&data, NULL, 0);

      lock = al_lock_bitmap(menu->icon, ALLEGRO_PIXEL_FORMAT_ARGB_8888, ALLEGRO_LOCK_READONLY);
      memcpy(data, lock->data, w * h * 4);
      al_unlock_bitmap(menu->icon);
      
      info->hbmpUnchecked = hbmp;
      menu->extra2 = hbmp;

      ReleaseDC(menu->parent->display ? al_get_win_window_handle(menu->parent->display) : NULL, hdc);
   }

   if (menu->popup) {
      info->hSubMenu = (HMENU) menu->popup->extra1;
   }

}
static void destroy_menu_info(MENUITEMINFO *info) {
   if (info->dwTypeData) {
      al_free(info->dwTypeData);
   }
}
bool _al_init_menu(ALLEGRO_MENU *menu)
{
   menu->extra1 = CreateMenu();
   return menu->extra1 != NULL;
}

bool _al_init_popup_menu(ALLEGRO_MENU *menu)
{
   menu->extra1 = CreatePopupMenu();
   return menu->extra1 != NULL;
}

bool _al_insert_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
   MENUITEMINFO info;
   init_menu_info(&info, item);
  
   InsertMenuItem((HMENU) item->parent->extra1, i, TRUE, &info);
   destroy_menu_info(&info);
   return true;
}

bool _al_update_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
   MENUITEMINFO info;

   init_menu_info(&info, item);
   SetMenuItemInfo((HMENU) item->parent->extra1, i, TRUE, &info);

   if (item->parent->display)
      DrawMenuBar(al_get_win_window_handle(item->parent->display));
   destroy_menu_info(&info);
   return true;
}

bool _al_destroy_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
   DeleteMenu((HMENU) item->parent->extra1, i, MF_BYPOSITION);

   if (item->parent->display)
      DrawMenuBar(al_get_win_window_handle(item->parent->display));

   return true;
}

bool _al_show_display_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu)
{
   HWND hwnd = al_get_win_window_handle(display);
   if (!hwnd) return false;

   ASSERT(menu->extra1);
   
   /* Note that duplicate callbacks are automatically filtered out, so it's safe
      to call this many times. */
   al_win_add_window_callback(display, menu_callback, NULL);

   got_wm_size_event = false;
   PostMessage(al_get_win_window_handle(display), WM_SHOW_MENU, 0, (LPARAM) menu);

   /* Wait for the WM_SIZE event, otherwise the compensatory
    * al_resize_display (see menu.c) won't work right. */
   if (wm_size_cond && global_mutex) {
      ALLEGRO_DEBUG("Sent WM_SHOW_MENU, waiting for WM_SIZE.\n");
      al_lock_mutex(global_mutex);
      while (!got_wm_size_event) {
         al_wait_cond(wm_size_cond, global_mutex);
      }
      al_unlock_mutex(global_mutex);
   }
   
   return true;
}

bool _al_hide_display_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu)
{
   HWND hwnd = al_get_win_window_handle(display);
   if (!hwnd) return false;

   /* Must be run from the main thread to avoid a crash. */
   got_wm_size_event = false;
   PostMessage(al_get_win_window_handle(display), WM_HIDE_MENU, 0, (LPARAM) menu);

   /* Wait for the WM_SIZE event, otherwise the compensatory
    * al_resize_display (see menu.c) won't work right. */
   if (wm_size_cond && global_mutex) {
      ALLEGRO_DEBUG("Sent WM_HIDE_MENU, waiting for WM_SIZE.\n");
      al_lock_mutex(global_mutex);
      while (!got_wm_size_event) {
         al_wait_cond(wm_size_cond, global_mutex);
      }
      al_unlock_mutex(global_mutex);
   }

   (void) menu;
   
   return true;
}

bool _al_show_popup_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu)
{
   /* The popup request must come from the main thread. */

   if (!display)
      return false;

   al_win_add_window_callback(display, menu_callback, NULL);
   PostMessage(al_get_win_window_handle(display), WM_SHOW_POPUP, 0, (LPARAM) menu);

   return true;
}

int _al_get_menu_display_height(void)
{
   return GetSystemMetrics(SM_CYMENU);
}


/* vim: set sts=3 sw=3 et: */
