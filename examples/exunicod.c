/*
 *    Example program for the Allegro library, by Eric Botcazou.
 *
 *    This program demonstrates the use of the 16-bit Unicode text
 *    encoding format with Allegro. The example displays a message
 *    translated to different languages scrolling on the screen
 *    using an external font containing the required characters to
 *    display those messages.
 *
 *    Note how the Allegro unicode string functions resemble the
 *    functions you can find in the standard C library, only these
 *    handle Unicode on all platforms.
 */


#include <stdlib.h>
#include "allegro.h"

#define DATAFILE_NAME   "unifont.dat"

#define NLANGUAGES 10


#if defined ALLEGRO_LITTLE_ENDIAN

/* UTF-16LE */
char message_en[] = "\x57\x00\x65\x00\x6c\x00\x63\x00\x6f\x00\x6d\x00\x65\x00\x20\x00\x74\x00\x6f\x00\x20\x00\x00\x00";

char message_fr[] = "\x42\x00\x69\x00\x65\x00\x6e\x00\x76\x00\x65\x00\x6e\x00\x75\x00\x65\x00\x20\x00\xE0\x00\x20\x00\x00\x00";

char message_es[] = "\x42\x00\x69\x00\x65\x00\x6e\x00\x76\x00\x65\x00\x6e\x00\x69\x00\x64\x00\x6f\x00\x20\x00\x61\x00\x20\x00\x00\x00";

char message_it[] = "\x42\x00\x65\x00\x6e\x00\x76\x00\x65\x00\x6e\x00\x75\x00\x74\x00\x69\x00\x20\x00\x61\x00\x64\x00\x20\x00\x00\x00";

char message_el[] = "\x9A\x03\xB1\x03\xBB\x03\xCE\x03\xC2\x03\x20\x00\xAE\x03\xC1\x03\xB8\x03\xB1\x03\xC4\x03\xB5\x03\x20\x00\xC3\x03\xC4\x03\xBF\x03\x20\x00\x00\x00";

char message_ru[] = "\x14\x04\x3e\x04\x31\x04\x40\x04\x3e\x04\x20\x00\x3f\x04\x3e\x04\x36\x04\x30\x04\x3b\x04\x3e\x04\x32\x04\x30\x04\x42\x04\x4c\x04\x20\x00\x32\x04\x20\x00\x00\x00";

char message_he[] = "\x20\x00\xDC\x05\xD0\x05\x20\x00\xDD\x05\xD9\x05\xD0\x05\xD1\x05\xD4\x05\x20\x00\xDD\x05\xD9\x05\xDB\x05\xD5\x05\xE8\x05\xD1\x05\x00\x00";

char message_ja[] = "\x78\x30\x88\x30\x46\x30\x53\x30\x5d\x30\x00\x00";

char message_ta[] = "\x20\x00\x89\x0B\x99\x0B\x82\x0B\x95\x0B\xC8\x0B\xB3\x0B\x20\x00\xB5\x0B\xB0\x0B\xC7\x0B\xB5\x0B\xB1\x0B\x82\x0B\x95\x0B\xBF\x0B\xB1\x0B\xA5\x0B\x00\x00";
                                                          
char message_zh[] = "\x22\x6b\xCE\x8F\x7F\x4f\x28\x75\x20\x00\x00\x00";

char allegro_str[] = "\x41\x00\x6c\x00\x6c\x00\x65\x00\x67\x00\x72\x00\x6f\x00\x00\x00";

#elif defined ALLEGRO_BIG_ENDIAN

/* UTF-16BE */
char message_en[] = "\x00\x57\x00\x65\x00\x6c\x00\x63\x00\x6f\x00\x6d\x00\x65\x00\x20\x00\x74\x00\x6f\x00\x20\x00\x00";

char message_fr[] = "\x00\x42\x00\x69\x00\x65\x00\x6e\x00\x76\x00\x65\x00\x6e\x00\x75\x00\x65\x00\x20\x00\xE0\x00\x20\x00\x00";

char message_es[] = "\x00\x42\x00\x69\x00\x65\x00\x6e\x00\x76\x00\x65\x00\x6e\x00\x69\x00\x64\x00\x6f\x00\x20\x00\x61\x00\x20\x00\x00";

char message_it[] = "\x00\x42\x00\x65\x00\x6e\x00\x76\x00\x65\x00\x6e\x00\x75\x00\x74\x00\x69\x00\x20\x00\x61\x00\x64\x00\x20\x00\x00";

char message_el[] = "\x03\x9A\x03\xB1\x03\xBB\x03\xCE\x03\xC2\x00\x20\x03\xAE\x03\xC1\x03\xB8\x03\xB1\x03\xC4\x03\xB5\x00\x20\x03\xC3\x03\xC4\x03\xBF\x00\x20\x00\x00";

char message_ru[] = "\x04\x14\x04\x3e\x04\x31\x04\x40\x04\x3e\x00\x20\x04\x3f\x04\x3e\x04\x36\x04\x30\x04\x3b\x04\x3e\x04\x32\x04\x30\x04\x42\x04\x4c\x00\x20\x04\x32\x00\x20\x00\x00";

char message_he[] = "\x00\x20\x05\xDC\x05\xD0\x00\x20\x05\xDD\x05\xD9\x05\xD0\x05\xD1\x05\xD4\x00\x20\x05\xDD\x05\xD9\x05\xDB\x05\xD5\x05\xE8\x05\xD1\x00\x00";

char message_ja[] = "\x30\x78\x30\x88\x30\x46\x30\x53\x30\x5d\x00\x00";

char message_ta[] = "\x00\x20\x0B\x89\x0B\x99\x0B\x82\x0B\x95\x0B\xC8\x0B\xB3\x00\x20\x0B\xB5\x0B\xB0\x0B\xC7\x0B\xB5\x0B\xB1\x0B\x82\x0B\x95\x0B\xBF\x0B\xB1\x0B\xA5\x00\x00";

char message_zh[] = "\x6b\x22\x8F\xCE\x4f\x7F\x75\x28\x00\x20\x00\x00";

char allegro_str[] = "\x00\x41\x00\x6c\x00\x6c\x00\x65\x00\x67\x00\x72\x00\x6f\x00\x00";

#else
   #error endianess not defined
#endif


struct MESSAGE {
   char *data;
   int right_to_left;
};


struct MESSAGE message[] = { {message_en, FALSE},
                             {message_fr, FALSE},
                             {message_es, FALSE},
                             {message_it, FALSE},
                             {message_el, FALSE},
                             {message_ru, FALSE},
                             {message_he, TRUE},
                             {message_ja, TRUE},
                             {message_ta, TRUE},
                             {message_zh, FALSE} };

int speed[] = {1, 2, 4, 3, 2, 3, 2, 4, 5, 3};


int main(int argc, char *argv[])
{
   DATAFILE *data;
   FONT *f;
   BITMAP *mesg_bmp[NLANGUAGES];
   int length[NLANGUAGES], pos[NLANGUAGES];
   int i, nmesgs, height, hpad, delta;
   char *mesg, buf[256], tmp[256], tmp2[256];

   /* set the text encoding format BEFORE initializing the library */
   set_uformat(U_UNICODE);

   /*  past this point, every string that we pass to or retrieve
    *  from any Allegro API call must be in 16-bit Unicode format
    */

   if (allegro_init() != 0)
      return 1;
   install_keyboard();
   install_timer();

   /* load the datafile containing the Unicode font */
   replace_filename(buf, uconvert_ascii(argv[0], tmp), uconvert_ascii(DATAFILE_NAME, tmp2), sizeof(buf));
   data = load_datafile(buf);
   if (!data) {
      allegro_message(uconvert_ascii("Unable to load %s\n", tmp), buf);
      return -1;
   }

   /* set the graphics mode */
   if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message(uconvert_ascii("Unable to set any graphic mode\n%s\n", tmp), allegro_error);
	 return 1;
      }
   }

   /* set the window title for windowed modes */
   set_window_title(uconvert_ascii("Unicode example program", tmp));

   clear_to_color(screen, makecol(255, 255, 255));

   /* get a handle to the Unicode font */
   f = data[0].dat;
   height = text_height(f) + 8;

   /* find the number of actually displayed messages */
   nmesgs = MIN(SCREEN_H/height, NLANGUAGES);
   hpad = (SCREEN_H - nmesgs*height)/(nmesgs+1);

   /* prepare the bitmaps */
   for (i=0; i<nmesgs; i++) {

      /* the regular Standard C string manipulation functions don't work
       * with 16-bit Unicode, so we use the Allegro Unicode API
       */
      mesg = malloc(ustrsize(message[i].data) + ustrsizez(allegro_str));

      if (message[i].right_to_left) {
         ustrcpy(mesg, allegro_str);
         ustrcat(mesg, message[i].data);
      }
      else {
         ustrcpy(mesg, message[i].data);
         ustrcat(mesg, allegro_str);
      }

      length[i] = text_length(f, mesg) + 8;
      pos[i] = ((float)AL_RAND()/RAND_MAX) * SCREEN_W;
      mesg_bmp[i] = create_system_bitmap(length[i], height);
      clear_to_color(mesg_bmp[i], makecol(255, 255, 255));
      textout_ex(mesg_bmp[i], f, mesg, 8, 1, makecol(0, 0, 0), -1);

      free(mesg);
   }

   /* do the scrolling */
   while (!keypressed()) {
      for (i=0; i<nmesgs; i++) {
         blit(mesg_bmp[i], screen, 0, 0, pos[i], hpad + i*(height+hpad), length[i], height);
         delta = pos[i] + length[i] - SCREEN_W;
         if (delta > 0)
            blit(mesg_bmp[i], screen, length[i] - delta, 0, 0, hpad + i*(height+hpad), delta, height);

         pos[i] += speed[i];
         if (pos[i] >= SCREEN_W)
            pos[i] -= SCREEN_W;
      }

      rest(33);
   }

   /* free allocated resources */
   for (i=0; i<nmesgs; i++)
      destroy_bitmap(mesg_bmp[i]);

   unload_datafile(data);

   return 0;
}
END_OF_MAIN()
