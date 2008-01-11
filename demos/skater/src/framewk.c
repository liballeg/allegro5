#include <allegro.h>
#ifdef DEMO_USE_ALLEGRO_GL
#include <alleggl.h>
#endif
#include "../include/credits.h"
#include "../include/fps.h"
#include "../include/framewk.h"
#include "../include/game.h"
#include "../include/gamepad.h"
#include "../include/global.h"
#include "../include/gmestate.h"
#include "../include/keyboard.h"
#include "../include/menus.h"
#include "../include/scrshot.h"
#include "../include/transitn.h"
#include "../include/updtedvr.h"

/*
   The callback function for the close icon on platforms that support it.
   The value of the close variable is regularly tested in the main loop
   to determine if the user clicked the close icon.
*/
static int closed = FALSE;
static void closehook(void)
{
   closed = TRUE;
}

/* name of the configuration file for storing demo-specific settings. */
#define DEMO_CFG "demo.cfg"

/*
   Timer callback. Installed with the frequency defined in the framework
   configuration file. The value of the timer variable is regularly tested
   in the main loop to determine when to run a frame of game logic.
*/
static volatile int timer = 0;
static void timer_f(void)
{
   timer++;
} END_OF_STATIC_FUNCTION(timer_f);

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

/*
   Module for performing smooth state transition animations.
*/
static TRANSITION *transition = NULL;


int init_framework(void)
{
   int error = DEMO_OK;
   int c;

   /* Attempt to initialize Allegro. */
   if (allegro_init() != 0) {
      return DEMO_ERROR_ALLEGRO;
   }

#ifdef DEMO_USE_ALLEGRO_GL
   /* Attempt to initialize AllegroGL. */
   if (install_allegro_gl() != 0) {
      return DEMO_ERROR_ALLEGRO;
   }
#endif

   /* Construct aboslute path for the configuration file. */
   get_executable_name(config_path, DEMO_PATH_LENGTH);
   replace_filename(config_path, config_path, DEMO_CFG, DEMO_PATH_LENGTH);

   /* Construct aboslute path for the datafile containing game menu data. */
   get_executable_name(data_path, DEMO_PATH_LENGTH);
   replace_filename(data_path, data_path, "demo.dat#menu.dat",
                    DEMO_PATH_LENGTH);

   /* Read configuration file. */
   read_config(config_path);

   /* Set window title and install close icon callback on platforms that
      support this. */
   set_window_title("Allegro Demo Game");
   set_close_button_callback(&closehook);

   /* Make sure the game will continue to run in the background when the
      user tabs away. Note: this should be made configurable! */
   set_display_switch_mode(SWITCH_BACKGROUND);

   /* Install the Allegro sound and music submodule. Note that this function
      failing is not considered a fatal error so no error checking is
      required. If this call fails, the game will still run, but with no
      sound or music (or both). */
   install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL);
   set_volume(sound_volume * 25, music_volume * 25);

   /* Attempt to set the gfx mode. */
   if ((error = change_gfx_mode()) != DEMO_OK) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error: %s\n", demo_error(error));
      return error;
   }

   /* Attempt to install the Allegro keyboard submodule. */
   if (install_keyboard() != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error installing keyboard: %s\n",
                      demo_error(DEMO_ERROR_ALLEGRO));
      return DEMO_ERROR_ALLEGRO;
   }

   /* Attempt to install the joystick submodule. Note: no need to check
      the return value as joystick isn't really required! */
   install_joystick(JOY_TYPE_AUTODETECT);

   /* Install the timer submodule and install the timer interrupt callback. */
   install_timer();
   LOCK_VARIABLE(timer);
   LOCK_FUNCTION(timer_f);
   install_int_ex(timer_f, BPS_TO_TIMER(logic_framerate));

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
void draw_framework(void)
{
   /* Before drawing to the backbuffer we must acquire it - just in case. */
   acquire_bitmap(update_driver.get_canvas());

   /* Draw either the current state or the transition animation if we're
      in between states. */
   if (transition) {
      draw_transition(update_driver.get_canvas(), transition);
   } else {
      /* DEMO_STATE_EXIT is a special case and doesn't eqate to and actual
         state that can be drawn. */
      if (current_state != DEMO_STATE_EXIT) {
         state[current_state].draw(update_driver.get_canvas());
      }
   }

   /* Draw the current framerate if required. */
   if (display_framerate) {
      draw_fps(fps, update_driver.get_canvas(), font, 0, 0,
               makecol(255, 255, 255), "%d FPS");
   }

   /* Release the previously acquired canvas bitmap (backbuffer). */
   release_bitmap(update_driver.get_canvas());

   /* Finally make the update driver flip the page so it becomes visible on
      the screen, however it may go about actually doing it. */
   update_driver.draw();
}


void run_framework(void)
{
   int done = FALSE;            /* will become TRUE when we're ready to close */
   int need_to_redraw = 1;      /* do we need to draw the next frame? */
   int next_state = current_state;
   int i;

   /* Initialize the first game screen. */
   state[current_state].init();

   /* Create a transition animation so that the first screen doesn't just
      pop up but comes in a nice animation that lasts 0.3 seconds. */
   transition = create_transition(NULL, &state[current_state], 0.3f);

   /* Initialize the timer variable - just in case. */
   timer = 0;

   /* Do the main loop; until we're not done. */
   while (!done) {
      /* Check if the timer has ticked. */
      while (timer > 0) {
         --timer;

         /* See if the user pressed F12 to see if the user wants to take
            a screenshot. */
         if (key[KEY_F12]) {
            /* See if the F12 key was already pressed before. */
            if (F12 == 0) {
               /* The user just pressed F12 (it wasn't pressed before), so
                  remember this and take a screenshot! */
               F12 = 1;
               take_screenshot(screenshot, update_driver.get_canvas());
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
                  done = TRUE;
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
      }

      /* Make sure the game doesn't freeze in case the computer isn't fast
         enough to run the logic as fast as we want it to. Note that this
         will skip logic frames so all time in the game will be stretched.
         Note also that this is very unlikely to happen. */
      while (timer > max_frame_skip) {
         fps_tick(fps);
         --timer;
      }

      /* In case a frame of logic has just been run or the user wants
         unlimited framerate, we must redraw the screen. */
      if (need_to_redraw == 1 || limit_framerate == 0) {
         /* Actually do the drawing. */
         draw_framework();

         /* Make sure we don't draw too many times. */
         need_to_redraw = 0;

         /* Let the framerate counter know that we just drew a frame. */
         fps_frame(fps);
      }

      /* Check if the user pressed the close icon. */
      done = done || closed;

      /* If power saving is enabled, let go of the CPU until we need it
         again. This is done by calling rest() and passing it 1. */
      if (reduce_cpu_usage) {
         while (timer == 0) {
            rest(1);
         }
      }
   }
}


void shutdown_framework(void)
{
   /* Uninstall the time callback function. */
   remove_int(timer_f);

   /* Save the configuration settings. */
   write_config(config_path);

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

   /* Destroy the screen update driver. */
   update_driver.destroy();
}
