#include "global.h"
#include "background_scroller.h"
#include "credits.h"
#include "menu.h"
#include "music.h"
#include "vcontroller.h"
#include "gamepad.h"
#include "mouse.h"

#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))

static int selected_item;
static int item_count;
static int locked;
static int freq_variation = 100;

void init_demo_menu(DEMO_MENU * menu, int PlayMusic)
{
   int i;

   selected_item = -1;
   item_count = 0;
   locked = 0;

   for (i = 0; menu[i].proc != NULL; i++) {
      menu[i].proc(&menu[i], DEMO_MENU_MSG_INIT, 0);
   }
   item_count = i;

   for (i = 0; menu[i].proc != NULL; i++) {
      if ((menu[i].flags & DEMO_MENU_SELECTED)) {
         selected_item = i;
         break;
      }
   }

   if (selected_item == -1) {
      for (i = 0; menu[i].proc != NULL; i++) {
         if ((menu[i].flags & DEMO_MENU_SELECTABLE)) {
            selected_item = i;
            menu[i].flags |= DEMO_MENU_SELECTED;
            break;
         }
      }
   }

   if (PlayMusic)
      play_music(DEMO_MIDI_MENU, 1);
}


int update_demo_menu(DEMO_MENU * menu)
{
   int tmp, c;

   update_background();
   update_credits();
   
   if (selected_item != -1) {
      menu[selected_item].proc(&menu[selected_item], DEMO_MENU_MSG_KEY, 0);
   }

   for (tmp = 0; menu[tmp].proc != 0; tmp++) {
      if (menu[tmp].proc(&menu[tmp], DEMO_MENU_MSG_TICK, 0) == DEMO_MENU_LOCK) {
         locked = 0;
         return DEMO_MENU_CONTINUE;
      }
   }

   if (locked == 1) {
      return DEMO_MENU_CONTINUE;
   }

   if (key_pressed(A5O_KEY_ESCAPE)) {
      return DEMO_MENU_BACK;
   }
   
   /* If a mouse button is pressed, select the item under it and send a
    * DEMO_MENU_MSG_CHAR with 13 to it (which is the same effect as hitting
    * the return key).
    */
   if (mouse_button_pressed(1)) {
      int i;
      for (i = 0; menu[i].proc != NULL; i++) {
         if (mouse_x() >= menu[i].x && mouse_y() >= menu[i].y &&
               mouse_x() < menu[i].x + menu[i].w &&
               mouse_y() < menu[i].y + menu[i].h) {
            if (menu[i].flags & DEMO_MENU_SELECTABLE) {
               if (selected_item != -1) {
                  menu[selected_item].flags &= ~DEMO_MENU_SELECTED;
               }
               selected_item = i;
               menu[i].flags |= DEMO_MENU_SELECTED;
               play_sound_id(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);
               return menu[i].proc(&menu[i], DEMO_MENU_MSG_CHAR, 13);
            }
         }
      }
   }

   if (key_pressed(A5O_KEY_UP)) {
      if (selected_item != -1) {
         tmp = selected_item;

         while (1) {
            --selected_item;
            if (selected_item < 0) {
               selected_item = item_count - 1;
            }

            if (menu[selected_item].flags & DEMO_MENU_SELECTABLE) {
               break;
            }
         }

         if (tmp != selected_item) {
            menu[tmp].flags &= ~DEMO_MENU_SELECTED;
            menu[selected_item].flags |= DEMO_MENU_SELECTED;
            play_sound_id(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation,
                       0);
         }
      }
   }
    
   if (key_pressed(A5O_KEY_DOWN)) {
      if (selected_item != -1) {
         tmp = selected_item;

         while (1) {
            ++selected_item;
            if (selected_item >= item_count) {
               selected_item = 0;
            }

            if (menu[selected_item].flags & DEMO_MENU_SELECTABLE) {
               break;
            }
         }

         if (tmp != selected_item) {
            menu[tmp].flags &= ~DEMO_MENU_SELECTED;
            menu[selected_item].flags |= DEMO_MENU_SELECTED;
            play_sound_id(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation,
                       0);
         }
      }
   }
    
    c = unicode_char(true);
    if(gamepad_button()) {
        c = 32;
    }

   if (selected_item != -1 && c) {
      tmp = menu[selected_item].proc(&menu[selected_item],
         DEMO_MENU_MSG_CHAR, c);
      if (tmp == DEMO_MENU_LOCK) {
         locked = 1;
         return DEMO_MENU_CONTINUE;
      } else {
         locked = 0;
         return tmp;
      }
   }

   return DEMO_MENU_CONTINUE;
}


void draw_demo_menu(DEMO_MENU * menu)
{
   int i, x, y, h, w;
   int tmp;
   static char logo_text[] = "Demo Game";

   draw_background();
   draw_credits();

   x = screen_width / 2;
   y = 1 * screen_height / 6 - al_get_font_line_height(demo_font_logo) / 2;
   demo_textprintf_centre(demo_font_logo, x + 6, y + 5,
                        al_map_rgba_f(0.125, 0.125, 0.125, 0.25), logo_text);
   demo_textprintf_centre(demo_font_logo, x, y, al_map_rgb_f(1, 1, 1), logo_text);

   /* calculate height of the whole menu and the starting y coordinate */
   h = 0;
   for (i = 0, h = 0; menu[i].proc != NULL; i++) {
      h += menu[i].proc(&menu[i], DEMO_MENU_MSG_HEIGHT, 0);
   }
   h += 2 * 8;
   y = 3 * screen_height / 5 - h / 2;

   /* calculate the width of the whole menu */
   w = 0;
   for (i = 0; menu[i].proc != NULL; i++) {
      tmp = menu[i].proc(&menu[i], DEMO_MENU_MSG_WIDTH, 0);
      menu[i].w = tmp;
      menu[i].x = (screen_width - tmp) / 2;
      if (tmp > w) {
         w = tmp;
      }
   }
   w += 2 * 8;
   if (w < screen_width / 3) w =  screen_width / 3;
   if (w > screen_width) w = screen_width;
   x = (screen_width - w) / 2;

   /* draw menu background */
   al_draw_filled_rectangle(x, y, x + w, y + h,
      al_map_rgba_f(0.37 * 0.6, 0.42 * 0.6, 0.45 * 0.6, 0.6));
   al_draw_rectangle(x, y, x + w, y + h, al_map_rgb(0, 0, 0), 1);

   /* draw menu items */
   y += 8;
   for (i = 0; menu[i].proc != NULL; i++) {
      menu[i].proc(&menu[i], DEMO_MENU_MSG_DRAW, y);
      menu[i].y = y;
      tmp = menu[i].proc(&menu[i], DEMO_MENU_MSG_HEIGHT, 0);
      menu[i].h = tmp;
      y += tmp;
   }
}


int demo_text_proc(DEMO_MENU * item, int msg, int extra)
{
   if (msg == DEMO_MENU_MSG_DRAW) {
      shadow_textprintf(demo_font, screen_width / 2,
                      extra, al_map_rgb(210, 230, 255), 2, item->name);
   } else if (msg == DEMO_MENU_MSG_WIDTH) {
      return al_get_text_width(demo_font, item->name);
   } else if (msg == DEMO_MENU_MSG_HEIGHT) {
      return al_get_font_line_height(demo_font) + 8;
   }

   return DEMO_MENU_CONTINUE;
}


int demo_edit_proc(DEMO_MENU * item, int msg, int extra)
{
   A5O_COLOR col;
   int w, h, x;
   int l, c;

   if (msg == DEMO_MENU_MSG_DRAW) {
      if (item->flags & DEMO_MENU_SELECTED) {
         col = al_map_rgb(255, 255, 0);
      } else {
         col = al_map_rgb(255, 255, 255);
      }

      w = demo_edit_proc(item, DEMO_MENU_MSG_WIDTH, 0);
      h = al_get_font_line_height(demo_font);

      al_draw_filled_rectangle((screen_width - w) / 2 - 2, extra - 2,
               (screen_width + w) / 2 + 2, extra + h + 2, al_map_rgb(0, 0, 0));
      al_draw_rectangle((screen_width - w) / 2 - 2, extra - 2,
           (screen_width + w) / 2 + 2, extra + h + 2, col, 1);
      shadow_textprintf(demo_font, screen_width / 2,
                      extra, col, 2, item->name);
      if (item->flags & DEMO_MENU_SELECTED) {
         x = (screen_width + al_get_text_width(demo_font, item->name)) / 2 + 2;
         al_draw_line(x + 0.5, extra + 2, x + 0.5, extra + h - 2, col, 1);
         al_draw_line(x + 1.5, extra + 2, x + 1.5, extra + h - 2, col, 1);
      }
   } else if (msg == DEMO_MENU_MSG_CHAR) {
      switch (extra) {
         case 8:
            l = strlen(item->name);
            if (l > 0) {
               item->name[l - 1] = 0;

               if (item->on_activate) {
                  item->on_activate(item);
               }

               play_sound_id(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);
            }
            break;

         default:
            l = strlen(item->name);
            c = extra & 0xff;
            if (l < item->extra && c >= 0x20 && c < 0x7f) {
               item->name[l] = c;

               if (item->on_activate) {
                  item->on_activate(item);
               }

               play_sound_id(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);
            }
            break;
      }
   } else if (msg == DEMO_MENU_MSG_WIDTH) {
      return MAX(al_get_text_width(demo_font, item->name),
                 item->extra * al_get_text_width(demo_font, " "));
   } else if (msg == DEMO_MENU_MSG_HEIGHT) {
      return al_get_font_line_height(demo_font) + 8;
   }

   return DEMO_MENU_CONTINUE;
}


int demo_button_proc(DEMO_MENU * item, int msg, int extra)
{
   A5O_COLOR col;

   if (msg == DEMO_MENU_MSG_DRAW) {
      if (item->flags & DEMO_MENU_SELECTED) {
         col = al_map_rgb(255, 255, 0);
      } else {
         col = al_map_rgb(255, 255, 255);
      }

      shadow_textprintf(demo_font, screen_width / 2,
                      extra, col, 2, item->name);
   } else if (msg == DEMO_MENU_MSG_CHAR) {
      switch (extra) {
         case 13:
         case 32:
            if (item->on_activate) {
               item->on_activate(item);
            }

            play_sound_id(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);
            return item->extra;
            break;
      }
   } else if (msg == DEMO_MENU_MSG_WIDTH || msg == DEMO_MENU_MSG_HEIGHT) {
      return demo_text_proc(item, msg, extra);
   }

   return DEMO_MENU_CONTINUE;
}


int demo_choice_proc(DEMO_MENU * item, int msg, int extra)
{
   int x, cw, ch, dy, cx;
   int choice_count = 0;
   int slider_width = screen_width / 6;
   int i, tmp;
   A5O_COLOR col;

   /* count number of choices */
   for (; item->data[choice_count] != 0; choice_count++);

   if (msg == DEMO_MENU_MSG_DRAW) {
      if (!choice_count) return DEMO_MENU_CONTINUE;
      if (item->flags & DEMO_MENU_SELECTED) {
         col = al_map_rgb(255, 255, 0);
      } else {
         col = al_map_rgb(255, 255, 255);
      }

      /* starting position */
      x = (screen_width - slider_width) / 2;

      /* print name of the item */
      shadow_textprintf(demo_font, x - 8, extra, col, 1,
                      item->name);

      /* draw slider thingy */
      ch = al_get_font_line_height(demo_font) / 2;
      ch = MAX(8, ch);
      dy = (al_get_font_line_height(demo_font) - ch) / 2;

      /* shadow */
      al_draw_rectangle(x + shadow_offset,
           extra + dy + shadow_offset,
           x + slider_width + shadow_offset, extra + dy + ch + shadow_offset,
           al_map_rgb(0, 0, 0), 1);
      cw = (slider_width - 4) / choice_count;
      cw = MAX(cw, 8);
      cx = (slider_width - 4) * item->extra / choice_count;
      if (cx + cw > slider_width - 4) {
         cx = slider_width - 4 - cw;
      }
      if (item->extra == choice_count - 1) {
         cw = slider_width - 4 - cx;
      }
      al_draw_filled_rectangle(x + 3 + cx, extra + dy + 3,
               x + 3 + cx + cw, extra + dy + ch, al_map_rgb(0, 0, 0));

      /* slider */
      al_draw_rectangle(x, extra + dy, x + slider_width, extra + dy + ch,
           col, 1);
      al_draw_filled_rectangle(x + 2 + cx, extra + dy + 2, x + 2 + cx + cw,
               extra + dy + ch - 2, col);

      x += slider_width;

      /* print selected choice */
      shadow_textprintf(demo_font, x + 8, extra, col,
                      0, (char *)(item->data)[item->extra]);
   } else if (msg == DEMO_MENU_MSG_KEY) {
      if (key_pressed(A5O_KEY_LEFT)) {
         if (item->extra > 0) {
            --item->extra;
            play_sound_id(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);

            if (item->on_activate) {
               item->on_activate(item);
            }
         }
      }
            
      if (key_pressed(A5O_KEY_RIGHT)) {
         if (item->extra < choice_count - 1) {
            ++item->extra;
            play_sound_id(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);

            if (item->on_activate) {
               item->on_activate(item);
            }
         }
      }
   } else if (msg == DEMO_MENU_MSG_CHAR && extra == 13) {
      if (mouse_button_pressed(1)) {
         x = (screen_width - slider_width) / 2;
         item->extra = (mouse_x() - x) * choice_count / slider_width;
         play_sound_id(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);
         if (item->on_activate) {
            item->on_activate(item);
         }
      }
   } else if (msg == DEMO_MENU_MSG_WIDTH) {
      cw = al_get_text_width(demo_font, item->name);
      for (i = 0; item->data[i] != 0; i++) {
         tmp = al_get_text_width(demo_font, (char *)(item->data)[i]);
         if (tmp > cw) {
            cw = tmp;
         }
      }

      return MAX(al_get_text_width(demo_font, item->name),
                 cw) * 2 + slider_width + 2 * 8;
   } else if (msg == DEMO_MENU_MSG_HEIGHT) {
      return demo_text_proc(item, msg, extra);
   }

   return DEMO_MENU_CONTINUE;
}



int demo_key_proc(DEMO_MENU * item, int msg, int extra)
{
   A5O_COLOR col;

   if (msg == DEMO_MENU_MSG_DRAW) {
      if (item->flags & DEMO_MENU_SELECTED) {
         col = al_map_rgb(255, 255, 0);
      } else {
         col = al_map_rgb(255, 255, 255);
      }

      shadow_textprintf(demo_font, screen_width / 2 - 16,
                      extra, col, 1, item->name);

      if (item->flags & DEMO_MENU_EXTRA) {
         shadow_textprintf(demo_font,
                         screen_width / 2 + 16, extra, col, 0, "...");
      } else {
         shadow_textprintf(demo_font,
                         screen_width / 2 + 16, extra, col, 0,
                         controller[controller_id]->
                         get_button_description(controller
                                                [controller_id],
                                                item->extra));
      }
   } else if (msg == DEMO_MENU_MSG_CHAR) {
      switch (extra) {
         case 13:
         case 32:
            item->flags |= DEMO_MENU_EXTRA;
            play_sound_id(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);
            return DEMO_MENU_LOCK;
            break;
      }
   } else if (msg == DEMO_MENU_MSG_TICK) {
      if (item->flags & DEMO_MENU_EXTRA) {
         if (controller[controller_id]->
             calibrate_button(controller[controller_id], item->extra) == 1) {
            item->flags &= ~DEMO_MENU_EXTRA;
            play_sound_id(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);
            if (item->on_activate) {
               item->on_activate(item);
            }
            return DEMO_MENU_LOCK;
         } else if (key_pressed(A5O_KEY_ESCAPE)) {
            item->flags &= ~DEMO_MENU_EXTRA;
            return DEMO_MENU_LOCK;
         }
      }
   } else if (msg == DEMO_MENU_MSG_WIDTH) {
      int w1 = al_get_text_width(demo_font, item->name);
      int w2 = al_get_text_width(demo_font,
                           (item->
                            flags & DEMO_MENU_EXTRA) ? "..." :
                           controller[controller_id]->
                           get_button_description(controller[controller_id],
                                                  item->extra));

      return 2 * (16 + ((w2 > w1) ? w2 : w1));
   } else if (msg == DEMO_MENU_MSG_HEIGHT) {
      return demo_text_proc(item, msg, extra);
   }

   return DEMO_MENU_CONTINUE;
}


int demo_color_proc(DEMO_MENU * item, int msg, int extra)
{
   int x, h, cw, cx, i, c;
   A5O_COLOR col1, col2;
   int rgb[3];
   static char buf[64];
   int changed = 0;
   int slider_width = screen_width / 6;

   slider_width /= 3;
   slider_width -= 4;

   if (msg == DEMO_MENU_MSG_DRAW) {
      if (item->flags & DEMO_MENU_SELECTED) {
         col1 = al_map_rgb(255, 255, 0);
         col2 = al_map_rgb(255, 255, 255);
      } else {
         col1 = al_map_rgb(255, 255, 255);
         col2 = al_map_rgb(255, 255, 255);
      }

      x = screen_width / 2 - (slider_width + 4) * 3 / 2;
      h = al_get_font_line_height(demo_font);

      shadow_textprintf(demo_font, x - 8, extra, col1,
                      1, item->name);

      c = *(int *)(item->data);
      rgb[0] = c & 255;
      rgb[1] = (c >> 8) & 255;
      rgb[2] = (c >> 16) & 255;

      for (i = 0; i < 3; i++) {
         cw = 4;
         cx = (slider_width - 4 - cw) * rgb[i] / 255;

         al_draw_rectangle(x + 2, extra + 5,
              x + slider_width + 2, extra + h - 1, al_map_rgb(0, 0, 0), 1);
         al_draw_filled_rectangle(x + 3 + cx, extra + 6,
                  x + 3 + cx + cw, extra + h - 4, al_map_rgb(0, 0, 0));

         al_draw_rectangle(x, extra + 3, x + slider_width,
              extra + h - 3, item->extra == i ? col1 : col2, 1);
         al_draw_filled_rectangle(x + 2 + cx, extra + 5,
                  x + 2 + cx + cw, extra + h - 5,
                  item->extra == i ? col1 : col2);

         x += slider_width + 4;
      }

      snprintf(buf, sizeof(buf), "%d,%d,%d", rgb[0], rgb[1], rgb[2]);
      shadow_textprintf(demo_font, x + 8, extra, al_map_rgb(rgb[0], rgb[1], rgb[2]), 0, buf);
   } else if (msg == DEMO_MENU_MSG_KEY) {
      c = *(int *)(item->data);

      rgb[0] = (c >> 0) & 255;
      rgb[1] = (c >> 8) & 255;
      rgb[2] = (c >> 16) & 255;
   
   

     if (key_pressed(A5O_KEY_LEFT)) {
         if (rgb[item->extra] > 0) {
            if (key_down(A5O_KEY_LSHIFT) || key_down(A5O_KEY_RSHIFT)) {
               --rgb[item->extra];
            } else {
               rgb[item->extra] -= 16;
               rgb[item->extra] = MAX(0, rgb[item->extra]);
            }

            changed = 1;
         }
      }

      if (key_pressed(A5O_KEY_RIGHT)) {
         if (rgb[item->extra] < 255) {
            if (key_down(A5O_KEY_LSHIFT) || key_down(A5O_KEY_RSHIFT)) {
               ++rgb[item->extra];
            } else {
               rgb[item->extra] += 16;
               rgb[item->extra] = MIN(255, rgb[item->extra]);
            }

            changed = 1;
         }
      }

      if (key_pressed(A5O_KEY_TAB)) {
         if (key_down(A5O_KEY_LSHIFT) || key_down(A5O_KEY_RSHIFT)) {
            --item->extra;
            if (item->extra < 0) {
               item->extra += 3;
            }
         } else {
            ++item->extra;
            item->extra %= 3;
         }
         play_sound_id(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);
         if (item->on_activate) {
            item->on_activate(item);
         }
      }

      if (changed) {
         *(int *)(item->data) = rgb[0] + (rgb[1] << 8) + (rgb[2] << 16);

         play_sound_id(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);

         if (item->on_activate) {
            item->on_activate(item);
         }
      }
   } else if (msg == DEMO_MENU_MSG_WIDTH) {
      return MAX(al_get_text_width(demo_font, item->name) * 2 + 8 * 2 +
                 3 * (slider_width + 4),
                 8 * 2 + 3 * (slider_width + 4) +
                 2 * al_get_text_width(demo_font, "255,255,255"));
   } else if (msg == DEMO_MENU_MSG_HEIGHT) {
      return al_get_font_line_height(demo_font);
   }

   return DEMO_MENU_CONTINUE;
}


int demo_separator_proc(DEMO_MENU * item, int msg, int extra)
{
   if (msg == DEMO_MENU_MSG_WIDTH) {
      return extra - extra;
   } else if (msg == DEMO_MENU_MSG_HEIGHT) {
      return item->extra;
   }

   return DEMO_MENU_CONTINUE;
}
