#ifndef GAME_HPP
#define GAME_HPP

#include "cosmic_protector.hpp"

#ifdef ALLEGRO_MSVC
/* MSVC (up to ver. 9 at least) ignores exception specifications */
#pragma warning( disable : 4290 )
#endif

const char* getResource(const char* fmt, ...);
bool loadResources(void);
bool init(void);
void done(void);
float randf(float lo, float hi);

extern bool kb_installed;
extern bool joy_installed;


#endif // GAME_HPP

