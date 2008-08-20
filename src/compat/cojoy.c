/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Old high level joystick input framework.
 *
 *      By Shawn Hargreaves.
 *
 *      Converted into an emulation layer by Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"



/* dummy driver for this emulation */
static JOYSTICK_DRIVER joystick_emu =
{
   AL_ID('J','E','M','U'),
   empty_string,
   empty_string,
   "Old joystick API emulation"
};


int _joystick_installed = FALSE;

JOYSTICK_DRIVER *joystick_driver = NULL;

int _joy_type = JOY_TYPE_NONE;

JOYSTICK_INFO joy[MAX_JOYSTICKS];
int num_joysticks = 0;

static int joy_loading = FALSE;

static ALLEGRO_JOYSTICK *new_joy[MAX_JOYSTICKS];



/* clear_joystick_vars:
 *  Resets the joystick state variables to their default values.
 */
static void clear_joystick_vars(void)
{
   AL_CONST char *unused = get_config_text("unused");
   int i, j, k;

   #define ARRAY_SIZE(a)   ((int)sizeof((a)) / (int)sizeof((a)[0]))

   for (i=0; i<ARRAY_SIZE(joy); i++) {
      joy[i].flags = 0;
      joy[i].num_sticks = 0;
      joy[i].num_buttons = 0;

      for (j=0; j<ARRAY_SIZE(joy[i].stick); j++) {
	 joy[i].stick[j].flags = 0;
	 joy[i].stick[j].num_axis = 0;
	 joy[i].stick[j].name = unused;

	 for (k=0; k<ARRAY_SIZE(joy[i].stick[j].axis); k++) {
	    joy[i].stick[j].axis[k].pos = 0;
	    joy[i].stick[j].axis[k].d1 = FALSE;
	    joy[i].stick[j].axis[k].d2 = FALSE;
	    joy[i].stick[j].axis[k].name = unused;
	 }
      }

      for (j=0; j<ARRAY_SIZE(joy[i].button); j++) {
	 joy[i].button[j].b = FALSE;
	 joy[i].button[j].name = unused;
      }
   }

   num_joysticks = 0;
}



/* fill_old_joystick_info:
 *  Fill parts of the joy[] structure for joystick number N
 *  using information from NEW_JOY.
 */
static void fill_old_joystick_info(int n, ALLEGRO_JOYSTICK *new_joy)
{
   int s, a, b;
   int new_flags;

   /* meaningless anyway */
   joy[n].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE;

   /* sticks */
   joy[n].num_sticks = al_get_num_joystick_sticks(new_joy);
   ASSERT(joy[n].num_sticks <= MAX_JOYSTICK_STICKS);

   for (s=0; s<joy[n].num_sticks; s++) {
      joy[n].stick[s].num_axis = al_get_num_joystick_axes(new_joy, s);
      ASSERT(joy[n].stick[s].num_axis <= MAX_JOYSTICK_AXIS);

      new_flags = al_get_joystick_stick_flags(new_joy, s);
      joy[n].stick[s].flags =
         ((new_flags & ALLEGRO_JOYFLAG_DIGITAL)  ? JOYFLAG_DIGITAL  : 0) |
         ((new_flags & ALLEGRO_JOYFLAG_ANALOGUE) ? JOYFLAG_ANALOGUE : 0);

      if (joy[n].stick[s].num_axis == 1)
         joy[n].stick[s].flags |= JOYFLAG_UNSIGNED;
      else
         joy[n].stick[s].flags |= JOYFLAG_SIGNED;

      joy[n].stick[s].name = al_get_joystick_stick_name(new_joy, s);
   
      for (a=0; a<joy[n].stick[s].num_axis; a++)
         joy[n].stick[s].axis[a].name = al_get_joystick_axis_name(new_joy, s, a);
   }

   /* buttons */
   joy[n].num_buttons = al_get_num_joystick_buttons(new_joy);
   ASSERT(joy[n].num_buttons <= MAX_JOYSTICK_BUTTONS);

   for (b=0; b<joy[n].num_buttons; b++)
      joy[n].button[b].name = al_get_joystick_button_name(new_joy, b);
}



/* update_calib:
 *  Updates the calibration flags for the specified joystick structure.
 */
static void update_calib(int n)
{
   int c = FALSE;
   int i;

   for (i=0; i<joy[n].num_sticks; i++) {
      if (joy[n].stick[i].flags & (JOYFLAG_CALIB_DIGITAL | JOYFLAG_CALIB_ANALOGUE)) {
	 joy[n].stick[i].flags |= JOYFLAG_CALIBRATE;
	 c = TRUE;
      }
      else
	 joy[n].stick[i].flags &= ~JOYFLAG_CALIBRATE;
   }

   if (c)
      joy[n].flags |= JOYFLAG_CALIBRATE;
   else
      joy[n].flags &= ~JOYFLAG_CALIBRATE;
}



/* install_joystick:
 *  Initialises the joystick module.
 */
int install_joystick(int type)
{
   int c;

   if (_joystick_installed)
      return 0;

   if (!al_install_joystick())
      return -1;

   clear_joystick_vars();

   usetc(allegro_error, 0);

   for (c=0; c<MAX_JOYSTICKS; c++) {
      new_joy[num_joysticks] = al_get_joystick(c);
      if (new_joy[num_joysticks]) {
         fill_old_joystick_info(num_joysticks, new_joy[num_joysticks]);
         num_joysticks++;
      }
   }

   if (num_joysticks == 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("No joysticks found"));
      al_uninstall_joystick();
      return -1;
   }

   _joy_type = joystick_emu.id;
   joystick_driver = &joystick_emu;

   poll_joystick();

   _add_exit_func(remove_joystick, "remove_joystick");
   _joystick_installed = TRUE;

   return 0;
}



/* remove_joystick:
 *  Shuts down the joystick module.
 */
void remove_joystick(void)
{
   if (_joystick_installed) {
      while (num_joysticks > 0) {
         num_joysticks--;
         al_release_joystick(new_joy[num_joysticks]);
         new_joy[num_joysticks] = NULL;
      }

      al_uninstall_joystick();

      joystick_driver = NULL;
      _joy_type = JOY_TYPE_NONE;

      clear_joystick_vars();

      _remove_exit_func(remove_joystick);
      _joystick_installed = FALSE;
   }
}



/* convert_joystick_state:
 *  Stores the information in a ALLEGRO_JOYSTICK_STATE structure into an
 *  older-style JOYSTICK_INFO structure.
 */
static void convert_joystick_state(JOYSTICK_INFO *old_info, ALLEGRO_JOYSTICK_STATE *state)
{
   int axis, i, j;

   /* axes */
   axis = 0;
   for (i=0; i < old_info->num_sticks; i++) {
      for (j=0; j < old_info->stick[i].num_axis; j++) {
         if (old_info->stick[i].flags & JOYFLAG_SIGNED)
            old_info->stick[i].axis[j].pos = state->stick[i].axis[j] * 128;
         else
            old_info->stick[i].axis[j].pos = (state->stick[i].axis[j] + 1.0) * 127.5;

         if (state->stick[i].axis[j] < -0.5) {
            old_info->stick[i].axis[j].d1 = TRUE;
            old_info->stick[i].axis[j].d2 = FALSE;
         }
         else if (state->stick[i].axis[j] > +0.5) {
            old_info->stick[i].axis[j].d1 = FALSE;
            old_info->stick[i].axis[j].d2 = TRUE;
         }
         else {
            old_info->stick[i].axis[j].d1 = FALSE;
            old_info->stick[i].axis[j].d2 = FALSE;
         }
      }
   }

   /* buttons */
   for (i = 0; i < old_info->num_buttons; i++) {
      old_info->button[i].b = state->button[i] ? TRUE : FALSE;
   }
}



/* poll_joystick:
 *  Reads the current input state into the joystick status variables.
 */
int poll_joystick()
{
   ALLEGRO_JOYSTICK_STATE state;
   int c;

   if (!_joystick_installed)
      return -1;

   for (c=0; c<num_joysticks; c++) {
      al_get_joystick_state(new_joy[c], &state);
      convert_joystick_state(&joy[c], &state);
   }

   return 0;
}



/*
 * Note: The newer joystick API doesn't deal with calibration (yet?),
 * so we cannot provide proper compatibility for the calibration and
 * related routines from the old joystick API.  However, the only
 * joystick drivers in Allegro 4.x that actually dealt with
 * calibration were written for DOS.
 */


/* save_joystick_data:
 *  After calibrating a joystick, this function can be used to save the
 *  information into the specified file, from where it can later be 
 *  restored by calling load_joystick_data().
 */
int save_joystick_data(AL_CONST char *filename)
{
   char tmp1[64], tmp2[64];

   if (filename) {
      push_config_state();
      set_config_file(filename);
   }

   set_config_id(uconvert_ascii("joystick", tmp1), uconvert_ascii("joytype", tmp2), _joy_type);

   /*
   if ((joystick_driver) && (joystick_driver->save_data))
      joystick_driver->save_data();
   */

   if (filename)
      pop_config_state();

   return 0;
}



/* load_joystick_data:
 *  Restores a set of joystick calibration data previously saved by
 *  save_joystick_data().
 */
int load_joystick_data(AL_CONST char *filename)
{
   char tmp1[64], tmp2[64];
   int ret, c;

   joy_loading = TRUE;

   if (_joystick_installed)
      remove_joystick();

   if (filename) {
      push_config_state();
      set_config_file(filename);
   }

   _joy_type = get_config_id(uconvert_ascii("joystick", tmp1), uconvert_ascii("joytype", tmp2), -1);

   if (_joy_type < 0) {
      _joy_type = JOY_TYPE_NONE;
      ret = -1;
   }
   else {
      ret = install_joystick(_joy_type);

      if (ret == 0) {
         /*
	 if (joystick_driver->load_data)
	    ret = joystick_driver->load_data();
         */
      }
      else
	 ret = -2;
   }

   if (filename)
      pop_config_state();

   if (ret == 0) {
      for (c=0; c<num_joysticks; c++)
	 update_calib(c);

      poll_joystick();
   }

   joy_loading = FALSE;

   return ret;
}



/* calibrate_joystick_name:
 *  Returns the name of the next calibration operation to be performed on
 *  the specified stick.
 *  Currently does nothing in this emulation.
 */
AL_CONST char *calibrate_joystick_name(int n)
{
   return NULL;
}



/* calibrate_joystick:
 *  Performs the next caliabration operation required for the specified stick.
 *  Currently does nothing in this emulation.
 */
int calibrate_joystick(int n)
{
   return -1;
}



/* initialise_joystick:
 *  Bodge function to preserve backward compatibility with the old API.
 */
int initialise_joystick()
{
   int type = _joy_type;

   if (_joystick_installed)
      remove_joystick();

   #ifdef JOY_TYPE_STANDARD
      if (type == JOY_TYPE_NONE)
	 type = JOY_TYPE_STANDARD;
   #endif

   return install_joystick(type);
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */

