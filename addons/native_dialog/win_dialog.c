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
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"

#include "allegro5/platform/aintwin.h"

#include "allegro5/allegro_windows.h"

/* We use RichEdit by default. */
#include <richedit.h>

/* Non-zero if text log window class was registered. */
static int __al_class_registered = 0;

/* Handle of RichEdit module */
static HMODULE __al_rich_edit_module = 0;

/* Name of the edit control. Depend on system resources. */
static wchar_t* __al_edit_control = L"EDIT";

/* True if output support unicode */
static bool __al_unicode = false;


static int next(char *s)
{
   int i = 0;

   while  (s[i])
      i++;

   return i+1;
}

void al_show_native_file_dialog(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd)
{
   OPENFILENAME ofn;
   ALLEGRO_DISPLAY_WIN *win_display;
   int flags = 0;
   bool ret;
   char buf[4096] = { 0, };
   int i;

   memset(&ofn, 0, sizeof(OPENFILENAME));

   ofn.lStructSize = sizeof(OPENFILENAME);
   win_display = (ALLEGRO_DISPLAY_WIN *)display;
   ofn.hwndOwner = win_display->window;
   if (fd->initial_path) {
      strncpy(buf, al_path_cstr(fd->initial_path, '/'), 4096);
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
      /* FIXME: This appends a slash to a filename. */
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

      fd->paths = al_malloc(fd->count * sizeof(void *));
      i = next(buf);
      for (p = 0; p < (int)fd->count; p++) {
         fd->paths[p] = al_create_path(path);
         al_join_paths(fd->paths[p], al_create_path(buf+i));
         i += next(buf+i);
      }
   }
   else {
      fd->count = 1;
      fd->paths = al_malloc(sizeof(void *));
      fd->paths[0] = al_create_path(buf);
   }
}

int _al_show_native_message_box(ALLEGRO_DISPLAY *display,
	ALLEGRO_NATIVE_DIALOG *fd)
{
	UINT type = 0;
	int result;

	if (fd->mode & ALLEGRO_MESSAGEBOX_QUESTION) type |= MB_ICONQUESTION;
	if (fd->mode & ALLEGRO_MESSAGEBOX_WARN) type |= MB_ICONWARNING;
	if (fd->mode & ALLEGRO_MESSAGEBOX_ERROR) type |= MB_ICONERROR;
	if (fd->mode & ALLEGRO_MESSAGEBOX_YES_NO) type |= MB_YESNO;
	if (fd->mode & ALLEGRO_MESSAGEBOX_OK_CANCEL) type |= MB_OKCANCEL;

	result = MessageBox(al_get_win_window_handle(display),
		al_cstr(fd->text), al_cstr(fd->title), type);

	if (result == IDYES || result == IDOK)
		return 1;
	else
		return 0;
}


/* Emit close event. */
static void __al_emit_close_event(ALLEGRO_NATIVE_DIALOG *textlog, bool keypress)
{
   ALLEGRO_EVENT event;
   event.user.type = ALLEGRO_EVENT_NATIVE_DIALOG_CLOSE;
   event.user.timestamp = al_current_time();
   event.user.data1 = (intptr_t)textlog;
   event.user.data2 = (intptr_t)keypress;
   al_emit_user_event(&textlog->events, &event, NULL);
}

/* Output message to ANSI log. */
void __al_do_append_native_text_log_ansi(ALLEGRO_NATIVE_DIALOG *textlog)
{
   int index;
   CHARFORMATA format = { 0 };
   format.cbSize      = sizeof(format);
   format.dwMask      = CFM_COLOR;
   format.crTextColor = RGB(128, 255, 128);

   index = GetWindowTextLength(textlog->textview);
   SendMessageA(textlog->textview, EM_SETSEL, (WPARAM)index, (LPARAM)index);
   SendMessageA(textlog->textview, EM_SETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&format);
   SendMessageA(textlog->textview, EM_REPLACESEL, 0, (LPARAM)al_cstr(textlog->text));
   if (textlog->new_line) {
      SendMessageA(textlog->textview, EM_REPLACESEL, 0, (LPARAM)"\n");
   }
}

/* Output message to Unicode log. */
void __al_do_append_native_text_log_unicode(ALLEGRO_NATIVE_DIALOG *textlog)
{
#define BUFFER_SIZE  512
   bool flush;
   int index, ch, next;
   static WCHAR buffer[BUFFER_SIZE + 1] = { 0 };

   CHARFORMATW format = { 0 };
   format.cbSize      = sizeof(format);
   format.dwMask      = CFM_COLOR;
   format.crTextColor = RGB(128, 255, 128);

   index = GetWindowTextLength(textlog->textview);
   SendMessageW(textlog->textview, EM_SETSEL, (WPARAM)index, (LPARAM)index);
   SendMessageW(textlog->textview, EM_SETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&format);

   next = 0;
   index = 0;
   flush = false;
   while ((ch = al_ustr_get_next(textlog->text, &next)) >= 0) {
      buffer[index % BUFFER_SIZE] = (WCHAR)ch;
      flush = true;

      if ((next % BUFFER_SIZE) == 0) {
         buffer[BUFFER_SIZE] = L'\0';
         SendMessageW(textlog->textview, EM_REPLACESEL, 0, (LPARAM)buffer);
         flush = false;
      }

      index = next;
   }

   if (flush) {
      buffer[index % BUFFER_SIZE] = L'\0';
      SendMessageW(textlog->textview, EM_REPLACESEL, 0, (LPARAM)buffer);
   }

   if (textlog->new_line) {
      SendMessageW(textlog->textview, EM_REPLACESEL, 0, (LPARAM)"\r\n");
   }
}

/* General function to output log message. */
void __al_do_append_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   if (__al_unicode) {
      __al_do_append_native_text_log_unicode(textlog);
   }
   else {
      __al_do_append_native_text_log_ansi(textlog);
   }

   SendMessage(textlog->textview, WM_VSCROLL, SB_BOTTOM, 0);
}

/* Text log window callback. */
static LRESULT CALLBACK __al_text_log_callback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   CREATESTRUCT* create_struct;

   ALLEGRO_NATIVE_DIALOG* textlog = (ALLEGRO_NATIVE_DIALOG*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

   switch (uMsg)
   {
      case WM_CTLCOLORSTATIC:
         /* Set colors for text and background */
         SetBkColor((HDC)wParam, RGB(16, 16, 16));
         SetTextColor((HDC)wParam, RGB(128, 255, 128));
         return (LRESULT)CreateSolidBrush(RGB(16, 16, 16));

      case WM_CREATE:
         /* Set user data for window, so we will be able to retieve text log structure any time */
         create_struct = (CREATESTRUCT*)lParam;
         SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)create_struct->lpCreateParams);
         break;

      case WM_CLOSE:
         if (textlog->is_active) {
            if (!(textlog->mode & ALLEGRO_TEXTLOG_NO_CLOSE)) {
               __al_emit_close_event(textlog, false);
            }
            return 0;
         }
         break;

      case WM_DESTROY:
         PostQuitMessage(0);
         break;

      case WM_KEYDOWN:
         if (wParam == VK_ESCAPE) {
            __al_emit_close_event(textlog, true);
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
            MoveWindow(textlog->textview, client_rect.left, client_rect.top,
               client_rect.right - client_rect.left, client_rect.bottom - client_rect.top, TRUE);
         }
         break;

      case WM_USER:
         al_lock_mutex(textlog->text_mutex);

         __al_do_append_native_text_log(textlog);

         /* notify the original caller that we are all done */
         textlog->done = true;
         al_signal_cond(textlog->text_cond);
         al_unlock_mutex(textlog->text_mutex);
         break;
   }

   return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


void _al_open_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   LPCSTR font_name;
   HWND hWnd = NULL;
   HWND hLog = NULL;
   WNDCLASSA text_log_class = { 0 };
   RECT client_rect;
   HFONT hFont = NULL;
   MSG msg;
   BOOL ret;
   CHARFORMAT format = { 0 };

   al_lock_mutex(textlog->text_mutex);

   /* Prepare text log class info. */
   if (!__al_class_registered++) {

      text_log_class.hInstance      = (HINSTANCE)GetModuleHandle(NULL);
      text_log_class.lpszClassName  = "Allegro Text Log";
      text_log_class.lpfnWndProc    = __al_text_log_callback;
      text_log_class.hIcon          = NULL;
      text_log_class.hCursor        = NULL;
      text_log_class.lpszMenuName   = NULL;
      text_log_class.hbrBackground  = (HBRUSH)GetStockObject(GRAY_BRUSH);

      if (RegisterClassA(&text_log_class) == 0) {
         /* Failure, window class is a basis and we do not have one. */
         al_unlock_mutex(textlog->text_mutex);
         __al_class_registered--;
         return;
      }
   }

   /* Load RichEdit control. */
   if (!__al_rich_edit_module) {

      if (__al_rich_edit_module = LoadLibraryA("msftedit.dll")) {
         /* 4.1 and emulation of 3.0, 2.0, 1.0 */
         __al_edit_control = L"RICHEDIT50W"; /*MSFTEDIT_CLASS*/
         __al_unicode      = true;
      }
      else if (__al_rich_edit_module = LoadLibraryA("riched20.dll")) {
         /* 3.0, 2.0 */
         __al_edit_control = L"RichEdit20W"; /*RICHEDIT_CLASS*/
         __al_unicode      = true;
      }
      else if (__al_rich_edit_module = LoadLibraryA("riched32.dll")) {
         /* 1.0 */
         __al_edit_control = L"RichEdit"; /*RICHEDIT_CLASS*/
         __al_unicode      = false;
      }
      else
      {
         __al_edit_control = L"EDIT";
         __al_unicode      = false;
      }
   }


   /* Create text log window. */
   hWnd = CreateWindowA("Allegro Text Log", al_cstr(textlog->title),
      WS_CAPTION | WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
      (HINSTANCE)GetModuleHandle(NULL), textlog);
   if (!hWnd) {
      if (__al_rich_edit_module) {
         FreeLibrary(__al_rich_edit_module);
         __al_rich_edit_module = NULL;
      }
      UnregisterClassA("Allegro Text Log", (HINSTANCE)GetModuleHandle(NULL));
      al_unlock_mutex(textlog->text_mutex);
      return;
   }

   /* Get client area of the log window. */
   GetClientRect(hWnd, &client_rect);

   /* Create edit control. */
   hLog = CreateWindowW(__al_edit_control, NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL | ES_READONLY,
        client_rect.left, client_rect.top, client_rect.right - client_rect.left, client_rect.bottom - client_rect.top,
        hWnd, NULL, (HINSTANCE)GetModuleHandle(NULL), NULL);
   if (!hLog)
   {
      if (__al_rich_edit_module) {
         FreeLibrary(__al_rich_edit_module);
         __al_rich_edit_module = NULL;
      }
      DestroyWindow(hWnd);
      UnregisterClassA("Allegro Text Log", (HINSTANCE)GetModuleHandle(NULL));
      al_unlock_mutex(textlog->text_mutex);
      return;
   }

   /* Enable double-buffering. */
   SetWindowLong(hLog, GWL_EXSTYLE, GetWindowLong(hLog, GWL_EXSTYLE) | 0x02000000L/*WS_EX_COMPOSITED*/);

   /* Select font name. */
   if (textlog->mode & ALLEGRO_TEXTLOG_MONOSPACE)
      font_name = "Courier New";
   else
      font_name = "Arial";

   /* Create font and set font. */
   hFont = CreateFont(-11, 0, 0, 0, FW_LIGHT, 0, 0,
      0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
      FF_MODERN | FIXED_PITCH, font_name);

   /* Assign font to the text log. */
   if (hFont) {
      SendMessage(hLog, WM_SETFONT, (WPARAM)hFont, 0);
   }

   /* Set background color of RichEdit control. */
   if (__al_rich_edit_module) {
      SendMessage(hLog, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(16, 16, 16));
   }

   /* We are ready to show our window. */
   ShowWindow(hWnd, SW_NORMAL);

   /* Save handles for future use. */
   textlog->window    = hWnd;
   textlog->textview  = hLog;
   textlog->is_active = true;

   /* Now notify al_show_native_textlog that the text log is ready. */
   textlog->done = true;
   al_signal_cond(textlog->text_cond);
   al_unlock_mutex(textlog->text_mutex);

   /* Process messages. */
   while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0)
   {
      if (ret != -1 && msg.message != WM_QUIT)
      {
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

   /* Release RichEdit module. */
   if (__al_rich_edit_module) {
      FreeLibrary(__al_rich_edit_module);
      __al_rich_edit_module = NULL;
   }

   /* Unregister window class. */
   if (!(--__al_class_registered)) {
      UnregisterClassA("Allegro Text Log", (HINSTANCE)GetModuleHandle(NULL));
   }

   /* Notify everyone that we're gone. */
   al_lock_mutex(textlog->text_mutex);
   textlog->done = true;
   al_signal_cond(textlog->text_cond);
   al_unlock_mutex(textlog->text_mutex);
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
   /* Post text as user message. This guratantee that whole proccesing will take
    * place on text log thread. */
   if (IsWindow(textlog->window)) {
      PostMessage(textlog->window, WM_USER, (WPARAM)textlog, 0);
   }
}
