/*
 *    Example program for the Allegro library, by Eric Botcazou.
 *
 *    This program demonstrates the use of the 16-bit Unicode text
 *    encoding format with Allegro. 
 */


#include <stdlib.h>
#include <allegro.h>

#define DATAFILE_NAME   "unifont.dat"

#define NLANGUAGES 8


char message_en[] = "W\x00" "e\x00" "l\x00" "c\x00" "o\x00" "m\x00" "e\x00" " \x00" "t\x00" "o\x00"
                    " \x00\x00\x00";

char message_fr[] = "B\x00" "i\x00" "e\x00" "n\x00" "v\x00" "e\x00" "n\x00" "u\x00" "e\x00"
                    " \x00" "\xE0\x00" " \x00\x00\x00";

char message_es[] = "B\x00" "i\x00" "e\x00" "n\x00" "v\x00" "e\x00" "n\x00" "i\x00" "d\x00" "o\x00"
                    " \x00" "a\x00" " \x00\x00\x00";

char message_el[] = "\x9A\x03\xB1\x03\xBB\x03\xCE\x03\xC2\x03"
                    " \x00\xAE\x03\xC1\x03\xB8\x03\xB1\x03\xC4\x03\xB5\x03"
                    " \x00\xC3\x03\xC4\x03\xBF\x03" " \x00\x00\x00";

char message_ru[] = "\x14\x04" ">\x04" "1\x04" "@\x04" ">\x04" " \x00" "?\x04" ">\x04"
                    "6\x04" "0\x04" ";\x04" ">\x04" "2\x04" "0\x04" "B\x04" "L\x04"
                    " \x00" "2\x04" " \x00\x00\x00";

char message_he[] = " \x00\xDC\x05\xD0\x05" " \x00\xDD\x05\xD9\x05\xD0\x05\xD1\x05\xD4\x05"
                    " \x00\xDD\x05\xD9\x05\xDB\x05\xD5\x05\xE8\x05\xD1\x05\x00\x00";

char message_ja[] = "x0\x88" "0F0S0]0\x00\x00";

char message_zh[] = "\"k\xCE\x8F\x7F" "O(u \x00\00\00";


char allegro_str[] = "A\x00" "l\x00" "l\x00" "e\x00" "g\x00" "r\x00" "o\x00\x00\x00";


struct MESSAGE {
   char *data;
   int right_to_left;
};


struct MESSAGE message[] = { {message_en, FALSE},
                             {message_fr, FALSE},
                             {message_es, FALSE},
                             {message_el, FALSE},
                             {message_ru, FALSE},
                             {message_he, TRUE},
                             {message_ja, TRUE},
                             {message_zh, FALSE} };

int speed[] = {1, 2, 4, 3, 2, 3, 2, 4};


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

   allegro_init();
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
   if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) != 0) {
      allegro_message(uconvert_ascii("Unable to set a graphic mode\n", tmp));
      return -1;
   }

   /* set the window title for windowed modes */
   set_window_title(uconvert_ascii("Unicode example program", tmp));

   clear_to_color(screen, makecol(255, 255, 255));

   /* set transparent text drawing mode */
   text_mode(-1);

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
      pos[i] = ((float)rand()/RAND_MAX) * SCREEN_W;
      mesg_bmp[i] = create_system_bitmap(length[i], height);
      clear_to_color(mesg_bmp[i], makecol(255, 255, 255));
      textout(mesg_bmp[i], f, mesg, 8, 1, makecol(0, 0, 0));

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
END_OF_MAIN();
