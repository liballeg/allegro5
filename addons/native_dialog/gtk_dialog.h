#ifndef __al_included_gtk_dialog_h
#define __al_included_gtk_dialog_h

#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"

#define ACK_OK       ((void *)0x1111)
#define ACK_ERROR    ((void *)0x2222)
#define ACK_OPENED   ((void *)0x3333)
#define ACK_CLOSED   ((void *)0x4444)

void _al_gtk_make_transient(A5O_DISPLAY *display, GtkWidget *window);


bool _al_gtk_ensure_thread(void);


/* The API is assumed to be synchronous, but the user calls will not be
 * on the GTK thread. The following structure is used to pass data from the
 * user thread to the GTK thread, and then wait until the GTK has processed it.
 */
typedef struct ARGS_BASE ARGS_BASE;

struct ARGS_BASE
{
   A5O_MUTEX *mutex;
   A5O_COND *cond;
   bool done;
   bool response;
};


bool _al_gtk_init_args(void *ptr, size_t size);
bool _al_gtk_wait_for_args(GSourceFunc func, void *data);
void *_al_gtk_lock_args(gpointer data);
gboolean _al_gtk_release_args(gpointer data);

#endif
