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
 *      Example program showing how to write a Windows screensaver. This
 *      uses a fullscreen DirectX mode when activated normally, or the
 *      GDI interface functions when it is running in preview mode from
 *      inside the Windows screensaver selection dialog.
 *
 *      Compile this like a normal executable, but rename the output
 *      program to have a .scr extension, and then copy it into your
 *      windows directory.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <time.h>

#include "allegro.h"
#include "winalleg.h"



BITMAP *buf;

int xline;
int yline;


typedef struct BOUNCER
{
   float x, y;
   float dx, dy;
} BOUNCER;


#define NUM_BOUNCERS    3

BOUNCER bouncer[NUM_BOUNCERS];



/* initialises our graphical effect */
void ss_init(void)
{
   int i;

   xline = 0;
   yline = 0;

   srand(time(NULL));

   for (i=0; i<NUM_BOUNCERS; i++) {
      bouncer[i].x = rand() % buf->w;
      bouncer[i].y = rand() % buf->h;

      bouncer[i].dx = 0.5 + (rand() & 255) / 256.0;
      bouncer[i].dy = 0.5 + (rand() & 255) / 256.0;

      if ((rand() & 255) < 128)
	 bouncer[i].dx *= -1;

      if ((rand() & 255) < 128)
	 bouncer[i].dy *= -1;
   }
}



/* animates the graphical effect */
void ss_update(void)
{
   int i;

   xline++;
   if (xline > buf->w)
      xline = 0;

   yline++;
   if (yline > buf->h)
      yline = 0;

   for (i=0; i<NUM_BOUNCERS; i++) {
      bouncer[i].x += bouncer[i].dx;
      bouncer[i].y += bouncer[i].dy;

      if ((bouncer[i].x < 0) || (bouncer[i].x > buf->w))
	 bouncer[i].dx *= -1;

      if ((bouncer[i].y < 0) || (bouncer[i].y > buf->h))
	 bouncer[i].dy *= -1;
   }
}



/* draws the graphical effect */
void ss_draw(void)
{
   clear_bitmap(buf);

   line(buf, 0, 0, buf->w, buf->h, makecol(0, 0, 255));
   line(buf, buf->w, 0, 0, buf->h, makecol(0, 0, 255));

   vline(buf, xline, 0, buf->h, makecol(255, 0, 0));
   hline(buf, 0, yline, buf->w, makecol(0, 255, 0));

   text_mode(-1);

   textout_centre(buf, font, "Allegro", bouncer[0].x, bouncer[0].y, makecol(0, 128, 255));
   textout_centre(buf, font, "Screensaver", bouncer[1].x, bouncer[1].y, makecol(255, 128, 0));
   textout_centre(buf, font, "<insert cool effect here>", bouncer[2].x, bouncer[2].y, makecol(255, 255, 255));
}



/* shuts down the graphical effect */
void ss_exit(void)
{
}



/* dialog procedure for the settings dialog */
BOOL CALLBACK settings_dlg_proc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg) {

      case WM_COMMAND: 
	 switch (HIWORD(wParam)) { 

	    case BN_CLICKED:
	       EndDialog(hwndDlg, 0);
	       return 1;
	 }
	 break;
   }

   return 0;
}



/* the settings dialog function */
int do_settings(HANDLE hInstance, HANDLE hPrevInstance, HWND hParentWnd)
{
   DialogBox(hInstance, "ID_CONFIG_DLG", hParentWnd, settings_dlg_proc);

   return 0;
}



/* the password dialog function */
int do_password(HANDLE hInstance, HANDLE hPrevInstance, HWND hParentWnd)
{
   MessageBox(hParentWnd, "Sorry, this screensaver doesn't implement the password stuff", "Allegro Screensaver", MB_OK);

   return 0;
}



/* window procedure for the screensaver preview */
LRESULT CALLBACK preview_wnd_proc(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
   PAINTSTRUCT ps;
   HDC hdc;

   switch (message) {

      case WM_CREATE:
	 SetTimer(hwnd, 1, 15, NULL);
	 return 0;

      case WM_TIMER:
	 ss_update();
	 InvalidateRect(hwnd, NULL, FALSE);
	 return 0;

      case WM_PAINT:
	 hdc = BeginPaint(hwnd, &ps);
	 ss_draw();
	 set_palette_to_hdc(hdc, _current_palette);
	 draw_to_hdc(hdc, buf, 0, 0);
	 EndPaint(hwnd, &ps);
	 return 0;

      case WM_DESTROY:
	 KillTimer(hwnd, 1);
	 PostQuitMessage(0);
	 return 0;
   }

   return DefWindowProc(hwnd, message, wParam, lParam);
}



/* the screensaver preview function */
int do_preview(HANDLE hInstance, HANDLE hPrevInstance, HWND hParentWnd)
{
   WNDCLASS wndclass;
   HWND hwnd;
   MSG msg;
   RECT rc;

   if (!hPrevInstance) {
      wndclass.style = CS_HREDRAW | CS_VREDRAW;
      wndclass.lpfnWndProc = preview_wnd_proc;
      wndclass.cbClsExtra = 0;
      wndclass.cbWndExtra = 0;
      wndclass.hInstance = hInstance;
      wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
      wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
      wndclass.hbrBackground = NULL;
      wndclass.lpszMenuName = NULL;
      wndclass.lpszClassName = "sspreview";

      RegisterClass(&wndclass);
   }

   if (hParentWnd)
      GetClientRect(hParentWnd, &rc);
   else
      rc.right = rc.bottom = 256;

   install_allegro(SYSTEM_NONE, &errno, atexit);
   set_palette(default_palette);
   set_gdi_color_format();

   buf = create_bitmap(rc.right, rc.bottom);

   ss_init();
   ss_update();

   hwnd = CreateWindow("sspreview", NULL, WS_CHILD, 
		       0, 0, rc.right, rc.bottom, 
		       hParentWnd, NULL, hInstance, NULL);

   ShowWindow(hwnd, SW_SHOW);
   UpdateWindow(hwnd);

   while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   ss_exit();
   destroy_bitmap(buf);

   return msg.wParam;
}



/* display switch callback */
static int foreground = TRUE;

static void dispsw_callback(void)
{
   foreground = FALSE;
}


/* run the saver normally, in fullscreen mode */
int do_saver(HANDLE hInstance, HANDLE hPrevInstance, HWND hParentWnd)
{
   HANDLE scrsaver_mutex;
   int mx, my, t;

   /* prevent multiple instances from running */
   scrsaver_mutex = CreateMutex(NULL, TRUE, "Allegro screensaver");

   if (!scrsaver_mutex || (GetLastError() == ERROR_ALREADY_EXISTS))
      return -1;

   allegro_init();
   install_keyboard();
   install_mouse();
   install_timer();
   
   /* try to set a fullscreen mode */
   if (set_gfx_mode(GFX_DIRECTX_ACCEL, 640, 480, 0, 0) != 0)
      if (set_gfx_mode(GFX_DIRECTX_SOFT, 640, 480, 0, 0) != 0)
         if (set_gfx_mode(GFX_DIRECTX_SAFE, 640, 480, 0, 0) != 0) {
            ReleaseMutex(scrsaver_mutex);
            return -1;
         }

   set_display_switch_mode(SWITCH_BACKAMNESIA);  /* not SWITCH_AMNESIA */
   set_display_switch_callback(SWITCH_OUT, dispsw_callback);

   buf = create_bitmap(SCREEN_W, SCREEN_H);

   ss_init();
   ss_update();

   mx = mouse_x;
   my = mouse_y;

   t = retrace_count;

   while (foreground && (!keypressed()) && (!mouse_b) && (mouse_x == mx) && (mouse_y == my)) {
      while (t < retrace_count) {
	 ss_update();
	 t++;
      }

      ss_draw();
      blit(buf, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

      poll_mouse();
   }

   ss_exit();
   destroy_bitmap(buf);

   ReleaseMutex(scrsaver_mutex);
   return 0;
}



/* the main program body */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
   HWND hwnd;
   char *args;

   args = lpszCmdParam;

   if ((args[0] == '-') || (args[0] == '/'))
      args++;

   if ((args[0]) && ((args[1] == ' ') || (args[1] == ':')))
      hwnd = (HWND)atoi(args+2);
   else
      hwnd = GetActiveWindow();

   switch (utolower(args[0])) {

      case 'c':
	 return do_settings(hInstance, hPrevInstance, hwnd);

      case 'a':
	 return do_password(hInstance, hPrevInstance, hwnd);

      case 'p':
	 return do_preview(hInstance, hPrevInstance, hwnd);

      case 's':
	 return do_saver(hInstance, hPrevInstance, hwnd);
   }

   return 0;
}
