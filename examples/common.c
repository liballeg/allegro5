#include <stdio.h>
#include <stdarg.h>

#ifdef ALLEGRO_POPUP_EXAMPLES
#include "allegro5/allegro_native_dialog.h"
void abort_example(char const *format, ...)
{
   char str[1024];
   va_list args;
   va_start(args, format);
   vsnprintf(str, sizeof str, format, args);
   va_end(args);

   // FIXME: We could check here if Allegro is installed and/or a display is
   // active - but it's probably better if make al_show_native_message_box
   // does that instead and falls back to other means to deliver the message
   // on all platforms (even without A5 installed or a display active).
   al_show_native_message_box("Error", "Cannot run example", str, NULL, 0);
   exit(1);
}
#else
void abort_example(char const *format, ...)
{
   va_list args;
   va_start(args, format);
   vfprintf(stderr, format, args);
   va_end(args);
   exit(1);
}
#endif
