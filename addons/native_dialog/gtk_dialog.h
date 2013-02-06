#ifndef __al_included_gtk_dialog_h
#define __al_included_gtk_dialog_h

#define ACK_OK       ((void *)0x1111)
#define ACK_ERROR    ((void *)0x2222)
#define ACK_OPENED   ((void *)0x3333)
#define ACK_CLOSED   ((void *)0x4444)

void _al_gtk_make_transient(ALLEGRO_DISPLAY *display, GtkWidget *window);


bool _al_gtk_ensure_thread(void);


#endif
