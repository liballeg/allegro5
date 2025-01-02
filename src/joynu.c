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
 *      New joystick API.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Joystick routines
 */


#define ALLEGRO_NO_COMPATIBILITY

#include <stdio.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_system.h"


ALLEGRO_DEBUG_CHANNEL("joystick");


/* the active joystick driver */
static ALLEGRO_JOYSTICK_DRIVER *new_joystick_driver = NULL;
static ALLEGRO_EVENT_SOURCE es;
static _AL_VECTOR joystick_mappings = _AL_VECTOR_INITIALIZER(_AL_JOYSTICK_MAPPING);


static void destroy_joystick_mapping(_AL_JOYSTICK_MAPPING *mapping);



static bool compat_5_2_10(void) {
   /* Mappings. */
   return _al_get_joystick_compat_version() < AL_ID(5, 2, 11, 0);
}



/* Function: al_install_joystick
 */
bool al_install_joystick(void)
{
   ALLEGRO_SYSTEM *sysdrv;
   ALLEGRO_JOYSTICK_DRIVER *joydrv;

   if (new_joystick_driver)
      return true;

   sysdrv = al_get_system_driver();
   ASSERT(sysdrv);

   /* Currently every platform only has at most one joystick driver. */
   if (sysdrv->vt->get_joystick_driver) {
      joydrv = sysdrv->vt->get_joystick_driver();
      /* Avoid race condition in case the joystick driver generates an
       * event right after ->init_joystick.
       */
      _al_event_source_init(&es);
      if (joydrv && joydrv->init_joystick()) {
         new_joystick_driver = joydrv;
         _al_add_exit_func(al_uninstall_joystick, "al_uninstall_joystick");
         return true;
      }
      _al_event_source_free(&es);
   }

   return false;
}



/* Function: al_uninstall_joystick
 */
void al_uninstall_joystick(void)
{
   if (new_joystick_driver) {
      /* perform driver clean up */
      new_joystick_driver->exit_joystick();
      _al_event_source_free(&es);
      new_joystick_driver = NULL;
   }
   for (int i = 0; i < (int)_al_vector_size(&joystick_mappings); i++) {
      destroy_joystick_mapping(_al_vector_ref(&joystick_mappings, i));
   }
   _al_vector_free(&joystick_mappings);
}



/* Function: al_is_joystick_installed
 */
bool al_is_joystick_installed(void)
{
   return (new_joystick_driver) ? true : false;
}



/* Function: al_reconfigure_joysticks
 */
bool al_reconfigure_joysticks(void)
{
   if (!new_joystick_driver)
      return false;

   /* XXX only until Windows and Mac joystick drivers are updated */
   if (!new_joystick_driver->reconfigure_joysticks) {
      new_joystick_driver->num_joysticks();
      return true;
   }

   return new_joystick_driver->reconfigure_joysticks();
}



/* Function: al_get_joystick_event_source
 */
ALLEGRO_EVENT_SOURCE *al_get_joystick_event_source(void)
{
   if (!new_joystick_driver)
      return NULL;
   return &es;
}



void _al_generate_joystick_event(ALLEGRO_EVENT *event)
{
   ASSERT(new_joystick_driver);

   _al_event_source_lock(&es);
   if (_al_event_source_needs_to_generate_event(&es)) {
      _al_event_source_emit_event(&es, event);
   }
   _al_event_source_unlock(&es);
}



/* Function: al_get_num_joysticks
 */
int al_get_num_joysticks(void)
{
   if (new_joystick_driver)
      return new_joystick_driver->num_joysticks();

   return 0;
}



/* Function: al_get_joystick
 */
ALLEGRO_JOYSTICK * al_get_joystick(int num)
{
   ASSERT(new_joystick_driver);
   ASSERT(num >= 0);

   return new_joystick_driver->get_joystick(num);
}



/* Function: al_release_joystick
 */
void al_release_joystick(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(new_joystick_driver);
   ASSERT(joy);

   new_joystick_driver->release_joystick(joy);
}



/* Function: al_get_joystick_active
 */
bool al_get_joystick_active(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(joy);

   return new_joystick_driver->get_active(joy);
}



/* Function: al_get_joystick_name
 */
const char *al_get_joystick_name(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(joy);

   return new_joystick_driver->get_name(joy);
}



/* Function: al_get_joystick_num_sticks
 */
int al_get_joystick_num_sticks(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(joy);

   return joy->info.num_sticks;
}



/* Function: al_get_joystick_stick_flags
 */
int al_get_joystick_stick_flags(ALLEGRO_JOYSTICK *joy, int stick)
{
   ASSERT(joy);
   ASSERT(stick >= 0);

   if (stick < joy->info.num_sticks)
      return joy->info.stick[stick].flags;

   return 0;
}



/* Function: al_get_joystick_stick_name
 */
const char *al_get_joystick_stick_name(ALLEGRO_JOYSTICK *joy, int stick)
{
   ASSERT(joy);
   ASSERT(stick >= 0);

   if (stick < joy->info.num_sticks)
      return joy->info.stick[stick].name;

   return NULL;
}



/* Function: al_get_joystick_num_axes
 */
int al_get_joystick_num_axes(ALLEGRO_JOYSTICK *joy, int stick)
{
   ASSERT(joy);

   if (stick < joy->info.num_sticks)
      return joy->info.stick[stick].num_axes;

   return 0;
}



/* Function: al_get_joystick_axis_name
 */
const char *al_get_joystick_axis_name(ALLEGRO_JOYSTICK *joy, int stick, int axis)
{
   ASSERT(joy);
   ASSERT(stick >= 0);
   ASSERT(axis >= 0);

   if (stick < joy->info.num_sticks)
      if (axis < joy->info.stick[stick].num_axes)
         return joy->info.stick[stick].axis[axis].name;

   return NULL;
}



/* Function: al_get_joystick_num_buttons
 */
int al_get_joystick_num_buttons(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(joy);

   return joy->info.num_buttons;
}



/* Function: al_get_joystick_button_name
 */
const char *al_get_joystick_button_name(ALLEGRO_JOYSTICK *joy, int button)
{
   ASSERT(joy);
   ASSERT(button >= 0);

   if (button < joy->info.num_buttons)
      return joy->info.button[button].name;

   return NULL;
}



/* Function: al_get_joystick_state
 */
void al_get_joystick_state(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *ret_state)
{
   ASSERT(new_joystick_driver);
   ASSERT(joy);
   ASSERT(ret_state);

   new_joystick_driver->get_joystick_state(joy, ret_state);
}



/* Fills the gamepad info with standard gamepad buttons and sticks.
 */
void _al_fill_gamepad_info(_AL_JOYSTICK_INFO *info)
{
   ASSERT(info);
   info->type = ALLEGRO_JOYSTICK_TYPE_GAMEPAD;

   info->button[0].name = _al_strdup("A");
   info->button[1].name = _al_strdup("B");
   info->button[2].name = _al_strdup("X");
   info->button[3].name = _al_strdup("Y");
   info->button[4].name = _al_strdup("Left Shoulder");
   info->button[5].name = _al_strdup("Right Shoulder");
   info->button[6].name = _al_strdup("Back");
   info->button[7].name = _al_strdup("Start");
   info->button[8].name = _al_strdup("Guide");
   info->button[9].name = _al_strdup("Left Thumb");
   info->button[10].name = _al_strdup("Right Thumb");
   info->num_buttons = 11;

   info->stick[0].axis[0].name = _al_strdup("X");
   info->stick[0].axis[1].name = _al_strdup("Y");
   info->stick[0].name = _al_strdup("DPad");
   info->stick[0].num_axes = 2;
   info->stick[0].flags |= ALLEGRO_JOYFLAG_DIGITAL;

   info->stick[1].axis[0].name = _al_strdup("X");
   info->stick[1].axis[1].name = _al_strdup("Y");
   info->stick[1].name = _al_strdup("Left Thumb");
   info->stick[1].num_axes = 2;
   info->stick[1].flags |= ALLEGRO_JOYFLAG_ANALOGUE;

   info->stick[2].axis[0].name = _al_strdup("X");
   info->stick[2].axis[1].name = _al_strdup("Y");
   info->stick[2].name = _al_strdup("Right Thumb");
   info->stick[2].num_axes = 2;
   info->stick[2].flags |= ALLEGRO_JOYFLAG_ANALOGUE;

   info->stick[3].axis[0].name = _al_strdup("Z");
   info->stick[3].name = _al_strdup("Left Trigger");
   info->stick[3].num_axes = 1;
   info->stick[3].flags |= ALLEGRO_JOYFLAG_ANALOGUE;

   info->stick[4].axis[0].name = _al_strdup("Z");
   info->stick[4].name = _al_strdup("Right Trigger");
   info->stick[4].num_axes = 1;
   info->stick[4].flags |= ALLEGRO_JOYFLAG_ANALOGUE;

   info->num_sticks = 5;
}



/* Destroys the joystick info.
 */
void _al_destroy_joystick_info(_AL_JOYSTICK_INFO *info)
{
   for (int i = 0; i < info->num_sticks; i++) {
      al_free(info->stick[i].name);
      for (int j = 0; j < info->stick[i].num_axes; j++)
         al_free(info->stick[i].axis[j].name);
   }
   for (int i = 0; i < info->num_buttons; i++)
      al_free(info->button[i].name);
   info->num_sticks = 0;
   info->num_buttons = 0;
}



#ifdef DEBUG_JOYSTICK
static void print_output(_AL_JOYSTICK_OUTPUT out)
{
   printf("Output: ");
   if (out.button_enabled) {
      printf("button=%d, ", out.button);
   }
   if (out.pos_enabled) {
      printf("pos_stick=%d, pos_axis=%d, pos_min=%f, pos_max=%f, ", out.pos_stick, out.pos_axis, out.pos_min, out.pos_max);
   }
   if (out.neg_enabled) {
      printf("neg_stick=%d, neg_axis=%d, neg_min=%f, neg_max=%f, ", out.neg_stick, out.neg_axis, out.neg_min, out.neg_max);
   }
   printf("\n");
}



static void print_mapping(_AL_JOYSTICK_MAPPING *mapping)
{
   char guid_str[33];
   guid_str[32] = '\0';
   _al_joystick_guid_to_str(mapping->guid, guid_str);
   printf("guid: %s\n", guid_str);

   for (int i = 0; i < (int)_al_vector_size(&mapping->button_map); i++) {
      _AL_JOYSTICK_OUTPUT *o = _al_vector_ref(&mapping->button_map, i);
      if (o->button_enabled || o->pos_enabled || o->neg_enabled) {
         printf("button %d\n", o->in_idx);
         print_output(*o);
      }
   }
   for (int i = 0; i < (int)_al_vector_size(&mapping->axis_map); i++) {
      _AL_JOYSTICK_OUTPUT *o = _al_vector_ref(&mapping->axis_map, i);
      if (o->button_enabled || o->pos_enabled || o->neg_enabled) {
         printf("axis %d\n", o->in_idx);
         print_output(*o);
      }
   }
   for (int i = 0; i < (int)_al_vector_size(&mapping->hat_map); i++) {
      _AL_JOYSTICK_OUTPUT *o = _al_vector_ref(&mapping->hat_map, i);
      if (o->button_enabled || o->pos_enabled || o->neg_enabled) {
         printf("hat %d\n", o->in_idx);
         print_output(*o);
      }
   }
   printf("====================\n");
}
#endif



/* Attempts to find an output in a mapping given the occurence index of the
 * input type.
 */
_AL_JOYSTICK_OUTPUT *_al_get_joystick_output(const _AL_VECTOR *map, int in_idx)
{
   for (int i = 0; i < (int)_al_vector_size(map); i++) {
      _AL_JOYSTICK_OUTPUT *o = _al_vector_ref(map, i);
      if (o->in_idx == in_idx) {
         return o;
      }
   }
   return NULL;
}



static void init_joystick_mapping(_AL_JOYSTICK_MAPPING *mapping)
{
   memset(mapping, 0, sizeof(_AL_JOYSTICK_MAPPING));
   _al_vector_init(&mapping->hat_map, sizeof(_AL_JOYSTICK_OUTPUT));
   _al_vector_init(&mapping->axis_map, sizeof(_AL_JOYSTICK_OUTPUT));
   _al_vector_init(&mapping->button_map, sizeof(_AL_JOYSTICK_OUTPUT));
}



static void destroy_joystick_mapping(_AL_JOYSTICK_MAPPING *mapping)
{
   _al_vector_free(&mapping->hat_map);
   _al_vector_free(&mapping->axis_map);
   _al_vector_free(&mapping->button_map);
}



/* Adjusts the output min/max if the output is a trigger. */
static void trigger_transform(_AL_JOYSTICK_OUTPUT *output, bool is_trigger)
{
   if (!is_trigger)
      return;

#define TRANSFORM(f) do { f = (f) / 2. + 0.5; } while (0)

   if (output->pos_enabled) {
      TRANSFORM(output->pos_min);
      TRANSFORM(output->pos_max);
   }
   if (output->neg_enabled) {
      TRANSFORM(output->neg_min);
      TRANSFORM(output->neg_max);
   }

#undef TRANSFORM
}



static _AL_JOYSTICK_OUTPUT *get_or_insert_output(_AL_VECTOR *vec, int in_idx)
{
   _AL_JOYSTICK_OUTPUT *output = _al_get_joystick_output(vec, in_idx);
   if (output)
      return output;
   output = _al_vector_alloc_back(vec);
   if (!output)
      return NULL;
   memset(output, 0, sizeof(_AL_JOYSTICK_OUTPUT));
   output->in_idx = in_idx;
   return output;
}



/* Possible fields in an SDL-style mapping.
 */
typedef struct _FIELD {
   const char* name;  /* Name of the output. */
   int button;        /* Output button. */
   int stick;         /* Output stick. */
   int axis;          /* Output axis. */
   int out_sign;      /* Output sign. */
   bool is_trigger;   /* Whether the output axis is a trigger. */
} _FIELD;



static bool parse_sdl_joystick_mapping(const char *str, _AL_JOYSTICK_MAPPING *mapping)
{
   init_joystick_mapping(mapping);

   _FIELD fields[] = {
      {"a:", ALLEGRO_GAMEPAD_BUTTON_A, 0, 0, 0, false},
      {"b:", ALLEGRO_GAMEPAD_BUTTON_B, 0, 0, 0, false},
      {"x:", ALLEGRO_GAMEPAD_BUTTON_X, 0, 0, 0, false},
      {"y:", ALLEGRO_GAMEPAD_BUTTON_Y, 0, 0, 0, false},
      {"leftshoulder:", ALLEGRO_GAMEPAD_BUTTON_LEFT_SHOULDER, 0, 0, 0, false},
      {"rightshoulder:", ALLEGRO_GAMEPAD_BUTTON_RIGHT_SHOULDER, 0, 0, 0, false},
      {"back:", ALLEGRO_GAMEPAD_BUTTON_BACK, 0, 0, 0, false},
      {"start:", ALLEGRO_GAMEPAD_BUTTON_START, 0, 0, 0, false},
      {"guide:", ALLEGRO_GAMEPAD_BUTTON_GUIDE, 0, 0, 0, false},
      {"leftstick:", ALLEGRO_GAMEPAD_BUTTON_LEFT_THUMB, 0, 0, 0, false},
      {"rightstick:", ALLEGRO_GAMEPAD_BUTTON_RIGHT_THUMB, 0, 0, 0, false},

      {"dpup:", -1, ALLEGRO_GAMEPAD_STICK_DPAD, 1, -1, false},
      {"dpdown:", -1, ALLEGRO_GAMEPAD_STICK_DPAD, 1, 1, false},
      {"dpleft:", -1, ALLEGRO_GAMEPAD_STICK_DPAD, 0, -1, false},
      {"dpright:", -1, ALLEGRO_GAMEPAD_STICK_DPAD, 0, 1, false},
      {"leftx:", -1, ALLEGRO_GAMEPAD_STICK_LEFT_THUMB, 0, 0, false},
      {"lefty:", -1, ALLEGRO_GAMEPAD_STICK_LEFT_THUMB, 1, 0, false},
      {"rightx:", -1, ALLEGRO_GAMEPAD_STICK_RIGHT_THUMB, 0, 0, false},
      {"righty:", -1, ALLEGRO_GAMEPAD_STICK_RIGHT_THUMB, 1, 0, false},
      {"lefttrigger:", -1, ALLEGRO_GAMEPAD_STICK_LEFT_TRIGGER, 0, 0, true},
      {"righttrigger:", -1, ALLEGRO_GAMEPAD_STICK_RIGHT_TRIGGER, 0, 0, true},
   };

   /*
    * In the mapping, hats are 1,2,4,8 => up,right,down,left
    */

   const char *c = str;
   size_t n = strcspn(c, ",");
   char guid_str[33];
   guid_str[32] = '\0';
   if (n >= sizeof(guid_str) || c[n] != ',') {
      ALLEGRO_ERROR("GUID too long or missing trailing comma.\n");
      return false;
   }
   memcpy(guid_str, c, n);
   mapping->guid = _al_joystick_guid_from_str(guid_str);
   c += n + 1;

   n = strcspn(c, ",");
   if (c[n] != ',') {
      ALLEGRO_ERROR("Missing trailing comma while parsing mapping name.\n");
      return false;
   }
   memcpy(mapping->name, c, n >= sizeof(mapping->name) ? sizeof(mapping->name) - 1 : n);
   c += n + 1;

   /* field out_sign: governs min/max values
    * out_sign: same as field out_sign, overrides if non-zero
    * in_sign: designates whether it's pos or neg
    */
   while (*c) {
      int out_sign = 0;
      if (*c == '+') {
         out_sign = 1;
         c++;
      } else if (*c == '-') {
         out_sign = -1;
         c++;
      }

      n = strlen("platform:");
      if (strncmp(c, "platform:", n) == 0) {
         c += n;
         n = strcspn(c, ",");
         if (n >= sizeof(mapping->platform) || c[n] != ',') {
            ALLEGRO_ERROR("Platform too long or missing trailing comma.\n");
            return false;
         }
         memcpy(mapping->platform, c, n);
         c += n + 1;
      }
      else {
         for (int i = 0; i < (int)(sizeof(fields) / sizeof(fields[0])); i++) {
            const _FIELD *field = &fields[i];
            n = strlen(field->name);
            if (strncmp(c, field->name, n) != 0)
               continue;

            c += n;

            int in_sign = 0;
            if (*c == '+') {
               in_sign = 1;
               c++;
            } else if (*c == '-') {
               in_sign = -1;
               c++;
            }

            /* Axes and hats are both treated as axes. */
            if (*c == 'a' || *c == 'h') {
               bool is_hat = *c == 'h';

               c += 1;
               int index = (int)strtoul(c, (char**) &c, 10);

               if (is_hat) {
                  /* Syntax is: h0.1 */
                  c += 1;
                  index *= 2; /* This assumes hats always have two axes. */
                  int hat_bit = (int)strtoul(c, (char**) &c, 10);
                  if (hat_bit == 1 || hat_bit == 4)
                     index += 1;
                  switch (hat_bit) {
                     case 1:
                        in_sign = -1;
                        break;
                     case 2:
                        in_sign = 1;
                        break;
                     case 4:
                        in_sign = 1;
                        break;
                     case 8:
                        in_sign = -1;
                        break;
                  }
               }

               float one = 1;
               if (*c == '~')
                  one = -1;

               _AL_JOYSTICK_OUTPUT *output;
               if (is_hat) {
                  output = get_or_insert_output(&mapping->hat_map, index);
                  if (!output)
                     return false;
               }
               else {
                  output = get_or_insert_output(&mapping->axis_map, index);
                  if (!output)
                     return false;
               }
               if (field->button >= 0) {
                  output->button = field->button;
                  output->button_enabled = true;
                  output->button_threshold = 0.5 * in_sign * one;
               }
               else {
                  if (out_sign == 0) {
                     /* Field sign is the default. */
                     out_sign = field->out_sign;
                  }

                  switch (in_sign) {
                     case -1:
                        output->neg_stick = field->stick;
                        output->neg_axis = field->axis;
                        output->neg_enabled = true;
                        switch (out_sign) {
                           case -1:
                              output->neg_min = 0;
                              output->neg_max = -one;
                              break;
                           case 0:
                              output->neg_min = -one;
                              output->neg_max = one;
                              break;
                           case 1:
                              output->neg_min = 0;
                              output->neg_max = one;
                              break;
                        }
                        break;
                     case 0:
                        output->pos_stick = field->stick;
                        output->pos_axis = field->axis;
                        output->neg_stick = field->stick;
                        output->neg_axis = field->axis;
                        output->pos_enabled = true;
                        output->neg_enabled = true;
                        switch (out_sign) {
                           case -1:
                              output->pos_min = -0.5 * one;
                              output->pos_max = -one;
                              output->neg_min = -0.5 * one;
                              output->neg_max = 0.;
                              break;
                           case 0:
                              output->pos_min = 0.;
                              output->pos_max = one;
                              output->neg_min = 0.;
                              output->neg_max = -one;
                              break;
                           case 1:
                              output->pos_min = 0.5 * one;
                              output->pos_max = one;
                              output->neg_min = 0.5 * one;
                              output->neg_max = 0.;
                              break;
                        }
                        break;
                     case 1:
                        output->pos_stick = field->stick;
                        output->pos_axis = field->axis;
                        output->pos_enabled = true;
                        switch (out_sign) {
                           case -1:
                              output->pos_min = 0;
                              output->pos_max = -one;
                              break;
                           case 0:
                              output->pos_min = -one;
                              output->pos_max = one;
                              break;
                           case 1:
                              output->pos_min = 0;
                              output->pos_max = one;
                              break;
                        }
                        break;
                  }
                  trigger_transform(output, field->is_trigger);
               }

#ifdef JOYSTICK_DEBUG
               if (is_hat) {
                  printf("hat: %d field: %s\n", index, field->name);
               }
               else {
                  printf("axis: %d field: %s\n", index, field->name);
               }
               print_output(*output);
               printf("\n");
#endif
            }
            /* Buttons. */
            else if (*c == 'b') {
               c += 1;
               int index = (int)strtoul(c, (char**) &c, 10);
               _AL_JOYSTICK_OUTPUT *output = get_or_insert_output(&mapping->button_map, index);
               if (!output)
                  return false;
               if (field->button >= 0) {
                  if (in_sign != 0) {
                     printf("Button with in sign? 2\n");
                     return false;
                  }
                  output->button = field->button;
                  output->button_enabled = true;
               }
               else {
                  /* Buttons only use the pos mapping. */
                  output->pos_stick = field->stick;
                  output->pos_axis = field->axis;
                  output->pos_enabled = true;
                  switch (out_sign) {
                     case -1:
                        output->pos_min = 0;
                        output->pos_max = -1.;
                        break;
                     case 0:
                        output->pos_min = -1.;
                        output->pos_max = 1.;
                        break;
                     case 1:
                        output->pos_min = 0;
                        output->pos_max = 1.;
                        break;
                  }
               }
#ifdef JOYSTICK_DEBUG
               printf("button %d %s\n", index, field->name);
               print_output(*output);
               printf("\n");
#endif
               trigger_transform(output, field->is_trigger);
            }
            else {
               ALLEGRO_ERROR("Unknown input type, expected 'b', 'h', or 'a': %c.\n", *c);
               return false;
            }
            break;
         }
      }
      n = strcspn(c, ",");
      if (c[n] != ',')
         break;
      c += n + 1;
   }
#ifdef JOYSTICK_DEBUG
   printf("=========================\n");
   fflush(stdout);
#endif
   return true;
}



/* Function: al_set_joystick_mappings
 */
bool al_set_joystick_mappings(const char *filename)
{
   if (!filename)
      return false;
   ALLEGRO_FILE *f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_ERROR("Couldn't open joystick mapping file: %s\n", filename);
      return false;
   }
   bool result = al_set_joystick_mappings_f(f);
   al_fclose(f);
   return result;
}


/* Function: al_set_joystick_mappings_f
 */
bool al_set_joystick_mappings_f(ALLEGRO_FILE *f)
{
   char line[1024];

   while (al_fgets(f, line, sizeof(line))) {
      if (line[0] == '#' || line[0] == '\n')
         continue;
      _AL_JOYSTICK_MAPPING *mapping = _al_vector_alloc_back(&joystick_mappings);
      if (!mapping)
         return false;
      if (!parse_sdl_joystick_mapping(line, mapping)) {
         ALLEGRO_ERROR("Could not parse mapping line: %s\n", line);
#ifdef JOYSTICK_DEBUG
         print_mapping(mapping);
         printf("========================");
#endif
         return false;
      }
   }
   ALLEGRO_INFO("Parsed %d joystick mappings\n", (int)_al_vector_size(&joystick_mappings));
   return true;
}



const _AL_JOYSTICK_MAPPING *_al_get_gamepad_mapping(const char *platform, ALLEGRO_JOYSTICK_GUID guid)
{
   if (compat_5_2_10())
      return NULL;
   ALLEGRO_JOYSTICK_GUID loose_guid = guid;
   /* Zero out the version. */
   loose_guid.val[12] = 0;
   loose_guid.val[13] = 0;
   char guid_str[33];
   guid_str[32] = '\0';
   _al_joystick_guid_to_str(guid, guid_str);

   ALLEGRO_INFO("Looking for joystick mapping for platform: %s guid: %s\n", platform, guid_str);
   /* Search in reverse order, so that subsequent al_set_joystick_mappings override prior ones. */
   for (int i = (int)_al_vector_size(&joystick_mappings) - 1; i >= 0; i--) {
      _AL_JOYSTICK_MAPPING *m = _al_vector_ref(&joystick_mappings, i);
      if (strcmp(m->platform, platform) != 0)
         continue;
      if (memcmp(&m->guid, &guid, sizeof(guid)) == 0)
         return m;
   }
   _al_joystick_guid_to_str(loose_guid, guid_str);
   ALLEGRO_INFO("Looking for joystick mapping for platform: %s loose guid: %s\n", platform, guid_str);
   for (int i = (int)_al_vector_size(&joystick_mappings) - 1; i >= 0; i--) {
      _AL_JOYSTICK_MAPPING *m = _al_vector_ref(&joystick_mappings, i);
      if (strcmp(m->platform, platform) != 0)
         continue;
      if (memcmp(&m->guid, &loose_guid, sizeof(loose_guid)) == 0)
         return m;
   }
   ALLEGRO_WARN("Could not find joystick mapping for platform: %s guid: %s\n", platform, guid_str);
   return NULL;
}



ALLEGRO_JOYSTICK_GUID _al_new_joystick_guid(uint16_t bus, uint16_t vendor, uint16_t product, uint16_t version,
      const char *vendor_name, const char *product_name, uint8_t driver_signature, uint8_t driver_data)
{
   ALLEGRO_JOYSTICK_GUID guid;
   uint16_t *guid_16 = (uint16_t*)guid.val;
   (void)vendor_name;

   memset(&guid, 0, sizeof(guid));

   guid_16[0] = bus;
   if (vendor) {
      guid_16[2] = vendor;
      guid_16[4] = product;
      guid_16[6] = version;
      guid.val[14] = driver_signature;
      guid.val[15] = driver_data;
   }
   else {
      size_t max_name_len = sizeof(guid) - 4;

      if (driver_signature) {
          max_name_len -= 2;
          guid.val[14] = driver_signature;
          guid.val[15] = driver_data;
      }
      if (product_name) {
         strncpy((char*)&guid.val[4], product_name, max_name_len);
      }
   }
   return guid;
}



void _al_joystick_guid_to_str(ALLEGRO_JOYSTICK_GUID guid, char* str)
{
   const char chars[] = "0123456789abcdef";
   for (int i = 0; i < (int)sizeof(guid.val); i++) {
      *str++ = chars[guid.val[i] >> 4];
      *str++ = chars[guid.val[i] & 0xf];
   }
}



static int nibble_from_char(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return 10 + c - 'A';
    if (c >= 'a' && c <= 'f')
        return 10 + c - 'a';
    return 0;
}



ALLEGRO_JOYSTICK_GUID _al_joystick_guid_from_str(const char *str)
{
    ALLEGRO_JOYSTICK_GUID guid;
    memset(&guid, 0, sizeof(guid));
    int len = (int)strlen(str);
    len &= ~0x1;
    if (len / 2 > (int)sizeof(guid.val))
        len = 2 * sizeof(guid.val);
    for (int i = 0; i < len; i += 2) {
        guid.val[i / 2] = (nibble_from_char(str[i]) << 4) | nibble_from_char(str[i + 1]);
    }
    return guid;
}


/* Function: al_get_joystick_guid
 */
ALLEGRO_JOYSTICK_GUID al_get_joystick_guid(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(joy);
   return joy->info.guid;
}


/* Function: al_get_joystick_type
 */
ALLEGRO_JOYSTICK_TYPE al_get_joystick_type(ALLEGRO_JOYSTICK *joy)
{
   ASSERT(joy);
   return joy->info.type;
}



/* Make an output that maps to a button, for unknown joysticks.
 */
_AL_JOYSTICK_OUTPUT _al_new_joystick_button_output(int button)
{
   _AL_JOYSTICK_OUTPUT output;
   memset(&output, 0, sizeof(output));
   output.button = button;
   output.button_enabled = true;
   return output;
}



/* Make an output that maps to a stick + axis, for unknown joysticks.
 */
_AL_JOYSTICK_OUTPUT _al_new_joystick_stick_output(int stick, int axis)
{
   _AL_JOYSTICK_OUTPUT output;
   memset(&output, 0, sizeof(output));
   output.button = -1;
   output.pos_stick = stick;
   output.pos_axis = axis;
   output.pos_min = 0.;
   output.pos_max = 1.;
   output.neg_stick = stick;
   output.neg_axis = axis;
   output.neg_min = 0.;
   output.neg_max = -1.;
   output.pos_enabled = true;
   output.neg_enabled = true;
   return output;
}


/*
 *  Helper to generate an event after an axis is moved.
 *  The joystick must be locked BEFORE entering this function.
 */
void _al_joystick_generate_axis_event(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *joystate,
      _AL_JOYSTICK_OUTPUT output, float pos)
{
   ALLEGRO_EVENT event;
   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

   if (!_al_event_source_needs_to_generate_event(es))
      return;

   if (output.button_enabled) {
      ALLEGRO_EVENT_TYPE event_type;
      bool generate;
      float threshold = output.button_threshold;
      /* Threshold sign also determines which direction the axis needs to move to trigger the button. */
      if (threshold < 0.) {
         threshold = -threshold;
         pos = -pos;
      }
      if (pos > threshold) {
         if (joystate->button[output.button] > 0) {
            generate = false;
         }
         else {
            event_type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
            generate = true;
         }
         joystate->button[output.button] = 32767;
      }
      else {
         if (joystate->button[output.button] > 0) {
            event_type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
            generate = true;
         }
         else {
            generate = false;
         }
         joystate->button[output.button] = 0;
      }
      if (generate) {
         event.joystick.type = event_type;
         event.joystick.timestamp = al_get_time();
         event.joystick.id = joy;
         event.joystick.stick = 0;
         event.joystick.axis = 0;
         event.joystick.pos = 0.0;
         event.joystick.button = output.button;
         _al_event_source_emit_event(es, &event);
      }
   }
   else {
      int out_stick;
      int out_axis;
      float out_pos;
      bool enabled;
      if (pos >= 0) {
         out_stick = output.pos_stick;
         out_axis = output.pos_axis;
         out_pos = output.pos_min + (output.pos_max - output.pos_min) * pos;
         enabled = output.pos_enabled;
      } else {
         out_stick = output.neg_stick;
         out_axis = output.neg_axis;
         out_pos = output.neg_min + (output.neg_max - output.neg_min) * -pos;
         enabled = output.neg_enabled;
      }
      if (enabled) {
         joystate->stick[out_stick].axis[out_axis] = out_pos;
         event.joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
         event.joystick.timestamp = al_get_time();
         event.joystick.id = joy;
         event.joystick.stick = out_stick;
         event.joystick.axis = out_axis;
         event.joystick.pos = out_pos;
         event.joystick.button = 0;
         _al_event_source_emit_event(es, &event);
      }
   }
}



/*
 *  Helper to generate an event after a button is pressed or released.
 *  The joystick must be locked BEFORE entering this function.
 */
void _al_joystick_generate_button_event(ALLEGRO_JOYSTICK *joy, ALLEGRO_JOYSTICK_STATE *joystate, _AL_JOYSTICK_OUTPUT output, int value)
{
   ALLEGRO_EVENT event;
   ALLEGRO_EVENT_SOURCE *es = al_get_joystick_event_source();

   if (!_al_event_source_needs_to_generate_event(es))
      return;

   if (output.button_enabled) {
      ALLEGRO_EVENT_TYPE event_type;
      bool generate;
      if (value > 0) {
         if (joystate->button[output.button] > 0) {
            generate = false;
         }
         else {
            event_type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
            generate = true;
         }
         joystate->button[output.button] = 32767;
      }
      else {
         if (joystate->button[output.button] > 0) {
            event_type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
            generate = true;
         }
         else {
            generate = false;
         }
         joystate->button[output.button] = 0;
      }
      if (generate) {
         event.joystick.type = event_type;
         event.joystick.timestamp = al_get_time();
         event.joystick.id = joy;
         event.joystick.stick = 0;
         event.joystick.axis = 0;
         event.joystick.pos = 0.0;
         event.joystick.button = output.button;
         _al_event_source_emit_event(es, &event);
      }
   }
   else if (output.pos_enabled) {
      float pos = value > 0 ? output.pos_max : output.pos_min;
      joystate->stick[output.pos_stick].axis[output.pos_axis] = pos;
      event.joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
      event.joystick.timestamp = al_get_time();
      event.joystick.id = joy;
      event.joystick.stick = output.pos_stick;
      event.joystick.axis = output.pos_axis;
      event.joystick.pos = pos;
      event.joystick.button = 0;
      _al_event_source_emit_event(es, &event);
   }
}

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
/* vim: set sts=3 sw=3 et: */
