#ifndef                __DEMO_GAMESTATE_H__
#define                __DEMO_GAMESTATE_H__

/*
   Structure that defines one game state/screen/page. Each screen is
   basically just a bunch of functions that initialize the state, run
   a frame of logic and draw it to the screen. Each actual state must
   fill in the struct by setting the function pointers to point to
   the functions that implement that particular state. Note that not
   all functions are mandatory, but most are.
*/
typedef struct GAMESTATE GAMESTATE;

struct GAMESTATE {
   /* Supposed to return the ID of the state. Each stat must have a
      unique ID. Not: all actual IDs are defined in defines.h. */
   int (*id) (void);

   /* Initializes the gamestate. This may be setting some static
      variables to their default values, generating game data,
      randomizing the playing field, etc. */
   void (*init) (void);

   /* Destroys the gamestate. Note that a particular gamestate may
      set this pointr to NULL if there's nothing to destroy. */
   void (*deinit) (void);

   /* Runs one frame of logic, for example gather input, update game
      physics, enemy AI, do collision detection, etc. Must return ID
      of the next state. Most of the time this should be the ID of
      the state itself, but for example when ESC is pressed this
      should be the ID of the main menu or something along those lines. */
   int (*update) (void);

   /* Draws the state to the given bitmap. Note that the state should
      assume that the target bitmap is acquired and cleared. */
   void (*draw) (void);
};

#endif                                /* __DEMO_GAMESTATE_H__ */
