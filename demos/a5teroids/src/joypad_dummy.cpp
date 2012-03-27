#include "joypad_c.h"

/* Joypad is some iOS/MacOS X API.
 * This file is just a stub for when it isn't being used.
 * The real implementation is in joypad_handler.m.
 * Most platforms use the regular Allegro joystick API.
 */
#if !defined(USE_JOYPAD)

void joypad_start(void)
{
}

void joypad_find(void)
{
}

void joypad_stop_finding(void)
{
}

void get_joypad_state(bool *u, bool *d, bool *l, bool *r, bool *b, bool *esc)
{
    *u = *d = *l = *r = *b = *esc = false;
}

bool is_joypad_connected(void)
{
    return false;
}

#endif /* !USE_JOYPAD */

/* vim: set sts=3 sw=3 et: */
