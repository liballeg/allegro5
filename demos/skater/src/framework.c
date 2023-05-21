#include <allegro5/allegro.h>
#ifdef ALLEGRO_ANDROID
#include <allegro5/allegro_android.h>
#endif
#include "global.h"
#include "credits.h"
#include "fps.h"
#include "framework.h"
#include "game.h"
#include "gamepad.h"
#include "gamestate.h"
#include "keyboard.h"
#include "mouse.h"
#include "menus.h"
#include "screenshot.h"
#include "transition.h"

static int closed = false;

/* name of the configuration file for storing demo-specific settings. */
#define DEMO_CFG "demo.cfg"

static int timer = 0;

/* Screenshot module; for taking screenshots. */
static SCREENSHOT *screenshot = NULL;

/* Status of the F12 key; used for taking screenshots. */
static int F12 = 0;

/* Framerate counter module. */
static FPS *fps = NULL;

/*
   Static array of gamestates. Only state_count states are actually initialized.
   The current_state variable points to the currently active state and
   last_state points to the previous state. This required to do smooth
   transitions between states.
*/
static GAMESTATE state[DEMO_MAX_GAMESTATES];
static int current_state = 0, last_state;
static int state_count = 0;

ALLEGRO_EVENT_QUEUE *event_queue;

/*
   Module for performing smooth state transition animations.
*/
static TRANSITION *transition = NULL;



static void drop_build_config_dir(ALLEGRO_PATH *path)
{
   const char *s = al_get_path_tail(path);
   if (s) {
      if (0 == strcmp(s, "Debug")
         || 0 == strcmp(s, "Release")
         || 0 == strcmp(s, "RelWithDebInfo")
         || 0 == strcmp(s, "Profile"))
      {
         al_drop_path_tail(path);
      }
   }
}


int init_framework(void)
{
   int error = DEMO_OK;
   int c;
   ALLEGRO_PATH *path;

   /* Attempt to initialize Allegro. */
   if (!al_init()) {
      return DEMO_ERROR_ALLEGRO;
   }

   al_init_image_addon();
   al_init_primitives_addon();
   al_init_font_addon();

   /* Construct aboslute path for the configuration file. */
   al_set_app_name("Allegro Skater Demo");
   al_set_org_name("");
   path = al_get_standard_path(ALLEGRO_USER_SETTINGS_PATH);
   al_make_directory(al_path_cstr(path, '/'));
   al_set_path_filename(path, DEMO_CFG);
   strncpy(config_path, al_path_cstr(path, '/'), DEMO_PATH_LENGTH);
   al_destroy_path(path);

   /* Construct absolute path for the datafile containing game menu data. */
#ifdef ALLEGRO_ANDROID
   al_android_set_apk_file_interface();
   strncpy(data_path, "/data", DEMO_PATH_LENGTH);
   (void)drop_build_config_dir;
#else
   path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
   al_set_path_filename(path, "");
   drop_build_config_dir(path);
   al_append_path_component(path, "data");
   strncpy(data_path, al_path_cstr(path, '/'), DEMO_PATH_LENGTH);
   al_destroy_path(path);
#endif

   /* Read configuration file. */
   read_global_config(config_path);

   /* Set window title and install close icon callback on platforms that
      support this. */
   al_set_window_title(screen, "Allegro Demo Game");

   /* Install the Allegro sound and music submodule. Note that this function
      failing is not considered a fatal error so no error checking is
      required. If this call fails, the game will still run, but with no
      sound or music (or both). */
   al_install_audio();
   al_init_acodec_addon();
   al_reserve_samples(8);
   
   event_queue = al_create_event_queue();

   /* Attempt to set the gfx mode. */
   if ((error = change_gfx_mode()) != DEMO_OK) {
      fprintf(stderr, "Error: %s\n", demo_error(error));
      return error;
   }

   /* Attempt to install the Allegro keyboard submodule. */
   if (!al_install_keyboard()) {
      fprintf(stderr, "Error installing keyboard: %s\n",
                      demo_error(DEMO_ERROR_ALLEGRO));
      return DEMO_ERROR_ALLEGRO;
   }

   /* Attempt to install the joystick submodule. Note: no need to check
      the return value as joystick isn't really required! */
   al_install_joystick();
    
   if (al_install_touch_input())
      al_register_event_source(event_queue, al_get_touch_input_event_source());
   
   al_install_mouse();

   al_register_event_source(event_queue, al_get_keyboard_event_source());
   al_register_event_source(event_queue, al_get_mouse_event_source());
   al_register_event_source(event_queue, al_get_joystick_event_source());

   /* Create a timer. */
   {
      ALLEGRO_TIMER *t = al_create_timer(1.0 / logic_framerate);
      al_register_event_source(event_queue, al_get_timer_event_source(t));
      al_start_timer(t);
   }

   /* Seed the random number generator. */
   srand((unsigned)time(NULL));

   /* Create the screenshot module. Screenshots will be saved in TGA format
      and named SHOTxxxx.TGA. */
   screenshot = create_screenshot("SHOT", "TGA");

   /* Create the frame rate counter module. */
   fps = create_fps(logic_framerate);

   /* Initialize the game state array. */
   c = DEMO_MAX_GAMESTATES;
   while (c--) {
      state[c].deinit = state[c].init = NULL;
      state[c].id = state[c].update = NULL;
      state[c].draw = NULL;
   }

   /* Create all the game states/screens/pages. New screens may be added
      here as required. */
   create_main_menu(&state[0]);
   create_options_menu(&state[1]);
   create_gfx_menu(&state[2]);
   create_sound_menu(&state[3]);
   create_misc_menu(&state[4]);
   create_controls_menu(&state[5]);
   create_intro(&state[6]);
   create_about_menu(&state[7]);
   create_new_game(&state[8]);
   create_continue_game(&state[9]);
   create_success_menu(&state[10]);

   state_count = 11;            /* demo game has 11 screens */
   current_state = 6;           /* the game will start with screen #7 - the intro */

   /* Create the keyboard and gamepad controller modules. */
   controller[DEMO_CONTROLLER_KEYBOARD] =
      create_keyboard_controller(config_path);
   controller[DEMO_CONTROLLER_GAMEPAD] =
      create_gamepad_controller(config_path);

   /* Initialize the module for displaying credits. */
   init_credits();

   return error;
}


/*
   Draws the current state or transition animation to the backbuffer
   and flips the page.

   Parameters:
      none

   Returns:
      none
*/
static void draw_framework(void)
{

   /* Draw either the current state or the transition animation if we're
      in between states. */
   if (transition) {
      draw_transition(transition);
   } else {
      /* DEMO_STATE_EXIT is a special case and doesn't eqate to and actual
         state that can be drawn. */
      if (current_state != DEMO_STATE_EXIT) {
         state[current_state].draw();
      }
   }

   /* Draw the current framerate if required. */
   if (display_framerate) {
      draw_fps(fps, plain_font, screen_width, 0, al_map_rgb(255, 255, 255), "%d FPS");
   }

   al_flip_display();
}


void run_framework(void)
{
   int done = false;            /* will become true when we're ready to close */
   int need_to_redraw = 1;      /* do we need to draw the next frame? */
   int next_state = current_state;
   int i;
   bool background_mode = false;
   bool paused = false;

   /* Initialize the first game screen. */
   state[current_state].init();

   /* Create a transition animation so that the first screen doesn't just
      pop up but comes in a nice animation that lasts 0.3 seconds. */
   transition = create_transition(NULL, &state[current_state], 0.3f);

   timer = 0;

   /* Do the main loop; until we're not done. */
   while (!done) {

      ALLEGRO_EVENT event;

      al_wait_for_event(event_queue, &event);
      switch (event.type) {
         
         case ALLEGRO_EVENT_MOUSE_AXES:
            mouse_handle_event(&event);
            break;
         
         case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            mouse_handle_event(&event);
            break;
         
         case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            mouse_handle_event(&event);
            break;

         case ALLEGRO_EVENT_KEY_DOWN:
            keyboard_event(&event);
            break;

         case ALLEGRO_EVENT_KEY_CHAR:
            keyboard_event(&event);
            break;

         case ALLEGRO_EVENT_KEY_UP:
            keyboard_event(&event);
            break;

         case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN:
            gamepad_event(&event);
            break;

         case ALLEGRO_EVENT_JOYSTICK_BUTTON_UP:
            gamepad_event(&event);
            break;

         case ALLEGRO_EVENT_JOYSTICK_AXIS:
            gamepad_event(&event);
            break;
        
         case ALLEGRO_EVENT_TOUCH_BEGIN:
            gamepad_event(&event);
            break;
         case ALLEGRO_EVENT_TOUCH_END:
              gamepad_event(&event);
            break;

         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            closed = true;
            break;
        
         case ALLEGRO_EVENT_TIMER:
            if (!paused)
               timer++;
            break;
         
         case ALLEGRO_EVENT_DISPLAY_ORIENTATION:
            if (event.display.orientation == ALLEGRO_DISPLAY_ORIENTATION_90_DEGREES ||
                event.display.orientation == ALLEGRO_DISPLAY_ORIENTATION_270_DEGREES)
               screen_orientation = event.display.orientation;
            break;
         
         case ALLEGRO_EVENT_DISPLAY_RESIZE:
            al_acknowledge_resize(screen);
            screen_width = al_get_display_width(screen);
            screen_height = al_get_display_height(screen);
            if (fullscreen == 0) {
               window_width = screen_width;
               window_height = screen_height;
            }
            break;
         
         case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
            background_mode = true;
            al_acknowledge_drawing_halt(screen);
            break;
         
         case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
            background_mode = false;
            al_acknowledge_drawing_resume(screen);
            break;
            
         case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
            paused = true;
            break;
            
         case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
            paused = false;
            break;
            
      }
      
      if (!al_is_event_queue_empty(event_queue)) continue;

      /* Check if the timer has ticked. */
      while (timer > 0) {
         --timer;

         /* See if the user pressed F12 to see if the user wants to take
            a screenshot. */
         if (key_pressed(ALLEGRO_KEY_F12)) {
            /* See if the F12 key was already pressed before. */
            if (F12 == 0) {
               /* The user just pressed F12 (it wasn't pressed before), so
                  remember this and take a screenshot! */
               F12 = 1;
               take_screenshot(screenshot);
            }
         } else {
            /* Remember for later that F12 is not pressed. */
            F12 = 0;
         }

         /* Do one frame of logic. If we're in between states, then update
            the transition animation module, otherwise update the current
            game state. */
         if (transition) {
            /* Advance the transition animation. If it returns non-zero, it
               means the animation finished playing. */
            if (update_transition(transition) == 0) {
               /* Destroy the animation, we're done with it. */
               destroy_transition(transition);
               transition = NULL;

               /* Complete the transition to the new state by deiniting the
                  previous one. */
               if (state[last_state].deinit) {
                  state[last_state].deinit();
               }

               /* Stop the main loop if there is no more game screens. */
               if (current_state == DEMO_STATE_EXIT) {
                  done = true;
                  break;
               }
            }
         }
         /* We're not in between states, so update the current one. */
         else {
            /* Update the current state. It returns the ID of the
               next state. */

            next_state = state[current_state].update();

            /* Did the current state just close the game? */
            if (next_state == DEMO_STATE_EXIT) {
               /* Create a transition so the game doesn't just close but
                  instead goes out in a nice animation. */
               transition =
                  create_transition(&state[current_state], NULL, 0.3f);
               last_state = current_state;
               current_state = next_state;
               break;
            }
            /* If the next state is different then the current one, then
               start a transition. */
            else if (next_state != state[current_state].id()) {
               last_state = current_state;

               /* Find the index of the next state in the state array.
                  Note that ID of a state is not the same as its index in
                  the array! */
               for (i = 0; i < state_count; i++) {
                  /* Did we find it? */
                  if (state[i].id() == next_state) {
                     /* We found the new state. Initialize it and start the
                        transition animation. */
                     state[i].init();
                     transition =
                        create_transition(&state[current_state], &state[i],
                                          0.3f);
                     current_state = i;
                     break;
                  }
               }
            }
         }

         /* We just did one logic frame so we assume we will need to update
            the visuals to reflect the changes this logic frame made. */
         need_to_redraw = 1;

         /* Let the framerate counter know that one logic frame was run. */
         fps_tick(fps);

         keyboard_tick();
         mouse_tick();
      }

      /* In case a frame of logic has just been run or the user wants
         unlimited framerate, we must redraw the screen. */
      if (need_to_redraw == 1 && !background_mode) {
         /* Actually do the drawing. */
         draw_framework();

         /* Make sure we don't draw too many times. */
         need_to_redraw = 0;

         /* Let the framerate counter know that we just drew a frame. */
         fps_frame(fps);
      }

      /* Check if the user pressed the close icon. */
      done = done || closed;

   }
}


void shutdown_framework(void)
{
   al_destroy_event_queue(event_queue);
   
   /* Save the configuration settings. */
   write_global_config(config_path);

   /* Destroy the screenshot module. */
   destroy_screenshot(screenshot);

   /* Destroy the framerate counter module. */
   destroy_fps(fps);

   /* Destroy controller modules. */
   destroy_vcontroller(controller[DEMO_CONTROLLER_KEYBOARD], config_path);
   destroy_vcontroller(controller[DEMO_CONTROLLER_GAMEPAD], config_path);

   /* Destroy the game states/screens. */
   destroy_game();

   /* Get rid of all the game data. */
   unload_data();

   /* Destroy the credits module. */
   destroy_credits();

   /* Destroy the transition module. */
   if (transition) {
      destroy_transition(transition);
   }
}
