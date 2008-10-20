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
 *      PSP mini-keyboard driver using the PSP controller.
 *      TODO: Maybe implement it using the OSK.
 *
 *      By diedel.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include <pspctrl.h>

#ifndef ALLEGRO_PSP
#error Something is wrong with the makefile
#endif

#define PREFIX_I                "al-pkey INFO: "
#define PREFIX_W                "al-pkey WARNING: "
#define PREFIX_E                "al-pkey ERROR: "

#define SAMPLING_CYCLE 0
#define SAMPLING_MODE  PSP_CTRL_MODE_DIGITAL


static int psp_keyboard_init(void);
static void psp_keyboard_exit(void);
static void psp_poll_keyboard(void);

/*
 * Lookup table for converting PSP_CTRL_* codes into Allegro KEY_* codes
 * TODO: Choose an alternative mapping?
 */
static const int psp_to_scancode[][2] = {
   { PSP_CTRL_SELECT,     KEY_ESC   },
   { PSP_CTRL_START,      KEY_ENTER },
   { PSP_CTRL_UP,         KEY_UP    },
   { PSP_CTRL_RIGHT,      KEY_RIGHT },
   { PSP_CTRL_DOWN,       KEY_DOWN  },
   { PSP_CTRL_LEFT,       KEY_LEFT  },
   { PSP_CTRL_TRIANGLE,   KEY_SPACE },
   { PSP_CTRL_CIRCLE,     KEY_SPACE },
   { PSP_CTRL_CROSS,      KEY_SPACE },
   { PSP_CTRL_SQUARE,     KEY_SPACE }
};

#define NKEYS (sizeof psp_to_scancode / sizeof psp_to_scancode[0])


/* The last polled input. */
static SceCtrlData old_pad = {0, 0, 0, 0};


KEYBOARD_DRIVER keybd_simulator_psp =
{
   KEYSIM_PSP,
   empty_string,
   empty_string,
   "PSP keyboard simulator",
   FALSE,  // int autorepeat;
   psp_keyboard_init,
   psp_keyboard_exit,
   psp_poll_keyboard,
   NULL,   // AL_METHOD(void, set_leds, (int leds));
   NULL,   // AL_METHOD(void, set_rate, (int delay, int rate));
   NULL,   // AL_METHOD(void, wait_for_input, (void));
   NULL,   // AL_METHOD(void, stop_waiting_for_input, (void));
   NULL,   // AL_METHOD(int,  scancode_to_ascii, (int scancode));
   NULL    // scancode_to_name
};



/* psp_keyboard_init:
 *  Installs the keyboard handler.
 */
static int psp_keyboard_init(void)
{
   sceCtrlSetSamplingCycle(SAMPLING_CYCLE);
   sceCtrlSetSamplingMode(SAMPLING_MODE);
   TRACE(PREFIX_I "PSP keyboard installed\n");

   /* TODO: Maybe write a keyboard "interrupt" handler using a dedicated thread
    * that polls the PSP controller periodically. */

   return 0;
}


/* psp_keyboard_exit:
 *  Removes the keyboard handler.
 */
static void psp_keyboard_exit(void)
{
}


/* psp_poll_keyboard:
 *  Polls the PSP "mini-keyboard".
 */
static void psp_poll_keyboard(void)
{
   SceCtrlData pad;
   int buffers_to_read = 1;
   int i, changed;

   sceCtrlPeekBufferPositive(&pad, buffers_to_read);

   for (i = 0; i < NKEYS; i++) {
      changed = (pad.Buttons ^ old_pad.Buttons) & psp_to_scancode[i][0];
      if (changed) {
         if (pad.Buttons & psp_to_scancode[i][0]) {
            TRACE(PREFIX_I "PSP Keyboard: [%d] pressed\n", psp_to_scancode[i][1]);
            _handle_key_press(0, psp_to_scancode[i][1]);
         }
         else {
            TRACE(PREFIX_I "PSP Keyboard: [%d] released\n", psp_to_scancode[i][1]);
            _handle_key_release(psp_to_scancode[i][1]);
         }
      }
   }

   old_pad = pad;
}
