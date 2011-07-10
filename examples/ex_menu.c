#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/allegro_primitives.h"

enum {
   FILE_OPEN_ID = 1,
   FILE_CHECKBOX_ID,
   FILE_DISABLED_ID,
   FILE_CLOSE_ID,
   FILE_EXIT_ID,
   HELP_ABOUT_ID
};

ALLEGRO_MENU_INFO main_menu_info[] = {
   ALLEGRO_START_OF_MENU("&File", 0),
      { "&Open", FILE_OPEN_ID, 0, NULL },
      { "&Checkbox", FILE_CHECKBOX_ID, ALLEGRO_MENU_ITEM_CHECKED, NULL },
      { "&Disabled", FILE_DISABLED_ID, ALLEGRO_MENU_ITEM_DISABLED, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { "E&xit", FILE_EXIT_ID, 0, NULL },
      ALLEGRO_END_OF_MENU,

   ALLEGRO_START_OF_MENU("&Help", 0),
      { "&About", HELP_ABOUT_ID, 0, NULL },
      ALLEGRO_END_OF_MENU,
 
   ALLEGRO_END_OF_MENU
};

ALLEGRO_MENU_INFO child_menu_info[] = {
   ALLEGRO_START_OF_MENU("&File", 0),
      { "&Close", FILE_CLOSE_ID, 0, NULL },
      ALLEGRO_END_OF_MENU,
   ALLEGRO_END_OF_MENU
};

int main(void)
{
   int menu_height = 0;

   ALLEGRO_DISPLAY *display;
   ALLEGRO_MENU *menu;
   ALLEGRO_EVENT_QUEUE *queue;
   bool menu_visible = true;
   ALLEGRO_MENU *pmenu;

   al_init();
   al_init_native_dialog_addon();
   al_init_primitives_addon();
   al_install_keyboard();
   al_install_mouse();

   queue = al_create_event_queue();

   display = al_create_display(640, 480);
   menu = al_build_menu(main_menu_info);
   al_set_display_menu(display, menu);

   pmenu = al_create_popup_menu();
   if (pmenu)
      al_append_menu_item(pmenu, "E&xit", FILE_EXIT_ID, 0, NULL, NULL);

   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_default_menu_event_source());
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());

   while (true) {
      ALLEGRO_EVENT event;

      al_clear_to_color(al_map_rgb(0,0,255));
      al_draw_filled_rectangle(0,0,640,32, al_map_rgb(255,0,0));
      al_draw_filled_rectangle(0,480-32,640,480, al_map_rgb(255,0,0));
      al_draw_line(0,0, 640,480, al_map_rgb(255,255,255), 3.0);
      al_flip_display();

      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
         break;
      else if (event.type == ALLEGRO_EVENT_MENU_CLICK) {
         if (event.user.data2 == (intptr_t) display) {
            if (event.user.data1 == FILE_OPEN_ID) {
               ALLEGRO_DISPLAY *d = al_create_display(320, 240);
               if (d) {
                  ALLEGRO_MENU *menu = al_build_menu(child_menu_info);
                  al_set_display_menu(d, menu);
                  al_clear_to_color(al_map_rgb(0,0,0));
                  al_flip_display();

                  al_set_target_backbuffer(display);
               }
            }
            else if (event.user.data1 == FILE_CHECKBOX_ID) {
               al_toggle_menu_item_flags(menu, FILE_CHECKBOX_ID, ALLEGRO_MENU_ITEM_CHECKED);
               al_toggle_menu_item_flags(menu, FILE_DISABLED_ID, ALLEGRO_MENU_ITEM_DISABLED);
            }
            else if (event.user.data1 == FILE_EXIT_ID)
               break;
         }
         else {
            if (event.user.data1 == FILE_CLOSE_ID) {
               ALLEGRO_DISPLAY *d = (ALLEGRO_DISPLAY *) event.user.data2;
               if (d) {
                  ALLEGRO_MENU *menu = al_remove_display_menu(d);
                  if (menu)
                     al_destroy_menu(menu);
                  al_destroy_display(d);
               }
            }
         }
      }
      else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
         if (event.mouse.display == display && event.mouse.button == 2) {
            if (pmenu)
               al_popup_menu(pmenu, display, event.mouse.x, event.mouse.y + menu_height, 0);
         }
      }
      else if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
         if (event.keyboard.display == display) {
            if (event.keyboard.unichar == ' ') {
               if (menu_visible)
                  al_remove_display_menu(display);
               else
                  al_set_display_menu(display, menu);

               menu_visible = !menu_visible;
            }
         }
      }
      else if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
         int dh;

         /* The Windows implementation currently uses part of the client's height to
          * render the window. This triggers a resize event, which can be trapped and
          * used to upsize the window to be the size we expect it to be.
          */

         al_acknowledge_resize(display);
         dh = 480 - al_get_display_height(display);

         if (dh > 0) {
            al_resize_display(display, 640, 480 + dh);
            menu_height = dh;
         }
         else if (dh < 0) {
            al_resize_display(display, 640, 480);
            menu_height = 0;
         }
      }
   }

   return 0;
}
