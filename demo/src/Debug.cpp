#include "a5teroids.hpp"

void debug_message(char* fmt, ...)
{
   /*
   Configuration& cfg = Configuration::getInstance();
   if (!cfg.debuggingEnabled())
      return;
   */

   va_list ap;
   char msg[1000];

   va_start(ap, fmt);
   vsnprintf(msg, (sizeof(msg)/sizeof(*msg))-1, fmt, ap);
   printf("%s", msg);
}

