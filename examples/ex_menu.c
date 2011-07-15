#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/allegro_image.h"
#include <stdio.h>

/* The following is a list of menu item ids. They can be any non-zero, positive
 * integer. A menu item must have an id in order for it to generate an event.
 * Also, each menu item's id should be unique to get well defined results. 
 */
enum {
   FILE_ID = 1,
   FILE_OPEN_ID,
   FILE_CLOSE_ID,
   FILE_EXIT_ID,
   DYNAMIC_ID,
   DYNAMIC_CHECKBOX_ID,
   DYNAMIC_DISABLED_ID,
   DYNAMIC_DELETE_ID,
   DYNAMIC_CREATE_ID,
   HELP_ABOUT_ID
};

/* This is one way to define a menu. The entire system, nested menus and all,
 * can be defined by this single array. 
 */
ALLEGRO_MENU_INFO main_menu_info[] = {
   ALLEGRO_START_OF_MENU("&File", FILE_ID),
      { "&Open", FILE_OPEN_ID, 0, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { "E&xit", FILE_EXIT_ID, 0, NULL },
      ALLEGRO_END_OF_MENU,
   
   ALLEGRO_START_OF_MENU("&Dynamic Options", DYNAMIC_ID),
      { "&Checkbox", DYNAMIC_CHECKBOX_ID, ALLEGRO_MENU_ITEM_CHECKED, NULL },
      { "&Disabled", DYNAMIC_DISABLED_ID, ALLEGRO_MENU_ITEM_DISABLED, NULL },
      { "DELETE ME!", DYNAMIC_DELETE_ID, 0, NULL },
      { "Click Me", DYNAMIC_CREATE_ID, 0, NULL },
      ALLEGRO_END_OF_MENU,

   ALLEGRO_START_OF_MENU("&Help", 0),
      { "&About", HELP_ABOUT_ID, 0, NULL },
      ALLEGRO_END_OF_MENU,
 
   ALLEGRO_END_OF_MENU
};

/* This is the menu on the secondary windows. */
ALLEGRO_MENU_INFO child_menu_info[] = {
   ALLEGRO_START_OF_MENU("&File", 0),
      { "&Close", FILE_CLOSE_ID, 0, NULL },
      ALLEGRO_END_OF_MENU,
   ALLEGRO_END_OF_MENU
};

int main(void)
{
   /* On Windows, the menu steals space from our drawable area. The menu_height
    * variable represents how much space is lost. (See the resize event.) 
    */
   int menu_height = 0;
   int dcount = 0;
   const int width = 320, height = 200;
   
   ALLEGRO_DISPLAY *display;
   ALLEGRO_MENU *menu;
   ALLEGRO_EVENT_QUEUE *queue;
   bool menu_visible = true;
   ALLEGRO_MENU *pmenu;
   ALLEGRO_BITMAP *bg;
   
   al_init();
   al_init_native_dialog_addon();
   al_init_image_addon();
   al_install_keyboard();
   al_install_mouse();

   queue = al_create_event_queue();

   display = al_create_display(width, height);
   al_set_window_title(display, "ex_menu - Main Window");
   
   menu = al_build_menu(main_menu_info);
   
   /* Add an icon to the Help/About item. Note that Allegro assumes ownership
    * of the bitmap. */
   al_set_menu_item_icon(menu, HELP_ABOUT_ID, al_load_bitmap("data/icon.tga"));
   
   if (!al_set_display_menu(display, menu)) {
      /* Since the menu could not be attached to the window, then treat it as
       * a popup menu instead. */
      pmenu = al_clone_menu_for_popup(menu);
      al_destroy_menu(menu);
      menu = pmenu;
   }
   else {
      /* Create a simple popup menu used when right clicking. */
      pmenu = al_create_popup_menu();
      if (pmenu) {
         al_append_menu_item(pmenu, "&Open", FILE_OPEN_ID, 0, NULL, NULL);
         al_append_menu_item(pmenu, "E&xit", FILE_EXIT_ID, 0, NULL, NULL);
      }
   }
   
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_default_menu_event_source());
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   
   bg = al_load_bitmap("data/mysha.pcx");

   while (true) {
      ALLEGRO_EVENT event;
      
      while (!al_wait_for_event_timed(queue, &event, 0.01)) {
         if (bg)
            al_draw_bitmap(bg, 0, 0, 0);
         al_flip_display();
      }

      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         if (event.display.source == display) {
            /* Closing the primary display */
            break;
         }
         else {
            /* Closing a secondary display */
            al_set_display_menu(event.display.source, NULL);
            al_destroy_display(event.display.source);
         }
      }
      else if (event.type == ALLEGRO_EVENT_MENU_CLICK) {
         /* data1: id
          * data2: display (could be null)
          * data3: menu    (could be null)
          */
         if (event.user.data2 == (intptr_t) display) {
            /* The main window. */
            if (event.user.data1 == FILE_OPEN_ID) {
               ALLEGRO_DISPLAY *d = al_create_display(320, 240);
               if (d) {
                  ALLEGRO_MENU *menu = al_build_menu(child_menu_info);
                  al_set_display_menu(d, menu);
                  al_clear_to_color(al_map_rgb(0,0,0));
                  al_flip_display();                  
                  al_register_event_source(queue, al_get_display_event_source(d));
                  al_set_target_backbuffer(display);
                  al_set_window_title(d, "ex_menu - Child Window");
               }
            }
            else if (event.user.data1 == DYNAMIC_CHECKBOX_ID) {
               al_toggle_menu_item_flags(menu, DYNAMIC_DISABLED_ID, ALLEGRO_MENU_ITEM_DISABLED);               
               al_set_menu_item_caption(menu, DYNAMIC_DISABLED_ID, 
                  (al_get_menu_item_flags(menu, DYNAMIC_DISABLED_ID) & ALLEGRO_MENU_ITEM_DISABLED) ?
                  "&Disabled" : "&Enabled");               
            }
            else if (event.user.data1 == DYNAMIC_DELETE_ID) {
               al_remove_menu_item(menu, DYNAMIC_DELETE_ID);
            }
            else if (event.user.data1 == DYNAMIC_CREATE_ID) {
               if (dcount < 5) {
                  char new_name[10];
                  
                  ++dcount;
                  if (dcount == 1) {
                     /* append a separator */
                     al_append_menu_item(al_find_menu(menu, DYNAMIC_ID), NULL, 0, 0, NULL, NULL);
                  }
                  
                  sprintf(new_name, "New #%d", dcount);                  
                  al_append_menu_item(al_find_menu(menu, DYNAMIC_ID), new_name, 0, 0, NULL, NULL);
                  
                  if (dcount == 5) {
                     /* disable the option */
                     al_set_menu_item_flags(menu, DYNAMIC_CREATE_ID, ALLEGRO_MENU_ITEM_DISABLED);
                  }
               }
            }
            else if (event.user.data1 == HELP_ABOUT_ID) {
               al_show_native_message_box(display, "About", "ex_menu",
                  "This is a sample program that shows how to use menus",
                  "OK", 0);
            }
            else if (event.user.data1 == FILE_EXIT_ID)
               break;
         }
         else {
            /* The child window  */
            if (event.user.data1 == FILE_CLOSE_ID) {
               ALLEGRO_DISPLAY *d = (ALLEGRO_DISPLAY *) event.user.data2;
               if (d) {
                  al_set_display_menu(d, NULL);
                  al_destroy_display(d);
               }
            }
         }
      }
      else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
         /* Popup a context menu on a right click. */
         if (event.mouse.display == display && event.mouse.button == 2) {
            if (pmenu)
               al_popup_menu(pmenu, display);
         }
      }
      else if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
         /* Toggle the menu if the spacebar is pressed */
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
         dh = height - al_get_display_height(display);

         if (dh > 0) {
            al_resize_display(display, width, height + dh);
            menu_height = dh;
         }
         else if (dh < 0) {
            al_resize_display(display, width, height);
            menu_height = 0;
         }
      }
   }
   
   /* You must remove the menu before destroying the display to free resources */
   al_set_display_menu(display, NULL);
   
   return 0;
}
