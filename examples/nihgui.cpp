/*
 * This is a GUI for example programs that require GUI-style interaction.
 * It's intended to be as simple and transparent as possible (simplistic,
 * even).
 */

#include <iostream>
#include <string>
#include <algorithm>
#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include <allegro5/allegro_primitives.h>

#include "nihgui.hpp"

#define CLAMP(x,y,z) (std::max)(x, (std::min)(y, z))

namespace {
   
class SaveState
{
   ALLEGRO_STATE state;

public:
   SaveState(int save=ALLEGRO_STATE_ALL)
   {
      al_store_state(&state, save);
   }
   ~SaveState()
   {
      al_restore_state(&state);
   }
};

class UString
{
   ALLEGRO_USTR_INFO info;
   const ALLEGRO_USTR *ustr;

public:
   UString(const ALLEGRO_USTR *s, int first, int end = -1)
   {
      if (end == -1)
         end = al_ustr_size(s);
      ustr = al_ref_ustr(&info, s, first, end);
   }

   // Conversion
   operator const ALLEGRO_USTR *() const
   {
      return ustr;
   }
};

};

/*---------------------------------------------------------------------------*/

Theme::Theme(const ALLEGRO_FONT *font)
{
   this->bg = al_map_rgb(255, 255, 255);
   this->fg = al_map_rgb(0, 0, 0);
   this->highlight = al_map_rgb(128, 128, 255);
   this->font = font;
}

/*---------------------------------------------------------------------------*/

Widget::Widget():
   grid_x(0),
   grid_y(0),
   grid_w(0),
   grid_h(0),
   dialog(NULL),
   x1(0),
   y1(0),
   x2(0),
   y2(0)
{
}

void Widget::configure(int xsize, int ysize, int x_padding, int y_padding)
{
   this->x1 = xsize * this->grid_x + x_padding;
   this->y1 = ysize * this->grid_y + y_padding;
   this->x2 = xsize * (this->grid_x + this->grid_w) - x_padding - 1;
   this->y2 = ysize * (this->grid_y + this->grid_h) - y_padding - 1;
}

bool Widget::contains(int x, int y)
{
   return (x >= this->x1 && y >= this->y1 && x <= this->x2 && y <= this->y2);
}

/*---------------------------------------------------------------------------*/

Dialog::Dialog(const Theme & theme, ALLEGRO_DISPLAY *display,
      int grid_m, int grid_n):
   theme(theme),
   display(display),
   grid_m(grid_m),
   grid_n(grid_n),
   x_padding(1),
   y_padding(1),

   draw_requested(true),
   quit_requested(false),
   mouse_over_widget(NULL),
   mouse_down_widget(NULL),
   key_widget(NULL),

   event_handler(NULL)
{
   this->event_queue = al_create_event_queue();
   al_register_event_source(this->event_queue, al_get_keyboard_event_source());
   al_register_event_source(this->event_queue, al_get_mouse_event_source());
   al_register_event_source(this->event_queue, al_get_display_event_source(display));
}

Dialog::~Dialog()
{
   this->display = NULL;

   al_destroy_event_queue(this->event_queue);
   this->event_queue = NULL;
}

void Dialog::set_padding(int x_padding, int y_padding)
{
   this->x_padding = x_padding;
   this->y_padding = y_padding;
}

void Dialog::add(Widget & widget, int grid_x, int grid_y, int grid_w,
   int grid_h)
{
   widget.grid_x = grid_x;
   widget.grid_y = grid_y;
   widget.grid_w = grid_w;
   widget.grid_h = grid_h;

   this->all_widgets.push_back(&widget);
   widget.dialog = this;
}

void Dialog::prepare()
{
   this->configure_all();

   /* XXX this isn't working right in X.  The mouse position is reported as
    * (0,0) initially, until the mouse pointer is moved.
    */
   ALLEGRO_MOUSE_STATE mst;
   al_get_mouse_state(&mst);
   this->check_mouse_over(mst.x, mst.y);
}

void Dialog::configure_all()
{
   const int xsize = al_get_display_width(display) / this->grid_m;
   const int ysize = al_get_display_height(display) / this->grid_n;

   for (std::list<Widget*>::iterator it = this->all_widgets.begin();
         it != this->all_widgets.end();
         ++it)
   {
      (*it)->configure(xsize, ysize, this->x_padding, this->y_padding);
   }
}

void Dialog::run_step(bool block)
{
   ALLEGRO_EVENT event;

   if (block) {
      al_wait_for_event(event_queue, NULL);
   }

   while (al_get_next_event(event_queue, &event)) {
      switch (event.type) {
         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            this->request_quit();
            break;

         case ALLEGRO_EVENT_KEY_CHAR:
            on_key_down(event.keyboard);
            break;

         case ALLEGRO_EVENT_MOUSE_AXES:
            on_mouse_axes(event.mouse);
            break;

         case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            on_mouse_button_down(event.mouse);
            break;

         case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            on_mouse_button_up(event.mouse);
            break;

         case ALLEGRO_EVENT_DISPLAY_EXPOSE:
            this->request_draw();
            break;

         default:
            if (event_handler) {
               event_handler->handle_event(event);
            }
            break;
      }
   }
}

void Dialog::on_key_down(const ALLEGRO_KEYBOARD_EVENT & event)
{
   if (event.display != this->display) {
      return;
   }

   // XXX think of something better when we need it
   if (event.keycode == ALLEGRO_KEY_ESCAPE) {
      this->request_quit();
   }

   if (this->key_widget) {
      this->key_widget->on_key_down(event);
   }
}

void Dialog::on_mouse_axes(const ALLEGRO_MOUSE_EVENT & event)
{
   const int mx = event.x;
   const int my = event.y;

   if (event.display != this->display) {
      return;
   }

   if (this->mouse_down_widget) {
      this->mouse_down_widget->on_mouse_button_hold(mx, my);
      return;
   }

   if (this->mouse_over_widget && this->mouse_over_widget->contains(mx, my)) {
      /* no change */
      return;
   }

   this->check_mouse_over(mx, my);
}

void Dialog::check_mouse_over(int mx, int my)
{
   for (std::list<Widget*>::iterator it = this->all_widgets.begin();
         it != this->all_widgets.end();
         ++it)
   {
      if ((*it)->contains(mx, my) && (*it)->want_mouse_focus()) {
         this->mouse_over_widget = (*it);
         this->mouse_over_widget->got_mouse_focus();
         return;
      }
   }

   if (this->mouse_over_widget) {
      this->mouse_over_widget->lost_mouse_focus();
      this->mouse_over_widget = NULL;
   }
}

void Dialog::on_mouse_button_down(const ALLEGRO_MOUSE_EVENT & event)
{
   if (event.button != 1)
      return;
   if (!this->mouse_over_widget)
      return;

   this->mouse_down_widget = this->mouse_over_widget;
   this->mouse_down_widget->on_mouse_button_down(event.x, event.y);

   /* transfer key focus */
   if (this->mouse_down_widget != this->key_widget) {
      if (this->key_widget) {
         this->key_widget->lost_key_focus();
         this->key_widget = NULL;
      }
      if (this->mouse_down_widget->want_key_focus()) {
         this->key_widget = this->mouse_down_widget;
         this->key_widget->got_key_focus();
      }
   }
}

void Dialog::on_mouse_button_up(const ALLEGRO_MOUSE_EVENT & event)
{
   if (event.button != 1)
      return;
   if (!this->mouse_down_widget)
      return;

   this->mouse_down_widget->on_mouse_button_up(event.x, event.y);
   if (this->mouse_down_widget->contains(event.x, event.y)) {
      this->mouse_down_widget->on_click(event.x, event.y);
   }
   this->mouse_down_widget = NULL;
}

void Dialog::request_quit()
{
   this->quit_requested = true;
}

bool Dialog::is_quit_requested() const
{
   return this->quit_requested;
}

void Dialog::request_draw()
{
   this->draw_requested = true;
}

bool Dialog::is_draw_requested() const
{
   return this->draw_requested;
}

void Dialog::draw()
{
   int cx, cy, cw, ch;
   al_get_clipping_rectangle(&cx, &cy, &cw, &ch);

   for (std::list<Widget *>::iterator it = this->all_widgets.begin();
         it != this->all_widgets.end();
         ++it)
   {
      Widget *wid = (*it);
      al_set_clipping_rectangle(wid->x1, wid->y1, wid->width(), wid->height());
      wid->draw();
   }

   al_set_clipping_rectangle(cx, cy, cw, ch);

   this->draw_requested = false;
}

const Theme & Dialog::get_theme() const
{
   return this->theme;
}

void Dialog::register_event_source(ALLEGRO_EVENT_SOURCE *source)
{
   al_register_event_source(this->event_queue, source);
}

void Dialog::set_event_handler(EventHandler *event_handler)
{
   this->event_handler = event_handler;
}

/*---------------------------------------------------------------------------*/

Label::Label(std::string text, bool centred) :
   text(text),
   centred(centred)
{
}

void Label::draw()
{
   const Theme & theme = this->dialog->get_theme();
   SaveState state;

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
   if (centred) {
      al_draw_text(theme.font, theme.fg, (this->x1 + this->x2 + 1)/2,
         this->y1, ALLEGRO_ALIGN_CENTRE, this->text.c_str());
   }
   else {
      al_draw_text(theme.font, theme.fg, this->x1, this->y1, 0, this->text.c_str());
   }
}

void Label::set_text(std::string new_text)
{
   this->text = new_text;
}

bool Label::want_mouse_focus()
{
   return false;
}

/*---------------------------------------------------------------------------*/

Button::Button(std::string text):
   text(text),
   pushed(false)
{
}

void Button::on_mouse_button_down(int mx, int my)
{
   (void)mx;
   (void)my;
   this->pushed = true;
   dialog->request_draw();
}

void Button::on_mouse_button_up(int mx, int my)
{
   (void)mx;
   (void)my;
   this->pushed = false;
   dialog->request_draw();
}

void Button::draw()
{
   const Theme & theme = this->dialog->get_theme();
   ALLEGRO_COLOR fg;
   ALLEGRO_COLOR bg;
   SaveState state;

   if (this->pushed) {
      fg = theme.bg;
      bg = theme.fg;
   }
   else {
      fg = theme.fg;
      bg = theme.bg;
   }

   al_draw_filled_rectangle(this->x1, this->y1,
      this->x2, this->y2, bg);
   al_draw_rectangle(this->x1 + 0.5, this->y1 + 0.5,
      this->x2 - 0.5, this->y2 - 0.5, fg, 0);
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
   al_draw_text(theme.font, fg, (this->x1 + this->x2 + 1)/2,
      this->y1, ALLEGRO_ALIGN_CENTRE, this->text.c_str());
}

bool Button::get_pushed()
{
   return pushed;
}

/*---------------------------------------------------------------------------*/

ToggleButton::ToggleButton(std::string text) :
   Button(text)
{
}

void ToggleButton::on_mouse_button_down(int mx, int my)
{
   (void)mx;
   (void)my;
   set_pushed(!this->pushed);
}

void ToggleButton::on_mouse_button_up(int mx, int my)
{
   (void)mx;
   (void)my;
}

void ToggleButton::set_pushed(bool pushed)
{
   if (this->pushed != pushed) {
      this->pushed = pushed;
      if (dialog)
         dialog->request_draw();
   }
}

/*---------------------------------------------------------------------------*/

const std::string List::empty_string;

List::List(int initial_selection) :
   selected_item(initial_selection)
{
}

bool List::want_key_focus()
{
   return true;
}

void List::on_key_down(const ALLEGRO_KEYBOARD_EVENT & event)
{
   switch (event.keycode) {
      case ALLEGRO_KEY_DOWN:
         if (selected_item < items.size() - 1) {
            selected_item++;
            dialog->request_draw();
         }
         break;

      case ALLEGRO_KEY_UP:
         if (selected_item > 0) {
            selected_item--;
            dialog->request_draw();
         }
         break;
   }
}

void List::on_click(int mx, int my)
{
   const Theme & theme = dialog->get_theme();
   unsigned int i = (my - this->y1) / al_get_font_line_height(theme.font);
   if (i < this->items.size()) {
      this->selected_item = i;
      dialog->request_draw();
   }

   (void)mx;
   (void)my;
}

void List::draw()
{
   const Theme & theme = dialog->get_theme();
   SaveState state;

   al_draw_filled_rectangle(x1 + 1, y1 + 1, x2 - 1, y2 - 1, theme.bg);

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
   const int font_height = al_get_font_line_height(theme.font);
   for (unsigned i = 0; i < items.size(); i++) {
      int yi = y1 + i * font_height;

      if (i == selected_item) {
         al_draw_filled_rectangle(x1 + 1, yi, x2 - 1, yi + font_height - 1,
            theme.highlight);
      }

      al_draw_text(theme.font, theme.fg, x1, yi, 0, items.at(i).c_str());
   }
}

void List::clear_items()
{
   this->items.clear();
   this->selected_item = 0;
}

void List::append_item(std::string text)
{
   this->items.push_back(text);
}

const std::string & List::get_selected_item_text() const
{
   if (this->selected_item < this->items.size())
      return this->items.at(this->selected_item);
   else
      return List::empty_string;
}

int List::get_cur_value() const
{
   return this->selected_item;
}

/*---------------------------------------------------------------------------*/

VSlider::VSlider(int cur_value, int max_value) :
   cur_value(cur_value),
   max_value(max_value)
{
}

void VSlider::on_mouse_button_down(int mx, int my)
{
   this->on_mouse_button_hold(mx, my);
}

void VSlider::on_mouse_button_hold(int mx, int my)
{
   double r = (double) (this->y2 - 1 - my) / (this->height() - 2);
   r = CLAMP(0.0, r, 1.0);
   cur_value = (int) (r * max_value);
   dialog->request_draw();

   (void)mx;
}

void VSlider::draw()
{
   const Theme & theme = dialog->get_theme();
   float left = x1 + 0.5, top = y1 + 0.5;
   float right = x2 + 0.5, bottom = y2 + 0.5;
   SaveState state;

   al_draw_rectangle(left, top, right, bottom, theme.fg, 1);

   double ratio = (double) this->cur_value / (double) this->max_value;
   int ypos = (int) (bottom - 0.5 - (int) (ratio * (height() - 7)));
   al_draw_filled_rectangle(left + 0.5, ypos - 5, right - 0.5, ypos, theme.fg);
}

int VSlider::get_cur_value() const
{
   return this->cur_value;
}

void VSlider::set_cur_value(int v)
{
   this->cur_value = v;
}

/*---------------------------------------------------------------------------*/

HSlider::HSlider(int cur_value, int max_value) :
   cur_value(cur_value),
   max_value(max_value)
{
}

void HSlider::on_mouse_button_down(int mx, int my)
{
   this->on_mouse_button_hold(mx, my);
}

void HSlider::on_mouse_button_hold(int mx, int my)
{
   double r = (double) (mx - 1 - this->x1) / (this->width() - 2);
   r = CLAMP(0.0, r, 1.0);
   cur_value = (int) (r * max_value);
   dialog->request_draw();

   (void)my;
}

void HSlider::draw()
{
   const Theme & theme = dialog->get_theme();
   const int cy = (y1 + y2) / 2;
   SaveState state;

   al_draw_filled_rectangle(x1, y1, x2, y2, theme.bg);
   al_draw_line(x1, cy, x2, cy, theme.fg, 0);

   double ratio = (double) this->cur_value / (double) this->max_value;
   int xpos = x1 + (int) (ratio * (width() - 2));
   al_draw_filled_rectangle(xpos - 2, y1, xpos + 2, y2, theme.fg);
}

int HSlider::get_cur_value() const
{
   return this->cur_value;
}

void HSlider::set_cur_value(int v)
{
   this->cur_value = v;
}

/*---------------------------------------------------------------------------*/

TextEntry::TextEntry(const char *initial_text) :
   focused(false),
   cursor_pos(0),
   left_pos(0)
{
   text = al_ustr_new(initial_text);
}

TextEntry::~TextEntry()
{
   al_ustr_free(text);
}

bool TextEntry::want_key_focus()
{
   return true;
}

void TextEntry::got_key_focus()
{
   this->focused = true;
   dialog->request_draw();
}

void TextEntry::lost_key_focus()
{
   this->focused = false;
   dialog->request_draw();
}

void TextEntry::on_key_down(const ALLEGRO_KEYBOARD_EVENT & event)
{
   switch (event.keycode) {
      case ALLEGRO_KEY_LEFT:
         al_ustr_prev(text, &cursor_pos);
         break;

      case ALLEGRO_KEY_RIGHT:
         al_ustr_next(text, &cursor_pos);
         break;

      case ALLEGRO_KEY_HOME:
         cursor_pos = 0;
         break;

      case ALLEGRO_KEY_END:
         cursor_pos = al_ustr_size(text);
         break;

      case ALLEGRO_KEY_DELETE:
         al_ustr_remove_chr(text, cursor_pos);
         break;

      case ALLEGRO_KEY_BACKSPACE:
         if (al_ustr_prev(text, &cursor_pos))
            al_ustr_remove_chr(text, cursor_pos);
         break;

      default:
         if (event.unichar >= ' ') {
            al_ustr_insert_chr(text, cursor_pos, event.unichar);
            cursor_pos += al_utf8_width(event.unichar);
         }
         break;
   }

   maybe_scroll();
   dialog->request_draw();
}

void TextEntry::maybe_scroll()
{
   const Theme & theme = dialog->get_theme();

   if (cursor_pos < left_pos + 3) {
      if (cursor_pos < 3)
         left_pos = 0;
      else
         left_pos = cursor_pos - 3;
   }
   else {
      for (;;) {
         const int tw = al_get_ustr_width(theme.font,
            UString(text, left_pos, cursor_pos));
         if (x1 + tw + CURSOR_WIDTH < x2) {
            break;
         }
         al_ustr_next(text, &left_pos);
      }
   }
}

void TextEntry::draw()
{
   const Theme & theme = dialog->get_theme();
   SaveState state;

   al_draw_filled_rectangle(x1, y1, x2, y2, theme.bg);

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);

   if (!focused) {
      al_draw_ustr(theme.font, theme.fg, x1, y1, 0, UString(text, left_pos));
   }
   else {
      int x = x1;

      if (cursor_pos > 0) {
         UString sub(text, left_pos, cursor_pos);
         al_draw_ustr(theme.font, theme.fg, x1, y1, 0, sub);
         x += al_get_ustr_width(theme.font, sub);
      }

      if ((unsigned) cursor_pos == al_ustr_size(text)) {
         al_draw_filled_rectangle(x, y1, x + CURSOR_WIDTH,
            y1 + al_get_font_line_height(theme.font), theme.fg);
      }
      else {
         int post_cursor = cursor_pos;
         al_ustr_next(text, &post_cursor);

         UString sub(text, cursor_pos, post_cursor);
         int subw = al_get_ustr_width(theme.font, sub);
         al_draw_filled_rectangle(x, y1, x + subw,
            y1 + al_get_font_line_height(theme.font), theme.fg);
         al_draw_ustr(theme.font, theme.bg, x, y1, 0, sub);
         x += subw;

         al_draw_ustr(theme.font, theme.fg, x, y1, 0,
            UString(text, post_cursor));
      }
   }
}

const char *TextEntry::get_text()
{
   return al_cstr(text);
}


/* vim: set sts=3 sw=3 et: */
