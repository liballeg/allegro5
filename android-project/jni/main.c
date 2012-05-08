#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <android/log.h>
#include <allegro5/platform/alandroid.h>
#include <allegro5/internal/aintern_display.h>

ALLEGRO_DEBUG_CHANNEL("main")

struct touch {
   bool down;
   double x, y;
} touch[10];

#define paste(a,b) a##b

#define print_standard_path(std) do {\
   ALLEGRO_PATH *path = al_get_standard_path(std); \
   ALLEGRO_DEBUG(#std ": %s", al_path_cstr(path, '/')); \
} while(0)

void print_standard_paths()
{
   print_standard_path(ALLEGRO_RESOURCES_PATH);
   print_standard_path(ALLEGRO_TEMP_PATH);
   print_standard_path(ALLEGRO_USER_DATA_PATH);
   print_standard_path(ALLEGRO_USER_HOME_PATH);
   print_standard_path(ALLEGRO_USER_SETTINGS_PATH);
   print_standard_path(ALLEGRO_USER_DOCUMENTS_PATH);
   print_standard_path(ALLEGRO_EXENAME_PATH);
}

void set_transform(ALLEGRO_DISPLAY *dpy)
{
   int w = al_get_display_width(dpy);
   int h = al_get_display_height(dpy);
   
   glViewport(0, 0, w, h);
   
   ALLEGRO_TRANSFORM t;
   al_identity_transform(&t);
   al_ortho_transform(&t, 0, w, h, 0, -1, 1);
   al_set_projection_transform(dpy, &t);
}

int main(int argc, char **argv)
{
   ALLEGRO_EVENT_QUEUE *queue = NULL;
   ALLEGRO_EVENT event;
   ALLEGRO_TIMER *timer = NULL;
   ALLEGRO_BITMAP *image = NULL;
   
   ALLEGRO_DEBUG("init allegro!");
	al_init();
   ALLEGRO_DEBUG("init primitives");
   al_init_primitives_addon();
   ALLEGRO_DEBUG("init image addon");
   al_init_image_addon();
   ALLEGRO_DEBUG("init touch input");
   al_install_touch_input();
   ALLEGRO_DEBUG("init keyboard");
   al_install_keyboard();
   
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_touch_input_event_source());
   
	ALLEGRO_DEBUG("creating display");
   ALLEGRO_DISPLAY *dpy = al_create_display(800, 480);
   if(!dpy) {
      ALLEGRO_ERROR("failed to create display!");
      return -1;
   }

   /* This is loaded from assets in the apk */
   al_set_apk_file_interface();
   image = al_load_bitmap("alexlogo.png");
   if(!image) {
      ALLEGRO_DEBUG("failed to load alexlogo.png");
   }
   al_set_standard_file_interface();
   
   al_convert_mask_to_alpha(image, al_map_rgb(255,0,255));
   
   al_register_event_source(queue, al_get_display_event_source(dpy));
   al_register_event_source(queue, al_get_keyboard_event_source());
   
   timer = al_create_timer(1/60.0);
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);
   
   double touches[4][2] = { };
   
   bool draw = true;
   bool running = true;
   bool paused = false;
   int count = 0;
   

   print_standard_paths();
   
//   ALLEGRO_DEBUG("try open");
//   ALLEGRO_PATH *path = al_get_standard_path(
//   FILE *fh = fopen("
   
   while(running) {
      al_wait_for_event(queue, &event);
      {
         switch(event.type) {
            case ALLEGRO_EVENT_TOUCH_BEGIN:
               //ALLEGRO_DEBUG("touch %i begin", event.touch.id);
               touch[event.touch.id].down = true;
               touch[event.touch.id].x = event.touch.x;
               touch[event.touch.id].y = event.touch.y;
               break;
               
            case ALLEGRO_EVENT_TOUCH_END:
               //ALLEGRO_DEBUG("touch %i end", event.touch.id);
               touch[event.touch.id].down = false;
               touch[event.touch.id].x = 0.0;
               touch[event.touch.id].y = 0.0;
               break;
               
            case ALLEGRO_EVENT_TOUCH_MOVE:
               //ALLEGRO_DEBUG("touch %i move: %fx%f", event.touch.id, event.touch.x, event.touch.y);
               touch[event.touch.id].x = event.touch.x;
               touch[event.touch.id].y = event.touch.y;
               break;
               
            case ALLEGRO_EVENT_TOUCH_CANCEL:
               //ALLEGRO_DEBUG("touch %i canceled", event.touch.id);
               break;
               
            case ALLEGRO_EVENT_KEY_UP:
               if(event.keyboard.keycode == ALLEGRO_KEY_BACK) {
                  ALLEGRO_DEBUG("back key pressed, exit!");
                  running = false;
               }
               else {
                  ALLEGRO_DEBUG("%i key pressed", event.keyboard.keycode);
               }
               break;
               
            case ALLEGRO_EVENT_TIMER:
               draw = true;
               if(count == 60) {
                  ALLEGRO_DEBUG("tick");
                  count = 0;
               }
               count++;
               break;
               
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
               ALLEGRO_DEBUG("display close");
               running = false;
               break;
               
            case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
               ALLEGRO_DEBUG("switch out!");
               // stop the timer so we don't run at all while our display isn't active
               al_stop_timer(timer);
               //al_set_target_backbuffer(0);
               ALLEGRO_DEBUG("after set target");
               paused = true;
               draw = false;
	       al_acknowledge_drawing_halt(dpy);
               break;
               
            case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
               ALLEGRO_DEBUG("switch in!");

               al_acknowledge_drawing_resume(dpy);
               ALLEGRO_DEBUG("done waiting for surface recreated");
               
               al_start_timer(timer);
               //al_set_target_backbuffer(dpy);
               //_al_android_setup_opengl_view(dpy);
               paused = false;
               break;
               
            case ALLEGRO_EVENT_DISPLAY_RESIZE:
               ALLEGRO_DEBUG("display resize");
               al_acknowledge_resize(dpy);
               ALLEGRO_DEBUG("done resize");
	       set_transform(dpy);
               break;

	    case ALLEGRO_EVENT_DISPLAY_ORIENTATION: {
	       set_transform(dpy);
               break;
	    }
         }
      }
      
      if(draw && al_event_queue_is_empty(queue)) {
         draw = false;
         
         al_clear_to_color(al_map_rgb(255,255,255));
      
         if(image) {
            al_draw_bitmap(image,
                           al_get_display_width(dpy)/2 - al_get_bitmap_width(image)/2,
                           al_get_display_height(dpy)/2 - al_get_bitmap_height(image)/2, 0);
         }
         
         int i;
         for(i=0; i<10; i++) {
            if(touch[i].down) {
               al_draw_filled_rectangle(touch[i].x-40, touch[i].y-40, touch[i].x+40, touch[i].y+40, al_map_rgb(100+i*20, 40+i*20, 40+i*20));
            }
         }

         al_flip_display();  
      }
   }
   
   ALLEGRO_DEBUG("done");
	return 0;
}
