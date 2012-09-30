#include "cosmic_protector.hpp"

void debug_message(const char * fmt, ...)
{
   va_list ap;

   va_start(ap, fmt);
   vprintf(fmt, ap);
   va_end(ap);
}

