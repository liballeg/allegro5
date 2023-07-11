#ifndef __al_included_allegro5_aintern_xdnd_h
#define __al_included_allegro5_aintern_xdnd_h

typedef struct {
   Atom XdndEnter;
   Atom XdndPosition;
   Atom XdndStatus;
   Atom XdndTypeList;
   Atom XdndActionCopy;
   Atom XdndDrop;
   Atom XdndFinished;
   Atom XdndSelection;
   Atom XdndLeave;
   Atom PRIMARY;

   Atom xdnd_req;
   Window xdnd_source;
} DndInfo;

void _al_display_xglx_init_dnd_atoms(ALLEGRO_SYSTEM_XGLX *s);
void _al_xwin_accept_drag_and_drop(ALLEGRO_DISPLAY *display, bool accept);
bool _al_display_xglx_handle_drag_and_drop(ALLEGRO_SYSTEM_XGLX *s,
   ALLEGRO_DISPLAY_XGLX *allegro_display, XEvent *event);
bool _al_display_xglx_handle_drag_and_drop_selection(ALLEGRO_SYSTEM_XGLX *s,
   ALLEGRO_DISPLAY_XGLX *allegro_display, XEvent *xevent);

#endif

/* vim: set sts=3 sw=3 et: */
