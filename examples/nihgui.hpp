#ifndef __included_nihgui_hpp
#define __included_nihgui_hpp

#include <list>
#include <string>
#include <vector>

#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"

class Theme;
class Dialog;
class Widget;

class Theme {
public:
   ALLEGRO_COLOR        bg;
   ALLEGRO_COLOR        fg;
   ALLEGRO_COLOR        highlight;
   const ALLEGRO_FONT   *font;

   // Null font is fine if you don't use a widget that requires text.
   explicit Theme(const ALLEGRO_FONT *font=NULL);
};

class Widget {
private:
   int      grid_x;
   int      grid_y;
   int      grid_w;
   int      grid_h;

protected:
   Dialog   *dialog;
   int      x1;
   int      y1;
   int      x2;
   int      y2;

public:
   Widget();
   virtual ~Widget() {}

   void           configure(int xsize, int ysize,
                     int x_padding, int y_padding);
   virtual bool   contains(int x, int y);
   unsigned int   width()  { return x2 - x1 + 1; }
   unsigned int   height() { return y2 - y1 + 1; }

   virtual bool   want_mouse_focus() { return true; }
   virtual void   got_mouse_focus() {}
   virtual void   lost_mouse_focus() {}
   virtual void   on_mouse_button_down(int mx, int my) { (void)mx; (void)my; }
   virtual void   on_mouse_button_hold(int mx, int my) { (void)mx; (void)my; }
   virtual void   on_mouse_button_up(int mx, int my) { (void)mx; (void)my; }
   virtual void   on_click(int mx, int my) { (void)mx; (void)my; }

   virtual bool   want_key_focus() { return false; }
   virtual void   got_key_focus() {}
   virtual void   lost_key_focus() {}
   virtual void   on_key_down(const ALLEGRO_KEYBOARD_EVENT & event) {
                     (void)event; }

   virtual void   draw() = 0;

   friend class Dialog;
};

class EventHandler {
public:
   virtual ~EventHandler() {}
   virtual void   handle_event(const ALLEGRO_EVENT & event) = 0;
};

class Dialog {
private:
   const Theme &        theme;
   ALLEGRO_DISPLAY *    display;
   ALLEGRO_EVENT_QUEUE *event_queue;
   int                  grid_m;
   int                  grid_n;
   int                  x_padding;
   int                  y_padding;

   bool                 draw_requested;
   bool                 quit_requested;
   std::list<Widget *>  all_widgets;
   Widget *             mouse_over_widget;
   Widget *             mouse_down_widget;
   Widget *             key_widget;

   EventHandler *       event_handler;

public:
   Dialog(const Theme & theme, ALLEGRO_DISPLAY *display,
      int grid_m, int grid_n);
   ~Dialog();

   void           set_padding(int x_padding, int y_padding);
   void           add(Widget & widget, int grid_x, int grid_y,
                     int grid_w, int grid_h);
   void           prepare();
   void           run_step(bool block);
   void           request_quit();
   bool           is_quit_requested() const;
   void           request_draw();
   bool           is_draw_requested() const;
   void           draw();
   const Theme &  get_theme() const;

   void           register_event_source(ALLEGRO_EVENT_SOURCE *source);
   void           set_event_handler(EventHandler *handler);

private:
   void           configure_all();
   void           on_key_down(const ALLEGRO_KEYBOARD_EVENT & event);
   void           on_mouse_axes(const ALLEGRO_MOUSE_EVENT & event);
   void           check_mouse_over(int mx, int my);
   void           on_mouse_button_down(const ALLEGRO_MOUSE_EVENT & event);
   void           on_mouse_button_up(const ALLEGRO_MOUSE_EVENT & event);
};

/*---------------------------------------------------------------------------*/

class Label : public Widget {
private:
   std::string    text;
   bool           centred;

public:
   Label(std::string text="", bool centred=true);
   void set_text(std::string text);
   virtual void   draw();
   virtual bool   want_mouse_focus();
};

class Button : public Widget {
protected:
   std::string    text;
   bool           pushed;

public:
   explicit Button(std::string text);
   virtual void   on_mouse_button_down(int mx, int my);
   virtual void   on_mouse_button_up(int mx, int my);
   virtual void   draw();

   bool           get_pushed();
};

class ToggleButton : public Button {
public:
   explicit ToggleButton(std::string text);
   virtual void   on_mouse_button_down(int mx, int my);
   virtual void   on_mouse_button_up(int mx, int my);

   void           set_pushed(bool pushed);
};

class List : public Widget {
private:
   static const   std::string empty_string;

   std::vector<std::string> items;
   unsigned int   selected_item;

public:
   List(int initial_selection = 0);
   virtual bool   want_key_focus();
   virtual void   on_key_down(const ALLEGRO_KEYBOARD_EVENT & event);
   virtual void   on_click(int mx, int my);
   virtual void   draw();

   void           clear_items();
   void           append_item(std::string text);
   const std::string & get_selected_item_text() const;
   int            get_cur_value() const;
};

class VSlider : public Widget {
private:
   int   cur_value;
   int   max_value;

public:
   VSlider(int cur_value = 0, int max_value = 1);

   virtual void   on_mouse_button_down(int mx, int my);
   virtual void   on_mouse_button_hold(int mx, int my);
   virtual void   draw();

   int            get_cur_value() const;
   void           set_cur_value(int v);
};

class HSlider : public Widget {
private:
   int   cur_value;
   int   max_value;

public:
   HSlider(int cur_value = 0, int max_value = 1);

   virtual void   on_mouse_button_down(int mx, int my);
   virtual void   on_mouse_button_hold(int mx, int my);
   virtual void   draw();

   int            get_cur_value() const;
   void           set_cur_value(int v);
};

class TextEntry : public Widget {
private:
   static const int CURSOR_WIDTH = 8;

   ALLEGRO_USTR   *text;
   bool           focused;
   int            cursor_pos;
   int            left_pos;

public:
   explicit TextEntry(const char *initial_text="");
   ~TextEntry();

   virtual bool   want_key_focus();
   virtual void   got_key_focus();
   virtual void   lost_key_focus();
   virtual void   on_key_down(const ALLEGRO_KEYBOARD_EVENT & event);
   virtual void   draw();

   const char *   get_text();

private:
   void maybe_scroll();
};

#endif

/* vim: set sts=3 sw=3 et: */
