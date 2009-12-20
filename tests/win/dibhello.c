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
 *      Example program showing how Allegro graphics can be used in
 *      Windows alongside the GDI. This method is slower than using
 *      DirectX, but good for when you want to use some of the Allegro
 *      drawing routines in a GUI program.
 *
 *      By Marian Dvorsky.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "winalleg.h"



HBITMAP hbmp;

BITMAP *a, *b;

PALETTE pal;



LRESULT CALLBACK WndProc(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
   RECT rect, updaterect;
   PAINTSTRUCT ps;
   HBITMAP oldbmp;
   HDC hdc, dc;

   switch (message) {

      case WM_QUERYNEWPALETTE:
	 InvalidateRect(hwnd, NULL, 1);
	 return 1;

      case WM_PALETTECHANGED:
	 if ((HWND)wParam != hwnd) {
	    hdc = GetDC(hwnd);
	    InvalidateRect(hwnd, NULL, 1);
	    ReleaseDC(hwnd, hdc);
	 }
	 return 1;

      case WM_PAINT:
	 if (GetUpdateRect(hwnd, &updaterect, FALSE)) {
	    hdc = BeginPaint(hwnd, &ps);

	    /* we will blit our nice bitmap first. We didn't covert it
	     * to HBITMAP so we will draw it this way:
	     */
	    set_palette_to_hdc(hdc, pal);
	    draw_to_hdc(hdc, b, 0, 0);

	    /* 24 bit image we have in HBITMAP as DDB. To draw such
	     * image, use this code: 
	     */
	    dc = CreateCompatibleDC(hdc);
	    oldbmp = SelectObject(dc, hbmp);
	    BitBlt(hdc, b->w, 0, a->w, a->h, dc, 0, 0, SRCCOPY);
	    SelectObject(dc, oldbmp);
	    DeleteDC(dc);

	    /* you can also use GDI functions as well */
	    rect.left = 640;
	    rect.top = 0;
	    rect.right = updaterect.right;
	    rect.bottom = 200;

	    FillRect(hdc, &rect, GetStockObject(WHITE_BRUSH));

	    rect.left = 0;
	    rect.top = 200;
	    rect.right = updaterect.right;
	    rect.bottom = updaterect.bottom;

	    FillRect(hdc, &rect, GetStockObject(WHITE_BRUSH));

	    TextOut(hdc, 20, 205, "This is 8-bit allegro.pcx", 25);
	    TextOut(hdc, 360, 205, "This is 24-bit Allegro memory bitmap", 36);

	    EndPaint(hwnd, &ps);
	 }
	 return 0;

      case WM_DESTROY:
	 DeleteObject(hbmp);
	 destroy_bitmap(a);
	 destroy_bitmap(b);
	 PostQuitMessage(0);
	 return 0;
   }

   return DefWindowProc(hwnd, message, wParam, lParam);
}



int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
   static char szAppName[] = "Hello";
   WNDCLASS wndclass;
   HWND hwnd;
   MSG msg;
   int x;

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

   hwnd = CreateWindow(szAppName,            /* window class name */
		       "HELLO Program",      /* window caption */
		       WS_OVERLAPPEDWINDOW,  /* window style  */
		       CW_USEDEFAULT,        /* initial x position */
		       CW_USEDEFAULT,        /* initial y position */
		       CW_USEDEFAULT,        /* initial x size */
		       CW_USEDEFAULT,        /* initial y size */
		       NULL,                 /* parent window handle */
		       NULL,                 /* window menu handle */
		       hInstance,            /* program instance handle */
		       NULL);                /* creation parameters */

   /* we have to install platform independent driver */
   if (install_allegro(SYSTEM_NONE, &errno, atexit) != 0)
      exit(0);

   set_color_conversion(COLORCONV_NONE);

   /* we can optionally set GDI color format (makes things faster) */
   set_gdi_color_format();

   /* let's try create 24 bit bitmap */
   a = create_bitmap_ex(24, 320, 200);

   /* we could load some 8 bit bitmap */
   b = load_bitmap("..\\..\\examples\\allegro.pcx", pal);
   if (!b) {
      /* the CMake MSVC workspace puts the executable one directory deeper */
      b = load_bitmap("..\\..\\..\\examples\\allegro.pcx", pal);
      if (!b) {
         MessageBox(NULL, "Can't load ..\\..\\examples\\allegro.pcx!", "Error",
            MB_OK);
         exit(0);
      }
   }

   /* draw something into a */
   clear_bitmap(a);
   textout_centre_ex(a, font, "Hello world", 160, 100, makecol24(255, 0, 0), 0);
   textout_centre_ex(a, font, "This was created using Allegro", 160, 120, makecol24(0, 255, 0), 0);

   for (x=0; x<255; x++) {
      vline(a, x+20, 10, 20, makecol24(x, 0, 0));
      vline(a, x+20, 30, 40, makecol24(0, x, 0));
      vline(a, x+20, 50, 60, makecol24(0, 0, x));
   }

   /* we will blit 24 bit bitmap as DDB */

   /* as far as we will use palette for 8 bit picture 'b', we will 
    * create this bitmap with color conversion table for palette pal
    * (which will be active when we're in 8-bit mode)
    */
   select_palette(pal);
   hbmp = convert_bitmap_to_hbitmap(a);

   ShowWindow(hwnd, nCmdShow);
   UpdateWindow(hwnd);

   while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   return msg.wParam;
}

