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
 *      Joystick driver routines for PSP.
 *
 *      By diedel.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"

#ifndef ALLEGRO_PSP
#error something is wrong with the makefile
#endif


static int psp_joy_init(void);
static void psp_joy_exit(void);
static int psp_joy_poll(void);


JOYSTICK_DRIVER joystick_psp = {
   JOYSTICK_PSP,         // int  id;
   empty_string,         // AL_CONST char *name;
   empty_string,         // AL_CONST char *desc;
   "PSP Controller",     // AL_CONST char *ascii_name;
   psp_joy_init,         // AL_METHOD(int, init, (void));
   psp_joy_exit,         // AL_METHOD(void, exit, (void));
   psp_joy_poll,         // AL_METHOD(int, poll, (void));
   NULL,                 // AL_METHOD(int, save_data, (void));
   NULL,                 // AL_METHOD(int, load_data, (void));
   NULL,                 // AL_METHOD(AL_CONST char *, calibrate_name, (int n));
   NULL                  // AL_METHOD(int, calibrate, (int n));
};



/* psp_joy_init:
 *  Initializes the PSP joystick driver.
 */
static int psp_joy_init(void)
{
}



/* psp_joy_exit:
 *  Shuts down the PSP joystick driver.
 */
static void psp_joy_exit(void)
{
}



/* psp_joy_poll:
 *  Polls the active joystick devices and updates internal states.
 */
static int psp_joy_poll(void)
{
}
