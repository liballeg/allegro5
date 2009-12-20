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
 *      Example program showing how to use Allegro as a sound
 *      library under Windows.
 *
 *      By Eric Botcazou.
 * 
 *      Original idea and improvements by Javier Gonzalez.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "winalleg.h"
#include "dibsound.rh" 


HINSTANCE hInst = NULL;
PALETTE pal;
BITMAP *bmp = NULL;



BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
   switch(Message) {

      case WM_INITDIALOG:
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wParam)) {

            case IDOK:
               EndDialog(hwnd, IDOK);
            return TRUE;

            case IDCANCEL:
               EndDialog(hwnd, IDCANCEL);
            return TRUE;
         }
         break;
   }

   return FALSE;
}



int OpenNewSample(SAMPLE **sample, HWND hwnd)
{
   OPENFILENAME openfilename;
   SAMPLE *new_sample;
   char filename[512];

   memset(&openfilename, 0, sizeof(OPENFILENAME));
   openfilename.lStructSize = sizeof(OPENFILENAME);
   openfilename.hwndOwner = hwnd;
   openfilename.lpstrFilter = "Sounds (*.wav;*.voc)\0*.wav;*.voc\0";
   openfilename.lpstrFile = filename;
   openfilename.nMaxFile = sizeof(filename);
   openfilename.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

   filename[0] = 0;

   if (GetOpenFileName(&openfilename)) {
      new_sample = load_sample(openfilename.lpstrFile);

      if (!new_sample) {
         MessageBox(hwnd, "This is not a standard sound file.", "Error!",
                    MB_ICONERROR | MB_OK);
         return -1;
      }

      /* make room for next samples */
      if (*sample)
         destroy_sample(*sample);

      *sample = new_sample;
      return 0;
   }

   return -1;
}



LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
   static SAMPLE *sample = NULL;
   static int volume = 128;
   static int pan = 128;
   static int speed = 1000;
   static int loop = FALSE;
   HDC hdc;
   RECT updaterect;
   PAINTSTRUCT ps;

   switch(Message) {

      case WM_COMMAND:
         switch(LOWORD(wParam)) {

            case CMD_FILE_OPEN:
               OpenNewSample(&sample, hwnd);
               break;

            case CMD_FILE_PLAY:
               if (!sample) {
                  if (OpenNewSample(&sample, hwnd) != 0)
                     break;
               }

               /* play it !! */
               play_sample(sample, volume, pan, speed, loop);
               break;

            case CMD_FILE_STOP:
               if (sample)
                  stop_sample(sample);
               break;

            case CMD_FILE_EXIT:
               PostMessage(hwnd, WM_CLOSE, 0, 0);
               break;

            case CMD_SET_VOLUME_UP:
               volume = MIN(volume+32, 255);
               if (sample)
                  adjust_sample(sample, volume, pan, speed, loop);
               break;

            case CMD_SET_VOLUME_DOWN:
               volume = MAX(volume-32, 0);
               if (sample)
                  adjust_sample(sample, volume, pan, speed, loop);
               break;

            case CMD_SET_VOLUME_DEFAULT:
               volume = 128;
               if (sample)
                  adjust_sample(sample, volume, pan, speed, loop);
               break;

            case CMD_SET_PAN_LEFT:
               pan = MAX(pan-32, 0);
               if (sample)
                  adjust_sample(sample, volume, pan, speed, loop);
               break;

            case CMD_SET_PAN_RIGHT:
               pan = MIN(pan+32, 255);
               if (sample)
                  adjust_sample(sample, volume, pan, speed, loop);
               break;

            case CMD_SET_PAN_CENTER:
               pan = 128;
               if (sample)
                  adjust_sample(sample, volume, pan, speed, loop);
               break;

            case CMD_SET_SPEED_UP:
               speed = MIN(speed*2, 8000);
               if (sample)
                  adjust_sample(sample, volume, pan, speed, loop);
               break;

            case CMD_SET_SPEED_DOWN:
               speed = MAX(speed/2, 125);
               if (sample)
                  adjust_sample(sample, volume, pan, speed, loop);
               break;

            case CMD_SET_SPEED_NORMAL:
               speed = 1000;
               if (sample)
                  adjust_sample(sample, volume, pan, speed, loop);
               break;

            case CMD_SET_LOOP_MODE:
               loop = !loop;
               CheckMenuItem(GetMenu(hwnd), CMD_SET_LOOP_MODE, loop ? MF_CHECKED : MF_UNCHECKED);
               if (sample)
                  adjust_sample(sample, volume, pan, speed, loop);
               break;

            case CMD_HELP_ABOUT:
               DialogBox(hInst, "ABOUTDLG", hwnd, AboutDlgProc);
               break;
         }
         return 0;

      case WM_QUERYNEWPALETTE:
	 InvalidateRect(hwnd, NULL, TRUE);
	 return TRUE;

      case WM_PALETTECHANGED:
	 if ((HWND)wParam != hwnd) {
	    hdc = GetDC(hwnd);
	    InvalidateRect(hwnd, NULL, TRUE);
	    ReleaseDC(hwnd, hdc);
	 }
	 return TRUE;

      case WM_PAINT:
         if (GetUpdateRect(hwnd, &updaterect, FALSE)) {
            hdc = BeginPaint(hwnd, &ps);
            set_palette_to_hdc(hdc, pal);
            draw_to_hdc(hdc, bmp, 0, 0);
            EndPaint(hwnd, &ps);
         }
         return 0;

      case WM_CLOSE:
         destroy_sample(sample);
         DestroyWindow(hwnd);
         return 0;

      case WM_DESTROY:
         PostQuitMessage(0);
         return 0;
   }

   return DefWindowProc(hwnd, Message, wParam, lParam);
}



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   static char szAppName[] = "Sound Player";
   WNDCLASS wndClass;
   HWND hwnd;
   HACCEL haccel;
   MSG msg;

   hInst = hInstance;

   if (!hPrevInstance) {
      wndClass.style         = CS_HREDRAW | CS_VREDRAW;
      wndClass.lpfnWndProc   = WndProc;
      wndClass.cbClsExtra    = 0;
      wndClass.cbWndExtra    = 0;
      wndClass.hInstance     = hInst;
      wndClass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
      wndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
      wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
      wndClass.lpszMenuName  = "MYMENU";
      wndClass.lpszClassName = szAppName;

      RegisterClass(&wndClass);
   }  

   hwnd = CreateWindow(szAppName, szAppName,
                       WS_OVERLAPPEDWINDOW,
                       CW_USEDEFAULT, CW_USEDEFAULT,
                       320, 240,
                       NULL, NULL,
                       hInst, NULL);

   if (!hwnd) {
      MessageBox(0, "Window Creation Failed.", "Error!",
                 MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);
      return 0;
   }

   /* register our window BEFORE calling allegro_init() */
   win_set_window(hwnd);

   /* initialize the library */
   if (allegro_init() != 0)
      return 0;

   /* install the digital sound module */
   install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL);

   /* allow Allegro to keep playing samples in the background */
   set_display_switch_mode(SWITCH_BACKGROUND);

   /* load some 8 bit bitmap */
   set_color_conversion(COLORCONV_NONE);
   bmp = load_bitmap("..\\..\\examples\\allegro.pcx", pal);
   if (!bmp) {
      /* the CMake MSVC workspace puts the executable one directory deeper */
      bmp = load_bitmap("..\\..\\..\\examples\\allegro.pcx", pal);
      if (!bmp) {
         MessageBox(hwnd, "Can't load ..\\..\\examples\\allegro.pcx", "Error!",
                    MB_ICONERROR | MB_OK);
         return 0;
      }
   }

   /* display the window */
   haccel = LoadAccelerators(hInst, "MYACCEL");
   ShowWindow(hwnd, nCmdShow);
   UpdateWindow(hwnd);

   /* process messages */
   while(GetMessage(&msg, NULL, 0, 0)) {
      if (!TranslateAccelerator(hwnd, haccel, &msg)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   return msg.wParam;
}
