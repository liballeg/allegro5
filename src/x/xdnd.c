// Note: adapted from SDL2
#include <sys/time.h>
#include <ctype.h>

#include <X11/Xatom.h>

#include "allegro5/allegro.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xevents.h"
#include "allegro5/internal/aintern_xsystem.h"
#include "allegro5/internal/aintern_xdnd.h"

ALLEGRO_DEBUG_CHANNEL("xdnd")

typedef struct {
    unsigned char *data;
    int format, count;
    Atom type;
} Property;

static int last_drop_x, last_drop_y;

void _al_display_xglx_init_dnd_atoms(ALLEGRO_SYSTEM_XGLX *s)
{
   #define GET_ATOM(name) s->dnd_info.name = XInternAtom(s->x11display, #name, False)
   GET_ATOM(XdndEnter);
   GET_ATOM(XdndPosition);
   GET_ATOM(XdndStatus);
   GET_ATOM(XdndTypeList);
   GET_ATOM(XdndActionCopy);
   GET_ATOM(XdndDrop);
   GET_ATOM(XdndFinished);
   GET_ATOM(XdndSelection);
   GET_ATOM(XdndLeave);
   GET_ATOM(PRIMARY);
}

/* Read property
 * Must call X11_XFree on results
 */
static void read_property(Property *p, Display *disp, Window w, Atom prop)
{
   unsigned char *ret = NULL;
   Atom type;
   int fmt;
   unsigned long count;
   unsigned long bytes_left;
   int bytes_fetch = 0;

   do {
      if (ret != 0) XFree(ret);
      XGetWindowProperty(disp, w, prop, 0, bytes_fetch, false,
         AnyPropertyType, &type, &fmt, &count, &bytes_left, &ret);
      bytes_fetch += bytes_left;
   } while (bytes_left != 0);

   p->data = ret;
   p->format = fmt;
   p->count = count;
   p->type = type;
}

/* Find text-uri-list in a list of targets and return it's atom
 * if available, else return None
 */
static Atom pick_target(Display *disp, Atom list[], int list_count)
{
   Atom request = None;
   char *name;
   int i;
   for (i = 0; i < list_count && request == None; i++) {
      name = XGetAtomName(disp, list[i]);
      if ((strcmp("text/uri-list", name) == 0) || (strcmp("text/plain", name) == 0)) {
         request = list[i];
      }
      XFree(name);
   }
   return request;
}


/* Wrapper for pick_target for a maximum of three targets, a special
 * case in the Xdnd protocol
 */
static Atom pick_target_from_atoms(Display *disp, Atom a0, Atom a1, Atom a2)
{
   int count = 0;
   Atom atom[3];
   if (a0 != None) atom[count++] = a0;
   if (a1 != None) atom[count++] = a1;
   if (a2 != None) atom[count++] = a2;
   return pick_target(disp, atom, count);
}

static void x11_reply(ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY_XGLX *allegro_display,
   Window window, Atom message_type)
{
   XClientMessageEvent m;
   memset(&m, 0, sizeof(XClientMessageEvent));
   m.type = ClientMessage;
   m.display = s->x11display;
   m.window = window;
   m.message_type = message_type;
   m.format = 32;
   m.data.l[0] = allegro_display->window;
   if (message_type == s->dnd_info.XdndStatus) {
      /* See https://freedesktop.org/wiki/Specifications/XDND/
       *
       * data.l[0] contains the XID of the target window.
       * data.l[1]:
       *   Bit 0 is set if the current target will accept the drop.
       * data.l[2,3] contains a rectangle in root coordinates that
       * means "don't send another XdndPosition message until the mouse
       * moves out of here".
       * data.l[4] contains the action accepted by the target
       */
      m.data.l[1] = s->dnd_info.xdnd_req != None ? 3 : 0;
      m.data.l[4] = s->dnd_info.XdndActionCopy;
   }
   if (message_type == s->dnd_info.XdndFinished) {
      /* See https://freedesktop.org/wiki/Specifications/XDND/
       *
       * data.l[0] contains the XID of the target window.
       * data.l[1]:
       *   Bit 0 is set if the current target accepted the drop
       * data.l[2] contains the action performed by the target.
       */
      m.data.l[1] = s->dnd_info.xdnd_req != None;
      m.data.l[2] = s->dnd_info.xdnd_req != None ? s->dnd_info.XdndActionCopy : None;
   }
   XSendEvent(s->x11display, window, false, NoEventMask, (XEvent*)&m);
   XSync(s->x11display, 0);
}

static void _send_event(ALLEGRO_DISPLAY_XGLX *allegro_display, char *text,
      bool is_file, int row, bool is_complete)
{
   ALLEGRO_EVENT_SOURCE *es = &allegro_display->display.es;
   _al_event_source_lock(es);

   if (_al_event_source_needs_to_generate_event(es)) {
      ALLEGRO_EVENT event;
      event.drop.type = ALLEGRO_EVENT_DROP;
      event.drop.timestamp = al_get_time();
      event.drop.x = last_drop_x;
      event.drop.y = last_drop_y;
      event.drop.text = text;
      event.drop.is_file = is_file;
      event.drop.row = row;
      event.drop.is_complete = is_complete;
      _al_event_source_emit_event(es, &event);
   }
   _al_event_source_unlock(es);
}

/* Return true if the event is handled as a DND related event else
 * false.
 */
bool _al_display_xglx_handle_drag_and_drop(ALLEGRO_SYSTEM_XGLX *s,
   ALLEGRO_DISPLAY_XGLX *allegro_display, XEvent *xevent)
{
   if (!(allegro_display->display.flags & ALLEGRO_DRAG_AND_DROP))
      return false;
   if (xevent->xclient.message_type == s->dnd_info.XdndEnter) {
      ALLEGRO_DEBUG("Xdnd entered\n");
      bool use_list = xevent->xclient.data.l[1] & 1;
      s->dnd_info.xdnd_source = xevent->xclient.data.l[0];

      if (use_list) {
         Property p;
         read_property(&p, s->x11display, s->dnd_info.xdnd_source,
            s->dnd_info.XdndTypeList);
         s->dnd_info.xdnd_req = pick_target(s->x11display, (Atom*)p.data, p.count);
         XFree(p.data);
      } else {
         s->dnd_info.xdnd_req = pick_target_from_atoms(s->x11display,
            xevent->xclient.data.l[2], xevent->xclient.data.l[3],
            xevent->xclient.data.l[4]);
      }
      if (s->dnd_info.xdnd_req == None) {
         ALLEGRO_WARN("Xdnd action unsupported\n");
      }
      return true;
   }
   else if (xevent->xclient.message_type == s->dnd_info.XdndPosition) {
      int root_x, root_y, window_x, window_y;
      Window ChildReturn;
      root_x = xevent->xclient.data.l[2] >> 16;
      root_y = xevent->xclient.data.l[2] & 0xffff;

      XTranslateCoordinates(s->x11display, DefaultRootWindow(s->x11display),
          allegro_display->window,
            root_x, root_y, &window_x, &window_y, &ChildReturn);

      // we don't send the text yet to avoid all the string allocations
      last_drop_x = window_x;
      last_drop_y = window_y;
      _send_event(allegro_display, NULL, false, 0, false);
      x11_reply(s, allegro_display, xevent->xclient.data.l[0], s->dnd_info.XdndStatus);
      return true;
   }
   else if (xevent->xclient.message_type == s->dnd_info.XdndDrop) {
      if (s->dnd_info.xdnd_req == None) {
         x11_reply(s, allegro_display, xevent->xclient.data.l[0], s->dnd_info.XdndFinished);
         _send_event(allegro_display, NULL, false, 0, true);
         ALLEGRO_DEBUG("Xdnd aborted\n");
      } else {
         ALLEGRO_DEBUG("Xdnd received, converting to selection\n");
         int xdnd_version = xevent->xclient.data.l[1] >> 24;
         if(xdnd_version >= 1) {
            XConvertSelection(s->x11display, s->dnd_info.XdndSelection,
               s->dnd_info.xdnd_req, s->dnd_info.PRIMARY, allegro_display->window,
               xevent->xclient.data.l[2]);
         } else {
            XConvertSelection(s->x11display, s->dnd_info.XdndSelection,
               s->dnd_info.xdnd_req, s->dnd_info.PRIMARY, allegro_display->window, CurrentTime);
         }
      }
      return true;
   }
   else if (xevent->xclient.message_type == s->dnd_info.XdndLeave) {
      _send_event(allegro_display, NULL, false, 0, true);
      ALLEGRO_DEBUG("Xdnd cancelled\n");
   }
   return false;
}

// Replaces "%xx" with the hex character and "+" with a space.
static void _urldecode(char *url)
{
   char const *src = url;
   char *dst = url;
   char a, b;
   while (*src) {
      if ((*src == '%') && ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
         if (a >= 'a') a = a - 'a' + 'A';
         if (a >= 'A') a = a - 'A' + 10;
         else a -= '0';
         if (b >= 'a') b = b - 'a' + 'A';
         if (b >= 'A') b = b - 'A' + 10;
         else b -= '0';
         *dst++ = 16 * a + b;
         src += 3;
      } else if (*src == '+') {
         *dst++ = ' ';
         src++;
      } else {
         *dst++ = *src++;
      }
   }
   *dst++ = '\0';
}

// in any of these cases:
// - /path
// - file:/path
// - file:///path
// - file://hostname/path
// we will return just /path
static char *_uri_to_filename(char *uri)
{
   char *uri2 = uri;
   if (memcmp(uri, "file://", 7) == 0) {
      uri2 = strstr(uri + 7, "/");
   }
   else if (memcmp(uri, "file:/", 6) == 0) {
      uri2 += 5;
   }
   else if (strstr(uri, ":/") != NULL) {
      return NULL;
   }
   memmove(uri, uri2, strlen(uri2) + 1);
   _urldecode(uri);
   return uri;
}

bool _al_display_xglx_handle_drag_and_drop_selection(ALLEGRO_SYSTEM_XGLX *s,
   ALLEGRO_DISPLAY_XGLX *allegro_display, XEvent *xevent)
{
   if (!(allegro_display->display.flags & ALLEGRO_DRAG_AND_DROP))
      return false;
   Atom target = xevent->xselection.target;

   if (target == s->dnd_info.xdnd_req) {
      Property p;
      read_property(&p, s->x11display, allegro_display->window, s->dnd_info.PRIMARY);
      if (p.format == 8) {
         ALLEGRO_USTR *text = al_ustr_new((char *)p.data);
         char *name = XGetAtomName(s->x11display, target);
         if (name) {
            ALLEGRO_INFO("Xdnd data received, format '%s'\n", name);
            int pos = 0;
            int row = 0;
            bool complete = false;
            while (!complete) {
               int pos2 = al_ustr_find_set_cstr(text, pos, "\r\n");
               if (pos2 == -1) pos2 = al_ustr_size(text);
               ALLEGRO_USTR *token = al_ustr_dup_substr(text, pos, pos2);
               if (al_ustr_get(text, pos2) == '\r') al_ustr_next(text, &pos2);
               if (al_ustr_get(text, pos2) == '\n') al_ustr_next(text, &pos2);
               if (al_ustr_get(text, pos2) == -1) complete = true;
               pos = pos2;

               if (strcmp(name, "text/plain") == 0) {
                  _send_event(allegro_display, al_cstr_dup(token), false, row, complete);
               }
               else if (strcmp(name, "text/uri-list") == 0) {
                  char *filename = _uri_to_filename(al_cstr_dup(token));
                  _send_event(allegro_display, filename, true, row, complete);
               }
               else {
                  ALLEGRO_WARN("Attempt to drop unknown format '%s'\n", name);
               }
               al_ustr_free(token);
               row++;
            }
            XFree(name);
         }
         al_ustr_free(text);
      }
      else {
         s->dnd_info.xdnd_req = None;
      }
      if (p.data) XFree(p.data);
      x11_reply(s, allegro_display, s->dnd_info.xdnd_source,
         s->dnd_info.XdndFinished);
      ALLEGRO_DEBUG("Xdnd completed\n");
      return true;
   }
   return false;
}

void _al_xwin_accept_drag_and_drop(ALLEGRO_DISPLAY *display, bool accept)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;

   Atom XdndAware = XInternAtom(system->x11display, "XdndAware", False);

   if (accept) {
      Atom xdnd_version = 5;
      XChangeProperty(system->x11display, glx->window, XdndAware, XA_ATOM, 32,
         PropModeReplace, (unsigned char*)&xdnd_version, 1);
   } else {
      XDeleteProperty(system->x11display, glx->window, XdndAware);
   }
}
