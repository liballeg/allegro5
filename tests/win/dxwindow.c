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
 *      Example program showing how to use Allegro's DirectX
 *      windowed driver with a native window.
 *
 *      By Eric Botcazou.
 * 
 *      Modified by V. Karthik Kumar to not use keyboard routines.
 * 
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "winalleg.h"
#include "dxwindow.rh"
#include "examples/running.h"


#define WINDOW_WIDTH  478
#define WINDOW_HEIGHT 358
#define RADIUS         50
#define STEP            4

#define TIMER_SPEED_MAX     150
#define TIMER_SPEED_MIN      20
#define TIMER_SPEED_DEFAULT  80
#define TIMER_STEP           10
#define TIMER_SPEED_BOOST    10

#define BOOST_DURATION     1500


struct point
{
   int x, y;
   fixed theta;
   int branch;
};


HINSTANCE hInst = NULL;
DATAFILE *dat;
int timer_speed;
volatile int tick = 0;
int tick_boost = -1;
int running;

struct point p[8];
int radius;
fixed scaled_radius;  /* to keep the radial speed constant */
fixed inv_scaled_radius;



void tick_counter(void)
{
   tick++;
   if (tick_boost > 0) {
      if (tick >= tick_boost + BOOST_DURATION/TIMER_SPEED_BOOST)
         tick_boost = 0;
   }
}



void init_trajectory(int w, int h, int bw, int bh, int r)
{
   p[0].x = bw + r;
   p[0].y = h - bh;
   p[0].theta = itofix(0);
   p[0].branch = 0;

   p[1].x = w - p[0].x;
   p[1].y = p[0].y;
   p[1].theta = p[0].theta;
   p[1].branch = 1;

   p[2].x = w - bw;
   p[2].y = h - (bh + r);
   p[2].theta = itofix(-64);
   p[2].branch = 2;

   p[3].x = p[2].x;
   p[3].y = h - p[2].y;
   p[3].theta = p[2].theta;
   p[3].branch = 3;

   p[4].x = p[1].x;
   p[4].y = bh;
   p[4].theta = itofix(-128);
   p[4].branch = 4;

   p[5].x = p[0].x;
   p[5].y = bh;
   p[5].theta = p[4].theta;
   p[5].branch = 5;

   p[6].x = bw;
   p[6].y = p[3].y;
   p[6].theta = itofix(-192);
   p[6].branch = 6;

   p[7].x = p[6].x;
   p[7].y = p[2].y;
   p[7].theta = p[6].theta;
   p[7].branch = 7;

   radius = r;

   scaled_radius = (r + bh) * fixtorad_r;  /* fixed = int*fixed */
   inv_scaled_radius = fixdiv(itofix(1), scaled_radius);
}



void calculate_position(struct point *pos, int delta_s)
{
   int diff;
   fixed theta_diff;

   switch (pos->branch) {

      case 0:
         pos->x += delta_s;
         if (pos->x > p[1].x) {
            diff = pos->x - p[1].x;
            *pos = p[1];
            calculate_position(pos, diff);
         }
         break;

      case 1:
         pos->theta -= inv_scaled_radius * delta_s;
         if (pos->theta < p[2].theta) {
            theta_diff = p[2].theta - pos->theta;
            *pos = p[2];
            calculate_position(pos, fixtoi(fixmul(theta_diff, scaled_radius)));
         }
         else {
            pos->x = p[1].x - fixtoi(fixsin(pos->theta) * radius);
            pos->y = p[1].y - radius + fixtoi(fixcos(pos->theta) * radius);
         }
         break;

      case 2:
         pos->y -= delta_s;
         if (pos->y < p[3].y) {
            diff = p[3].y - pos->y;
            *pos = p[3];
            calculate_position(pos, diff);
         }
         break;

      case 3:
         pos->theta -= inv_scaled_radius * delta_s;
         if (pos->theta < p[4].theta) {
            theta_diff = p[4].theta - pos->theta;
            *pos = p[4];
            calculate_position(pos, fixtoi(fixmul(theta_diff, scaled_radius)));
         }
         else {
            pos->x = p[3].x - radius - fixtoi(fixsin(pos->theta) * radius);
            pos->y = p[3].y + fixtoi(fixcos(pos->theta) * radius);
         }
         break;

      case 4:
         pos->x -= delta_s;
         if (pos->x < p[5].x) {
            diff = p[5].x - pos->x;
            *pos = p[5];
            calculate_position(pos, diff);
         }
         break;

      case 5:
         pos->theta -= inv_scaled_radius * delta_s;
         if (pos->theta < p[6].theta) {
            theta_diff = p[6].theta - pos->theta;
            *pos = p[6];
            calculate_position(pos, fixtoi(fixmul(theta_diff, scaled_radius)));
         }
         else {
            pos->x = p[5].x - fixtoi(fixsin(pos->theta) * radius);
            pos->y = p[5].y + radius + fixtoi(fixcos(pos->theta) * radius);
         }
         break;

      case 6:
         pos->y += delta_s;
         if (pos->y > p[7].y) {
            diff = pos->y - p[7].y;
            *pos = p[7];
            calculate_position(pos, diff);
         }
         break;

      case 7:
         pos->theta -= inv_scaled_radius * delta_s;
         if (pos->theta < itofix(-256)) {  /* not p[0].theta */
            theta_diff = itofix(-256) - pos->theta;  /* not p[0].theta */
            *pos = p[0];
            calculate_position(pos, fixtoi(fixmul(theta_diff, scaled_radius)));
         }
         else {
            pos->x = p[7].x + radius - fixtoi(fixsin(pos->theta) * radius);
            pos->y = p[7].y + fixtoi(fixcos(pos->theta) * radius);
         }
         break;
   }
}



void boost(void)
{
   tick_boost = tick;
   install_int(tick_counter, TIMER_SPEED_BOOST);
   play_sample((SAMPLE *)dat[SOUND_01].dat, 128, 128, 1000, FALSE);
}



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



LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
   switch(Message) {

      case WM_COMMAND:
         switch(LOWORD(wParam)) {

            case CMD_FILE_RUN:
               if (!running) {
                  install_int(tick_counter, timer_speed);
                  running = TRUE;
               }
               break;

            case CMD_FILE_STOP:
               if (running) {
                  remove_int(tick_counter);
                  running = FALSE;
               }
               break;

            case CMD_FILE_EXIT:
               PostMessage(hwnd, WM_CLOSE, 0, 0);
               break;

            case CMD_SET_SPEED_UP:
               timer_speed -= TIMER_STEP;
               if (timer_speed < TIMER_SPEED_MIN)
                  timer_speed = TIMER_SPEED_MIN;
               install_int(tick_counter, timer_speed);
               tick_boost = -1;
               break;

            case CMD_SET_SPEED_DOWN:
               timer_speed += TIMER_STEP;
               if (timer_speed > TIMER_SPEED_MAX)
                  timer_speed = TIMER_SPEED_MAX;
               install_int(tick_counter, timer_speed);
               tick_boost = -1;
               break;

            case CMD_SET_SPEED_DEFAULT:
               timer_speed = TIMER_SPEED_DEFAULT;
               install_int(tick_counter, timer_speed);
               tick_boost = -1;
               break;

            case CMD_HELP_ABOUT:
               DialogBox(hInst, "ABOUTDLG", hwnd, AboutDlgProc);
               break;
         }
         return 0;

      case WM_LBUTTONDOWN:
         if (getpixel(screen, lParam&0xFFFF, lParam>>16) != 0)
            boost();
         return 0;

      case WM_CLOSE:
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
   static char szAppName[] = "Running Pac-Man";
   WNDCLASS wndClass;
   RECT win_rect;
   HWND hwnd;
   HACCEL haccel;
   MSG msg;
   BITMAP *video_page[2];
   int screen_w, screen_h, sprite_w, sprite_h;
   int last_tick, next_page;
   struct point pos;

   hInst = hInstance;

   if (!hPrevInstance) {
      wndClass.style         = CS_HREDRAW | CS_VREDRAW;
      wndClass.lpfnWndProc   = WndProc;
      wndClass.cbClsExtra    = 0;
      wndClass.cbWndExtra    = 0;
      wndClass.hInstance     = hInst;
      wndClass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
      wndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
      wndClass.hbrBackground = CreateSolidBrush(0);
      wndClass.lpszMenuName  = "MYMENU";
      wndClass.lpszClassName = szAppName;

      RegisterClass(&wndClass);
   }  

   hwnd = CreateWindow(szAppName, szAppName,
                       WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
                       CW_USEDEFAULT, CW_USEDEFAULT,
                       WINDOW_WIDTH, WINDOW_HEIGHT,
                       NULL, NULL,
                       hInst, NULL);

   if (!hwnd) {
      MessageBox(0, "Unable to create the window.", "Error!",
                 MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);
      return 0;
   }

   /* get the dimensions of the client area of the window */
   GetClientRect(hwnd, &win_rect);
   screen_w = win_rect.right - win_rect.left;
   screen_h = win_rect.bottom - win_rect.top;

   /* the DirectX windowed driver requires the width to be a multiple of 4 */
   screen_w &= ~3;

   /* register our window BEFORE calling allegro_init() */
   win_set_window(hwnd);

   /* initialize the library */
   if (allegro_init() != 0)
      return 0;

   install_timer();
   install_keyboard();
   install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL);

   /* install the DirectX windowed driver */
   if (set_gfx_mode(GFX_DIRECTX_WIN, screen_w, screen_h, 0, 0) != 0) {
      MessageBox(hwnd, "Unable the set the graphics mode.", "Error!",
                 MB_ICONERROR | MB_OK);
      return 0;
   }

   /* allow Allegro to keep running in the background */
   set_display_switch_mode(SWITCH_BACKGROUND);

   /* load the data */
   dat = load_datafile("../../examples/running.dat");
   if (!dat) {
      /* the CMake MSVC workspace puts the executable one directory deeper */
      dat = load_datafile("../../../examples/running.dat");
      if (!dat) {
         MessageBox(hwnd, "Unable to load ../../examples/running.dat!",
            "Error!", MB_ICONERROR | MB_OK);
         return 0;
      }
   }

   sprite_w = ((BITMAP *)dat[FRAME_01].dat)->w;
   sprite_h = ((BITMAP *)dat[FRAME_01].dat)->h;

   /* initialize the trajectory parameters */
   init_trajectory(screen_w, screen_h, sprite_w/2, sprite_h/2, RADIUS);

   /* set the 8-bit palette */
   set_palette((RGB *)dat[PALETTE_001].dat);

   /* create two pages of video memory */
   video_page[0] = create_video_bitmap(screen_w, screen_h);
   video_page[1] = create_video_bitmap(screen_w, screen_h);

   /* display the window */
   haccel = LoadAccelerators(hInst, "MYACCEL");
   ShowWindow(hwnd, nCmdShow);
   UpdateWindow(hwnd);

   /* set initial conditions */
   pos = p[0];
   next_page = 1;
   running = TRUE;
   timer_speed = TIMER_SPEED_DEFAULT;
   install_int(tick_counter, timer_speed);
   last_tick = tick;

   /* main loop */
   while (TRUE) {

      /* process the keys */
      while (keypressed()) {
         if ((readkey() >> 8) == KEY_B)
            boost();
      }

      /* animate the screen */
      clear_bitmap(video_page[next_page]);
      if (!running)
         textout_ex(video_page[next_page], font, "Paused", 10, 10, 96, -1);

      /* update the position if it has ticked */
      if (tick != last_tick) {
         calculate_position(&pos, STEP);
         last_tick = tick;
      }

      /* handle end of boost */
      if (tick_boost == 0) {
         install_int(tick_counter, timer_speed);
         tick_boost = -1;
      }

      /* draw sprite onto the backbuffer */
      rotate_sprite(video_page[next_page], (BITMAP *)dat[last_tick%10].dat,
                    pos.x - sprite_w/2, pos.y - sprite_h/2, pos.theta);

      /* flip the two pages of video memory */ 
      show_video_bitmap(video_page[next_page]);
      next_page = 1 - next_page;

      /* process the Win32 messages */
      while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
         if (GetMessage(&msg, NULL, 0, 0)) {
            if (!TranslateAccelerator(hwnd, haccel, &msg)) {
               TranslateMessage(&msg);
               DispatchMessage(&msg);
            }
         }
         else
            goto End;
      }
   }

 End:
   /* clean up ourselves */
   destroy_bitmap(video_page[0]);
   destroy_bitmap(video_page[1]);

   unload_datafile(dat);

   return msg.wParam;
}
