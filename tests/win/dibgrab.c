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
 *      Example program showing how to read and write Allegro format
 *      bitmaps from a GDI device context.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "winalleg.h"



#define GRAB_W    64
#define GRAB_H    64


BITMAP *grab_region = NULL;

int captured = FALSE;

POINT location;

PALETTE pal;



LRESULT CALLBACK WndProc(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
   HWND dwnd;
   HDC hdc, ddc;
   PAINTSTRUCT ps;
   RECT rc, sz;
   OPENFILENAME fn;
   char filename[256];

   switch (message) {

      case WM_PAINT:
	 if (GetUpdateRect(hwnd, &rc, FALSE)) {
	    hdc = BeginPaint(hwnd, &ps);

	    set_palette_to_hdc(hdc, default_palette);

	    if (captured) {
	       dwnd = GetDesktopWindow();
	       ddc = GetDC(dwnd);

	       GetWindowRect(dwnd, &sz);

	       location.x = MIN(location.x, sz.right-GRAB_W);
	       location.y = MIN(location.y, sz.bottom-GRAB_W);

	       blit_from_hdc(ddc, grab_region, location.x, location.y, 0, 0, GRAB_W, GRAB_H);

	       ReleaseDC(dwnd, ddc);

	       GetClientRect(hwnd, &sz);

	       stretch_blit_to_hdc(grab_region, hdc, 0, 0, GRAB_W, GRAB_H, 0, 0, sz.right, sz.bottom);
	    }
	    else {
	       FillRect(hdc, &rc, GetStockObject(WHITE_BRUSH));
	       TextOut(hdc, 8, 8, "Click here, then drag over desktop", 34);
	    }

	    EndPaint(hwnd, &ps);
	 }
	 return 0;

      case WM_LBUTTONDOWN:
	 if (!captured) {
	    SetCapture(hwnd);
	    captured = TRUE;

	    location.x = (short)(lParam & 0xFFFF);
	    location.y = (short)(lParam >> 16);

	    ClientToScreen(hwnd, &location);

	    InvalidateRect(hwnd, NULL, FALSE);
	 }
	 return 0;

      case WM_LBUTTONUP:
	 if (captured) {
	    ReleaseCapture();
	    captured = FALSE;

	    InvalidateRect(hwnd, NULL, FALSE);

	    memset(&fn, 0, sizeof(fn));

	    fn.lStructSize = sizeof(fn);
	    fn.hwndOwner = hwnd;
	    fn.lpstrFilter = "Bitmap Files (*.bmp;*.pcx;*.tga)\0*.bmp;*.pcx;*.tga\0All Files (*.*)\0*.*\0";
	    fn.lpstrFile = filename;
	    fn.nMaxFile = sizeof(filename);
	    fn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

	    filename[0] = 0;

	    if (GetSaveFileName(&fn)) {
	       if (save_bitmap(filename, grab_region, pal) != 0)
		  MessageBox(hwnd, "Error saving bitmap file", NULL, MB_OK);
	    }
	 }
	 return 0;

      case WM_MOUSEMOVE:
	 if (captured) {
	    location.x = (short)(lParam & 0xFFFF);
	    location.y = (short)(lParam >> 16);

	    ClientToScreen(hwnd, &location);

	    InvalidateRect(hwnd, NULL, FALSE);
	 }
	 return 0;

      case WM_DESTROY:
	 PostQuitMessage(0);
	 return 0;
   }

   return DefWindowProc(hwnd, message, wParam, lParam);
}



int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
   static char szAppName[] = "Magnificator";
   WNDCLASS wndclass;
   HWND hwnd;
   MSG msg;

   if (!hPrevInstance) {
      wndclass.style = CS_HREDRAW | CS_VREDRAW;
      wndclass.lpfnWndProc = WndProc;
      wndclass.cbClsExtra = 0;
      wndclass.cbWndExtra = 0;
      wndclass.hInstance = hInstance;
      wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
      wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
      wndclass.hbrBackground = NULL;
      wndclass.lpszMenuName = NULL;
      wndclass.lpszClassName = szAppName;

      RegisterClass(&wndclass);
   }

   hwnd = CreateWindow(szAppName, szAppName,
		       WS_OVERLAPPEDWINDOW,
		       CW_USEDEFAULT, CW_USEDEFAULT,
		       256, 256,
		       NULL, NULL,
		       hInstance,
		       NULL);

   if (install_allegro(SYSTEM_NONE, &errno, atexit) != 0)
      exit(0);
   set_gdi_color_format();

   grab_region = create_bitmap_ex(32, GRAB_W, GRAB_H);

   ShowWindow(hwnd, nCmdShow);
   UpdateWindow(hwnd);

   while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   destroy_bitmap(grab_region);

   return msg.wParam;
}

