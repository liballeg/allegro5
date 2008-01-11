#include <allegro.h>
#include "../include/backscrl.h"
#include "../include/credits.h"
#include "../include/global.h"
#include "../include/menu.h"
#include "../include/music.h"
#include "../include/virtctl.h"


static int selected_item;
static int item_count;
BITMAP *demo_menu_canvas;
static int locked;
static int freq_variation = 100;

void init_demo_menu(DEMO_MENU * menu, int PlayMusic)
{
   int i;

   set_palette(demo_data[DEMO_MENU_PALETTE].dat);
   set_keyboard_rate(0, 0);
   clear_keybuf();

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
   int tmp;

   update_background();
   update_credits();

   for (tmp = 0; menu[tmp].proc != 0; tmp++) {
      if (menu[tmp].proc(&menu[tmp], DEMO_MENU_MSG_TICK, 0) == DEMO_MENU_LOCK) {
         locked = 0;
         return DEMO_MENU_CONTINUE;
      }
   }

   if (locked == 1) {
      return DEMO_MENU_CONTINUE;
   }

   if (keypressed()) {
      int c = readkey();

      clear_keybuf();
      switch (c >> 8) {
         case KEY_ESC:
            return DEMO_MENU_BACK;
            break;

         case KEY_UP:{
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
                     play_sound(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation,
                                0);
                  }
               }
            }
            break;

         case KEY_DOWN:{
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
                     play_sound(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation,
                                0);
                  }
               }
            }
            break;

         default:
            if (selected_item != -1) {
               tmp =
                  menu[selected_item].proc(&menu[selected_item],
                                           DEMO_MENU_MSG_KEY, c);
               if (tmp == DEMO_MENU_LOCK) {
                  locked = 1;
                  return DEMO_MENU_CONTINUE;
               } else {
                  locked = 0;
                  return tmp;
               }
            }
      };
   }

   return DEMO_MENU_CONTINUE;
}


void draw_demo_menu(BITMAP *canvas, DEMO_MENU * menu)
{
   int i, x, y, h, w;
   int tmp;
   static char logo_text[] = "Demo Game";

   /* make sure the procs know where to draw themselves */
   demo_menu_canvas = canvas;

   draw_background(canvas);
   draw_credits(canvas);

   x = SCREEN_W / 2;
   y = 1 * SCREEN_H / 6 - text_height(demo_font_logo) / 2;
   demo_textprintf_centre(canvas, demo_font_logo_m, x + 6, y + 5,
                        makecol(128, 128, 128), -1, logo_text);
   demo_textprintf_centre(canvas, demo_font_logo, x, y, -1, -1, logo_text);

   /* calculate height of the whole menu and the starting y coordinate */
   h = 0;
   for (i = 0, h = 0; menu[i].proc != NULL; i++) {
      h += menu[i].proc(&menu[i], DEMO_MENU_MSG_HEIGHT, 0);
   }
   h += 2 * 8;
   y = 3 * SCREEN_H / 5 - h / 2;

   /* calculate the width of the whole menu */
   w = 0;
   for (i = 0; menu[i].proc != NULL; i++) {
      tmp = menu[i].proc(&menu[i], DEMO_MENU_MSG_WIDTH, 0);
      if (tmp > w) {
         w = tmp;
      }
   }
   w += 2 * 8;
   w = MID(SCREEN_W / 3, w, SCREEN_W);
   x = (SCREEN_W - w) / 2;

   /* draw menu background */
   if (update_driver_id == DEMO_DOUBLE_BUFFER) {
      drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
      set_trans_blender(0, 0, 0, 144);
      rectfill(canvas, x, y, x + w, y + h, makecol(96, 108, 116));
      drawing_mode(DRAW_MODE_SOLID, 0, 0, 0);
   } else {
      rectfill(canvas, x, y, x + w, y + h, makecol(96, 108, 116));
   }
   rect(canvas, x, y, x + w, y + h, makecol(0, 0, 0));

   /* draw menu items */
   y += 8;
   for (i = 0; menu[i].proc != NULL; i++) {
      menu[i].proc(&menu[i], DEMO_MENU_MSG_DRAW, y);
      y += menu[i].proc(&menu[i], DEMO_MENU_MSG_HEIGHT, 0);
   }
}


int demo_text_proc(DEMO_MENU * item, int msg, int extra)
{
   if (msg == DEMO_MENU_MSG_DRAW) {
      shadow_textprintf(demo_menu_canvas, demo_font, SCREEN_W / 2,
                      extra, makecol(210, 230, 255), 2, item->name);
   } else if (msg == DEMO_MENU_MSG_WIDTH) {
      return text_length(demo_font, item->name);
   } else if (msg == DEMO_MENU_MSG_HEIGHT) {
      return text_height(demo_font) + 8;
   }

   return DEMO_MENU_CONTINUE;
}


int demo_edit_proc(DEMO_MENU * item, int msg, int extra)
{
   int col, w, h, x;
   int l, c;

   if (msg == DEMO_MENU_MSG_DRAW) {
      if (item->flags & DEMO_MENU_SELECTED) {
         col = makecol(255, 255, 0);
      } else {
         col = makecol(255, 255, 255);
      }

      w = demo_edit_proc(item, DEMO_MENU_MSG_WIDTH, 0);
      h = text_height(demo_font);

      rectfill(demo_menu_canvas, (SCREEN_W - w) / 2 - 2, extra - 2,
               (SCREEN_W + w) / 2 + 2, extra + h + 2, 0);
      rect(demo_menu_canvas, (SCREEN_W - w) / 2 - 2, extra - 2,
           (SCREEN_W + w) / 2 + 2, extra + h + 2, col);
      shadow_textprintf(demo_menu_canvas, demo_font, SCREEN_W / 2,
                      extra, col, 2, item->name);
      if (item->flags & DEMO_MENU_SELECTED) {
         x = (SCREEN_W + text_length(demo_font, item->name)) / 2 + 2;
         vline(demo_menu_canvas, x, extra + 2, extra + h - 2, col);
         vline(demo_menu_canvas, x + 1, extra + 2, extra + h - 2, col);
      }
   } else if (msg == DEMO_MENU_MSG_KEY) {
      switch (extra >> 8) {
         case KEY_BACKSPACE:
            l = ustrlen(item->name);
            if (l > 0) {
               item->name[l - 1] = 0;

               if (item->on_activate) {
                  item->on_activate(item);
               }

               play_sound(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);
            }
            break;

         default:
            l = ustrlen(item->name);
            c = extra & 0xff;
            if (l < item->extra && c >= 0x20 && c < 0x7f) {
               item->name[l] = c;

               if (item->on_activate) {
                  item->on_activate(item);
               }

               play_sound(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);
            }
            break;
      }
   } else if (msg == DEMO_MENU_MSG_WIDTH) {
      return MAX(text_length(demo_font, item->name),
                 item->extra * text_length(demo_font, " "));
   } else if (msg == DEMO_MENU_MSG_HEIGHT) {
      return text_height(demo_font) + 8;
   }

   return DEMO_MENU_CONTINUE;
}


int demo_button_proc(DEMO_MENU * item, int msg, int extra)
{
   int col;

   if (msg == DEMO_MENU_MSG_DRAW) {
      if (item->flags & DEMO_MENU_SELECTED) {
         col = makecol(255, 255, 0);
      } else {
         col = makecol(255, 255, 255);
      }

      shadow_textprintf(demo_menu_canvas, demo_font, SCREEN_W / 2,
                      extra, col, 2, item->name);
   } else if (msg == DEMO_MENU_MSG_KEY) {
      switch (extra >> 8) {
         case KEY_ENTER:
         case KEY_SPACE:
            if (item->on_activate) {
               item->on_activate(item);
            }

            play_sound(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);
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
   int col, x, cw, ch, dy, cx;
   int choice_count = 0;
   int slider_width = SCREEN_W / 6;
   int i, tmp;

   /* count number of choices */
   for (; item->data[choice_count] != 0; choice_count++);

   if (msg == DEMO_MENU_MSG_DRAW) {
      if (item->flags & DEMO_MENU_SELECTED) {
         col = makecol(255, 255, 0);
      } else {
         col = makecol(255, 255, 255);
      }

      /* starting position */
      x = (SCREEN_W - slider_width) / 2;

      /* print name of the item */
      shadow_textprintf(demo_menu_canvas, demo_font, x - 8, extra, col, 1,
                      item->name);

      /* draw slider thingy */
      ch = text_height(demo_font) / 2;
      ch = MAX(8, ch);
      dy = (text_height(demo_font) - ch) / 2;

      /* shadow */
      rect(demo_menu_canvas, x + shadow_offset,
           extra + dy + shadow_offset,
           x + slider_width + shadow_offset, extra + dy + ch + shadow_offset,
           0);
      cw = (slider_width - 4) / choice_count;
      cw = MAX(cw, 8);
      cx = (slider_width - 4) * item->extra / choice_count;
      if (cx + cw > slider_width - 4) {
         cx = slider_width - 4 - cw;
      }
      if (item->extra == choice_count - 1) {
         cw = slider_width - 4 - cx;
      }
      rectfill(demo_menu_canvas, x + 3 + cx, extra + dy + 3,
               x + 3 + cx + cw, extra + dy + ch - 1, 0);

      /* slider */
      rect(demo_menu_canvas, x, extra + dy, x + slider_width, extra + dy + ch,
           col);
      rectfill(demo_menu_canvas, x + 2 + cx, extra + dy + 2, x + 2 + cx + cw,
               extra + dy + ch - 2, col);

      x += slider_width;

      /* print selected choice */
      shadow_textprintf(demo_menu_canvas, demo_font, x + 8, extra, col,
                      0, (char *)(item->data)[item->extra]);
   } else if (msg == DEMO_MENU_MSG_KEY) {
      switch (extra >> 8) {
         case KEY_LEFT:
            if (item->extra > 0) {
               --item->extra;
               play_sound(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);

               if (item->on_activate) {
                  item->on_activate(item);
               }
            }
            break;

         case KEY_RIGHT:
            if (item->extra < choice_count - 1) {
               ++item->extra;
               play_sound(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);

               if (item->on_activate) {
                  item->on_activate(item);
               }
            }
            break;
      }
   } else if (msg == DEMO_MENU_MSG_WIDTH) {
      cw = text_length(demo_font, item->name);
      for (i = 0; item->data[i] != 0; i++) {
         tmp = text_length(demo_font, (char *)(item->data)[i]);
         if (tmp > cw) {
            cw = tmp;
         }
      }

      return MAX(text_length(demo_font, item->name),
                 cw) * 2 + slider_width + 2 * 8;
   } else if (msg == DEMO_MENU_MSG_HEIGHT) {
      return demo_text_proc(item, msg, extra);
   }

   return DEMO_MENU_CONTINUE;
}



int demo_key_proc(DEMO_MENU * item, int msg, int extra)
{
   int col;

   if (msg == DEMO_MENU_MSG_DRAW) {
      if (item->flags & DEMO_MENU_SELECTED) {
         col = makecol(255, 255, 0);
      } else {
         col = makecol(255, 255, 255);
      }

      shadow_textprintf(demo_menu_canvas, demo_font, SCREEN_W / 2 - 16,
                      extra, col, 1, item->name);

      if (item->flags & DEMO_MENU_EXTRA) {
         shadow_textprintf(demo_menu_canvas, demo_font,
                         SCREEN_W / 2 + 16, extra, col, 0, "...");
      } else {
         shadow_textprintf(demo_menu_canvas, demo_font,
                         SCREEN_W / 2 + 16, extra, col, 0,
                         controller[controller_id]->
                         get_button_description(controller
                                                [controller_id],
                                                item->extra));
      }
   } else if (msg == DEMO_MENU_MSG_KEY) {
      switch (extra >> 8) {
         case KEY_ENTER:
         case KEY_SPACE:
            item->flags |= DEMO_MENU_EXTRA;
            play_sound(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);
            return DEMO_MENU_LOCK;
            break;
      }
   } else if (msg == DEMO_MENU_MSG_TICK) {
      if (item->flags & DEMO_MENU_EXTRA) {
         if (controller[controller_id]->
             calibrate_button(controller[controller_id], item->extra) == 1) {
            item->flags &= ~DEMO_MENU_EXTRA;
            play_sound(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);
            if (item->on_activate) {
               item->on_activate(item);
            }
            return DEMO_MENU_LOCK;
         } else if (key[KEY_ESC]) {
            item->flags &= ~DEMO_MENU_EXTRA;
            return DEMO_MENU_LOCK;
         }
      }
   } else if (msg == DEMO_MENU_MSG_WIDTH) {
      int w1 = text_length(demo_font, item->name);
      int w2 = text_length(demo_font,
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
   int col1, col2, x, h, cw, cx, i, c;
   int rgb[3];
   static char buf[64];
   int changed = 0;
   int slider_width = SCREEN_W / 6;

   slider_width /= 3;
   slider_width -= 4;

   if (msg == DEMO_MENU_MSG_DRAW) {
      if (item->flags & DEMO_MENU_SELECTED) {
         col1 = makecol(255, 255, 0);
         col2 = makecol(255, 255, 255);
      } else {
         col1 = makecol(255, 255, 255);
         col2 = makecol(255, 255, 255);
      }

      x = SCREEN_W / 2 - (slider_width + 4) * 3 / 2;
      h = text_height(demo_font);

      shadow_textprintf(demo_menu_canvas, demo_font, x - 8, extra, col1,
                      1, item->name);

      c = *(int *)(item->data);
      rgb[0] = getr(c);
      rgb[1] = getg(c);
      rgb[2] = getb(c);

      for (i = 0; i < 3; i++) {
         cw = 4;
         cx = (slider_width - 4 - cw) * rgb[i] / 255;

         rect(demo_menu_canvas, x + 2, extra + 5,
              x + slider_width + 2, extra + h - 1, 0);
         rectfill(demo_menu_canvas, x + 3 + cx, extra + 6,
                  x + 3 + cx + cw, extra + h - 4, 0);

         rect(demo_menu_canvas, x, extra + 3, x + slider_width,
              extra + h - 3, item->extra == i ? col1 : col2);
         rectfill(demo_menu_canvas, x + 2 + cx, extra + 5,
                  x + 2 + cx + cw, extra + h - 5,
                  item->extra == i ? col1 : col2);

         x += slider_width + 4;
      }

      uszprintf(buf, sizeof(buf), "%d,%d,%d", rgb[0], rgb[1], rgb[2]);
      shadow_textprintf(demo_menu_canvas, demo_font, x + 8, extra, c, 0, buf);
   } else if (msg == DEMO_MENU_MSG_KEY) {
      c = *(int *)(item->data);

      rgb[0] = getr(c);
      rgb[1] = getg(c);
      rgb[2] = getb(c);

      switch (extra >> 8) {
         case KEY_LEFT:
            if (rgb[item->extra] > 0) {
               if (key_shifts & KB_SHIFT_FLAG) {
                  --rgb[item->extra];
               } else {
                  rgb[item->extra] -= 16;
                  rgb[item->extra] = MAX(0, rgb[item->extra]);
               }

               changed = 1;
            }
            break;

         case KEY_RIGHT:
            if (rgb[item->extra] < 255) {
               if (key_shifts & KB_SHIFT_FLAG) {
                  ++rgb[item->extra];
               } else {
                  rgb[item->extra] += 16;
                  rgb[item->extra] = MIN(255, rgb[item->extra]);
               }

               changed = 1;
            }
            break;

         case KEY_TAB:
            if (key_shifts & KB_SHIFT_FLAG) {
               --item->extra;
               if (item->extra < 0) {
                  item->extra += 3;
               }
            } else {
               ++item->extra;
               item->extra %= 3;
            }
            play_sound(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);
            if (item->on_activate) {
               item->on_activate(item);
            }
            break;
      }

      if (changed) {
         *(int *)(item->data) = makecol(rgb[0], rgb[1], rgb[2]);

         play_sound(DEMO_SAMPLE_BUTTON, 255, 128, -freq_variation, 0);

         if (item->on_activate) {
            item->on_activate(item);
         }
      }
   } else if (msg == DEMO_MENU_MSG_WIDTH) {
      return MAX(text_length(demo_font, item->name) * 2 + 8 * 2 +
                 3 * (slider_width + 4),
                 8 * 2 + 3 * (slider_width + 4) +
                 2 * text_length(demo_font, "255,255,255"));
   } else if (msg == DEMO_MENU_MSG_HEIGHT) {
      return text_height(demo_font);
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
