/*
 * This is a GUI for example programs that require GUI-style interaction.
 * It's intended to be as simple and transparent as possible (simplistic,
 * even).
 */

#include <iostream>
#include <string>
#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"

#include "nihgui.hpp"

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
   key_widget(NULL)
{
   this->event_queue = al_create_event_queue();
   al_register_event_source(this->event_queue,
      (ALLEGRO_EVENT_SOURCE *) al_get_keyboard());
   al_register_event_source(this->event_queue,
      (ALLEGRO_EVENT_SOURCE *) al_get_mouse());
   al_register_event_source(this->event_queue,
      (ALLEGRO_EVENT_SOURCE *) display);
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
   const int xsize = al_get_display_width() / this->grid_m;
   const int ysize = al_get_display_height() / this->grid_n;

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
         case ALLEGRO_EVENT_KEY_DOWN:
         case ALLEGRO_EVENT_KEY_REPEAT:
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
      if ((*it)->contains(mx, my)) {
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

/*---------------------------------------------------------------------------*/

Button::Button(std::string text):
   text(text),
   pushed(false)
{
}

void Button::on_mouse_button_down(int mx, int my)
{
   this->pushed = true;
   dialog->request_draw();
}

void Button::on_mouse_button_up(int mx, int my)
{
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

   al_draw_rectangle(this->x1, this->y1, this->x2, this->y2,
      fg, ALLEGRO_OUTLINED);
   al_draw_rectangle(this->x1 + 1, this->y1 + 1, this->x2 - 1, this->y2 - 1,
      bg, ALLEGRO_FILLED);
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, fg);
   al_font_textout_centre(theme.font, (this->x1 + this->x2 + 1)/2,
      this->y1, this->text.c_str(), -1);
}

/*---------------------------------------------------------------------------*/

const std::string List::empty_string;

List::List(int initial_selection) :
   selected_item(initial_selection)
{
}

void List::on_click(int mx, int my)
{
   const Theme & theme = dialog->get_theme();
   unsigned int i = (my - this->y1) / al_font_text_height(theme.font);
   if (i < this->items.size()) {
      this->selected_item = i;
      dialog->request_draw();
   }
}

void List::draw()
{
   const Theme & theme = dialog->get_theme();
   SaveState state;

   al_draw_rectangle(x1 + 1, y1 + 1, x2 - 1, y2 - 1, theme.bg, ALLEGRO_FILLED);

   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, theme.fg);
   const int font_height = al_font_text_height(theme.font);
   for (unsigned i = 0; i < items.size(); i++) {
      int yi = y1 + i * font_height;

      if (i == selected_item) {
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgb(255, 255, 255));
         al_draw_rectangle(x1 + 1, yi, x2 - 1, yi + font_height - 1,
            theme.highlight, ALLEGRO_FILLED);
         al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, theme.fg);
      }

      al_font_textout(theme.font, x1, yi, items.at(i).c_str(), -1);
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
}

void VSlider::draw()
{
   const Theme & theme = dialog->get_theme();
   const int cx = (x1 + x2) / 2;
   SaveState state;

   al_draw_rectangle(x1, y1, x2, y2, theme.bg, ALLEGRO_FILLED);
   al_draw_line(cx, y1, cx, y2, theme.fg);

   double ratio = (double) this->cur_value / (double) this->max_value;
   int ypos = y2 - (int) (ratio * (height() - 2));
   al_draw_rectangle(x1, ypos - 2, x2, ypos + 2, theme.fg, ALLEGRO_FILLED);
}

int VSlider::get_cur_value() const
{
   return this->cur_value;
}

/*---------------------------------------------------------------------------*/

TextEntry::TextEntry(std::string text) :
   text(text),
   focused(false),
   cursor_pos(0),
   left_pos(0)
{
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
         if (cursor_pos > 0)
            cursor_pos--;
         break;

      case ALLEGRO_KEY_RIGHT:
         if (cursor_pos < text.size())
            cursor_pos++;
         break;

      case ALLEGRO_KEY_HOME:
         cursor_pos = 0;
         break;

      case ALLEGRO_KEY_END:
         cursor_pos = text.size();
         break;

      case ALLEGRO_KEY_DELETE:
         if (cursor_pos < text.size())
            text.erase(cursor_pos, 1);
         break;

      case ALLEGRO_KEY_BACKSPACE:
         if (cursor_pos > 0)
            text.erase(--cursor_pos, 1);
         break;

      default:
         if (isprint(event.unichar))
            text.insert(cursor_pos++, 1, event.unichar);
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
         const char *s = text.c_str();
         const int tw = al_font_text_width(theme.font,
            s + left_pos, cursor_pos - left_pos);
         if (x1 + tw + CURSOR_WIDTH < x2) {
            break;
         }
         left_pos++;
      }
   }
}

void TextEntry::draw()
{
   const Theme & theme = dialog->get_theme();
   const char *s = text.c_str();
   SaveState state;

   al_draw_rectangle(x1, y1, x2, y2, theme.bg, ALLEGRO_FILLED);

   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, theme.fg);

   if (!focused) {
      al_font_textout(theme.font, x1, y1, s + left_pos, -1);
   }
   else {
      int x = x1;

      al_font_textout(theme.font, x1, y1, s + left_pos, cursor_pos - left_pos);
      x += al_font_text_width(theme.font, s + left_pos, cursor_pos - left_pos);

      if (cursor_pos == text.size()) {
         al_draw_rectangle(x, y1, x + CURSOR_WIDTH,
            y1 + al_font_text_height(theme.font), theme.fg, ALLEGRO_FILLED);
      }
      else {
         al_set_blender(ALLEGRO_INVERSE_ALPHA, ALLEGRO_ALPHA, theme.fg);
         al_font_textout(theme.font, x, y1, s + cursor_pos, 1);
         x += al_font_text_width(theme.font, s + cursor_pos, 1);

         al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, theme.fg);
         al_font_textout(theme.font, x, y1, s + cursor_pos + 1,
            text.size() - cursor_pos - 1);
      }
   }
}

const std::string & TextEntry::get_text()
{
   return text;
}

/* vim: set sts=3 sw=3 et: */
