#ifndef MISC_HPP
#define MISC_HPP

#include "a5teroids.hpp"

const char* getUserResource(const char* fmt, ...) throw (Error);
const char* getResource(const char* fmt, ...);
bool loadResources(void);
bool init(void);
void done(void);
float randf(float lo, float hi);

extern bool kb_installed;
extern bool joy_installed;


#endif // MISC_HPP

