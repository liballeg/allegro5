#include "allegro.h"
#include "allegro/internal/aintern_bitmap.h"

/* Thread local storage for graphics API state */

#if defined ALLEGRO_MINGW32

#include "winalleg.h"

/* Display parameters */
static DWORD _new_display_format_tls;
static DWORD _new_display_refresh_rate_tls;
static DWORD _new_display_flags_tls;
static DWORD _al_current_display_tls;
static DWORD _target_bitmap_tls;
/* For pushing/popping target bitmap */
static DWORD _target_bitmap_backup_tls;
/* Bitmap parameters */
static DWORD _new_bitmap_format_tls;
static DWORD _new_bitmap_flags_tls;
/* For pushing/popping bitmap parameters */
static DWORD _new_bitmap_format_backup_tls;
static DWORD _new_bitmap_flags_backup_tls;

void al_set_new_display_format(int format)
{
   TlsSetValue(_new_display_format_tls, (LPVOID)format);
}

void al_set_new_display_refresh_rate(int refresh_rate)
{
   TlsSetValue(_new_display_refresh_rate_tls, (LPVOID)refresh_rate);
}

void al_set_new_display_flags(int flags)
{
   TlsSetValue(_new_display_flags_tls, (LPVOID)flags);
}

int al_get_new_display_format(void)
{
   return (int)TlsGetValue(_new_display_format_tls);
}

int al_get_new_display_refresh_rate(void)
{
   return (int)TlsGetValue(_new_display_refresh_rate_tls);
}

int al_get_new_display_flags(void)
{
   return (int)TlsGetValue(_new_display_flags_tls);
}

/*
 * Make a display the current display. All the following Allegro commands in
 * the same thread will implicitly use this display from now on.
 */
void al_set_current_display(AL_DISPLAY *display)
{
   if (display)
      display->vt->set_current_display(display);
   TlsSetValue(_al_current_display_tls, (LPVOID)display);
}

/*
 * Get the current display.
 */
AL_DISPLAY *al_get_current_display(void)
{
   return (AL_DISPLAY *)TlsGetValue(_al_current_display_tls);
}

/*
 * Select the bitmap to which all subsequent drawing operation
 * will draw.
 */
void al_set_target_bitmap(AL_BITMAP *bitmap)
{
   TlsSetValue(_target_bitmap_tls, (LPVOID)bitmap);
   if (!(bitmap->flags & AL_MEMORY_BITMAP))
      _al_current_display->vt->set_target_bitmap(_al_current_display, bitmap);
}

/*
 * Retrieve the target for drawing operations.
 */
AL_BITMAP *al_get_target_bitmap(void)
{
   return (AL_BITMAP *)TlsGetValue(_target_bitmap_tls);
}

void _al_push_target_bitmap(void)
{
   TlsSetValue(_target_bitmap_backup_tls, TlsGetValue(_target_bitmap_tls));
}

void _al_pop_target_bitmap(void)
{
   TlsSetValue(_target_bitmap_tls, TlsGetValue(_target_bitmap_backup_tls));
   al_set_target_bitmap((AL_BITMAP *)TlsGetValue(_target_bitmap_tls));
}

void al_set_new_bitmap_format(int format)
{
   TlsSetValue(_new_bitmap_format_tls, (LPVOID)format);
}

void al_set_new_bitmap_flags(int flags)
{
   TlsSetValue(_new_bitmap_flags_tls, (LPVOID)flags);
}

int al_get_new_bitmap_format(void)
{
   return (int)TlsGetValue(_new_bitmap_format_tls);
}

int al_get_new_bitmap_flags(void)
{
   return (int)TlsGetValue(_new_bitmap_flags_tls);
}

void _al_push_bitmap_parameters()
{
	TlsSetValue(_new_bitmap_format_backup_tls, TlsGetValue(_new_bitmap_format_tls));
	TlsSetValue(_new_bitmap_flags_backup_tls, TlsGetValue(_new_bitmap_flags_tls));
}

void _al_pop_bitmap_parameters()
{
	TlsSetValue(_new_bitmap_format_tls, TlsGetValue(_new_bitmap_format_backup_tls));
	TlsSetValue(_new_bitmap_flags_tls, TlsGetValue(_new_bitmap_flags_backup_tls));
}

bool _al_tls_init(void)
{
   if ((_new_display_format_tls = TlsAlloc()) == TLS_OUT_OF_INDEXES) return false;
   if ((_new_display_refresh_rate_tls = TlsAlloc()) == TLS_OUT_OF_INDEXES) return false;
   if ((_new_display_flags_tls = TlsAlloc()) == TLS_OUT_OF_INDEXES) return false;
   if ((_al_current_display_tls = TlsAlloc()) == TLS_OUT_OF_INDEXES) return false;
   if ((_target_bitmap_tls = TlsAlloc()) == TLS_OUT_OF_INDEXES) return false;
   if ((_target_bitmap_backup_tls = TlsAlloc()) == TLS_OUT_OF_INDEXES) return false;
   if ((_new_bitmap_format_tls = TlsAlloc()) == TLS_OUT_OF_INDEXES) return false;
   if ((_new_bitmap_flags_tls = TlsAlloc()) == TLS_OUT_OF_INDEXES) return false;
   if ((_new_bitmap_format_backup_tls = TlsAlloc()) == TLS_OUT_OF_INDEXES) return false;
   if ((_new_bitmap_flags_backup_tls = TlsAlloc()) == TLS_OUT_OF_INDEXES) return false;
   return true;
}

#else

#if defined ALLEGRO_MSVC
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif

/* Display parameters */
static THREAD_LOCAL int _new_display_format_tls = 0;
static THREAD_LOCAL int _new_display_refresh_rate_tls = 0;
static THREAD_LOCAL int _new_display_flags_tls = 0;
static THREAD_LOCAL AL_DISPLAY *_al_current_display_tls;
static THREAD_LOCAL AL_BITMAP *_target_bitmap_tls;
/* For pushing/popping target bitmap */
static THREAD_LOCAL AL_BITMAP *_target_bitmap_backup_tls;
/* Bitmap parameters */
static THREAD_LOCAL int _new_bitmap_format_tls = 0;
static THREAD_LOCAL int _new_bitmap_flags_tls = 0;
/* For pushing/popping bitmap parameters */
static THREAD_LOCAL int _new_bitmap_format_backup_tls;
static THREAD_LOCAL int _new_bitmap_flags_backup_tls;

void al_set_new_display_format(int format)
{
   _new_display_format_tls = format;
}

void al_set_new_display_refresh_rate(int refresh_rate)
{
   _new_display_refresh_rate_tls = refresh_rate;
}

void al_set_new_display_flags(int flags)
{
   _new_display_flags_tls = flags;
}

int al_get_new_display_format(void)
{
   return _new_display_format_tls;
}

int al_get_new_display_refresh_rate(void)
{
   return _new_display_refresh_rate_tls;
}

int al_get_new_display_flags(void)
{
   return _new_display_flags_tls;
}

/*
 * Make a display the current display. All the following Allegro commands in
 * the same thread will implicitly use this display from now on.
 */
void al_set_current_display(AL_DISPLAY *display)
{
   if (display)
      display->vt->set_current_display(display);
   _al_current_display_tls = display;
}

/*
 * Get the current display.
 */
AL_DISPLAY *al_get_current_display(void)
{
   return _al_current_display_tls;
}

/*
 * Select the bitmap to which all subsequent drawing operation
 * will draw.
 */
void al_set_target_bitmap(AL_BITMAP *bitmap)
{
   _target_bitmap_tls = bitmap;
   if (!(bitmap->flags & AL_MEMORY_BITMAP))
      _al_current_display->vt->set_target_bitmap(_al_current_display_tls, bitmap);
}

/*
 * Retrieve the target for drawing operations.
 */
AL_BITMAP *al_get_target_bitmap(void)
{
   return _target_bitmap_tls;
}

void _al_push_target_bitmap(void)
{
   _target_bitmap_backup_tls = _target_bitmap_tls;
}

void _al_pop_target_bitmap(void)
{
   _target_bitmap_tls = _target_bitmap_backup_tls;
   al_set_target_bitmap(_target_bitmap_tls);
}

void al_set_new_bitmap_format(int format)
{
   _new_bitmap_format_tls = format;
}

void al_set_new_bitmap_flags(int flags)
{
   _new_bitmap_flags_tls = flags;
}

int al_get_new_bitmap_format(void)
{
   return _new_bitmap_format_tls;
}

int al_get_new_bitmap_flags(void)
{
   return _new_bitmap_flags_tls;
}

void _al_push_bitmap_parameters()
{
	_new_bitmap_format_backup_tls = _new_bitmap_format_tls;
	_new_bitmap_flags_backup_tls = _new_bitmap_flags_tls;
}

void _al_pop_bitmap_parameters()
{
	_new_bitmap_format_tls = _new_bitmap_format_backup_tls;
	_new_bitmap_flags_tls = _new_bitmap_flags_backup_tls;
}

#endif
