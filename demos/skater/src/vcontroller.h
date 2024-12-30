#ifndef __DEMO_VCONTROLLER_H__
#define __DEMO_VCONTROLLER_H__

/*
   Structure that defines a virtual controller. A virtual controller is
   an abstraction of physical controllers such as kyboards, mice, gamepads,
   joysticks, etc. to make them more easily manageable. This particular
   implementation supports up to 8 digital buttons, but can easily be
   expanded to support analogue axes as well. Each implementation of an
   actual controller should implement all the functions in this struct.
   Each function is passed a pointer to the associated object, a bit
   like the implicit this pointer in C++ classes.
*/

typedef struct VCONTROLLER VCONTROLLER;

struct VCONTROLLER {
   /* Status of each of the 8 virtual buttons. Virtual controllers should
      update this array in the poll function and the user programmer
      should read them in order to gather input. */
   int button[8];

   /* Pointer to some private internal data that each controller may use.
      Note: perhaps a cleaner implementation would be to let each controller
      use static variables instead. */
   void *private_data;

   /* Polls the underlying physical controller and updates the
      buttons array. */
   void (*poll) (VCONTROLLER *);

   /* Reads button assignments (and any other settings) from a config file. */
   void (*read_config) (VCONTROLLER *, const char *);

   /* Writes button assignments (and any other settings) to a config file. */
   void (*write_config) (VCONTROLLER *, const char *);

   /* Supposed to calibrate the specified button. Returns 0 if the button
      has not been calibrated and non-zero as soon as it is. The user
      programmer should call this function in a loop until it returns non
      zero or button calibration has been canceled in some way. */
   int (*calibrate_button) (VCONTROLLER *, int);

   /* Returns a short description of the specified button that may be
      used in the user interface (for example). */
   const char *(*get_button_description) (VCONTROLLER *, int);
};


/*
   Destroys the given controller, frees memory and writes button assignments
   to the config file.

   Parameters:
      VCONTROLLER *controller - The pointer to be destroyed.
      char *config_path - Path to the config file where settings should
         be written.

   Returns:
      nothing
*/
void destroy_vcontroller(VCONTROLLER * controller,
                         const char *config_path);

#endif /* __DEMO_VCONTROLLER_H__ */
