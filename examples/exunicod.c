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
#include <allegro.h>

#define DATAFILE_NAME   "unifont.dat"

#define NLANGUAGES 12


/* Animation timer. */
volatile int ticks = 0;
void ticker(void)
{
    ticks++;
}

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

char message_ka[] = "\x20\x00\xa8\x0C\xbf\x0C\xae\x0C\x97\x0C\xc6\x0C\x20\x00\xb8\x0C\xc1\x0C\xb8\x0C\xcd\x0C\xb5\x0C\xbe\x0C\x97\x0C\xa4\x0C";

char message_ta[] = "\x20\x00\x89\x0B\x99\x0B\x82\x0B\x95\x0B\xC8\x0B\xB3\x0B\x20\x00\xB5\x0B\xB0\x0B\xC7\x0B\xB5\x0B\xB1\x0B\x82\x0B\x95\x0B\xBF\x0B\xB1\x0B\xA5\x0B\x00\x00";

char message_zh[] = "\x22\x6b\xCE\x8F\x7F\x4f\x28\x75\x20\x00\x00\x00";

char message_de[] = "\x57\x00\x69\x00\x6c\x00\x6c\x00\x6b\x00\x6f\x00\x6d\x00\x6d\x00\x65\x00\x6e\x00\x20\x00\x62\x00\x65\x00\x69\x00\x20\x00\x00\x00";

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

char message_ka[] = "\x20\x00\x0C\xa8\x0C\xbf\x0C\xae\x0C\x97\x0C\xc6\x20\x00\x0C\xb8\x0C\xc1\x0C\xb8\x0C\xcd\x0C\xb5\x0C\xbe\x0C\x97\xa4\x0C";

char message_ta[] = "\x00\x20\x0B\x89\x0B\x99\x0B\x82\x0B\x95\x0B\xC8\x0B\xB3\x00\x20\x0B\xB5\x0B\xB0\x0B\xC7\x0B\xB5\x0B\xB1\x0B\x82\x0B\x95\x0B\xBF\x0B\xB1\x0B\xA5\x00\x00";

char message_zh[] = "\x6b\x22\x8F\xCE\x4f\x7F\x75\x28\x00\x20\x00\x00";

char message_de[] = "\x00\x57\x00\x69\x00\x6c\x00\x6c\x00\x6b\x00\x6f\x00\x6d\x00\x6d\x00\x65\x00\x6e\x00\x20\x00\x62\x00\x65\x00\x69\x00\x20\x00\x00";

char allegro_str[] = "\x00\x41\x00\x6c\x00\x6c\x00\x65\x00\x67\x00\x72\x00\x6f\x00\x00";

#elif !defined SCAN_DEPEND
   #error endianess not defined
#endif


struct MESSAGE {
   char *data, *str;
   int prefix_allegro;
   int dx, dy;
   int x, y, w, h;
   int c;
};


struct MESSAGE message[] = { {message_en, NULL, FALSE, -1,  0, 0, 0, 0, 0, 0},
                             {message_fr, NULL, FALSE, -1,  0, 0, 0, 0, 0, 0},
                             {message_es, NULL, FALSE, -1,  0, 0, 0, 0, 0, 0},
                             {message_it, NULL, FALSE, -1,  0, 0, 0, 0, 0, 0},
                             {message_el, NULL, FALSE, -1,  0, 0, 0, 0, 0, 0},
                             {message_ru, NULL, FALSE, -1,  0, 0, 0, 0, 0, 0},
                             {message_he, NULL, TRUE,   1,  0, 0, 0, 0, 0, 0},
                             {message_ja, NULL, TRUE,  -1,  0, 0, 0, 0, 0, 0},
                             {message_ka, NULL, TRUE,  -1,  0, 0, 0, 0, 0, 0},
                             {message_ta, NULL, TRUE,  -1,  0, 0, 0, 0, 0, 0},
                             {message_zh, NULL, FALSE, -1,  0, 0, 0, 0, 0, 0},
                             {message_de, NULL, FALSE, -1,  0, 0, 0, 0, 0, 0}
};


static int overlap(int i, int j, int pad)
{
   return message[i].x - pad < message[j].x + message[j].w + pad &&
      message[j].x - pad < message[i].x + message[i].w + pad &&
      message[i].y - pad < message[j].y + message[j].h + pad &&
      message[j].y - pad < message[i].y + message[i].h + pad;
}


int main(int argc, char *argv[])
{
   DATAFILE *data;
   FONT *f;
   BITMAP *buffer;
   int i, j, k, height;
   char buf[256], tmp[256], tmp2[256];
   int counter = 0, drawn = 0;
   int scroll_w, scroll_h;
   int background_color;

   /* set the text encoding format BEFORE initializing the library */
   set_uformat(U_UNICODE);

   /*  past this point, every string that we pass to or retrieve
    *  from any Allegro API call must be in 16-bit Unicode format
    */

   srand(time(NULL));
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

   /* create a buffer for drawing */
   buffer = create_bitmap(SCREEN_W, SCREEN_H);

   /* get a handle to the Unicode font */
   f = data[0].dat;
   height = text_height(f);

   /* The are for the text messages. If it gets too crowded once we have more
    * languages, this can be increased.
    */
   scroll_w = SCREEN_W * 2;
   scroll_h = SCREEN_H + height;

   /* one of the bright colors in the default palette */
   background_color = 56 + AL_RAND() % 48;

   /* prepare the messages */
   for (i = 0; i < NLANGUAGES; i++) {

      /* the regular Standard C string manipulation functions don't work
       * with 16-bit Unicode, so we use the Allegro Unicode API
       */
      message[i].str = malloc(ustrsize(message[i].data) + ustrsizez(allegro_str));

      if (message[i].prefix_allegro) {
         ustrcpy(message[i].str, allegro_str);
         ustrcat(message[i].str, message[i].data);
      }
      else {
         ustrcpy(message[i].str, message[i].data);
         ustrcat(message[i].str, allegro_str);
      }

      message[i].w = text_length(f, message[i].str);
      message[i].h = text_height(f);

      /* one of the dark colors in the default palette */
      message[i].c = 104 + AL_RAND() % 144;

      message[i].dx *= 1 + AL_RAND() % 4;
      message[i].dy = AL_RAND() % 3 - 1;

      /* find not-overlapped position, try 1000 times */
      for (k = 0; k < 1000; k++) {
         message[i].x = AL_RAND() % scroll_w;
         /* make sure the message is not sliced by a screen edge */
         message[i].y = 10 + AL_RAND() % (SCREEN_H - height - 20);
         for (j = 0; j < i; j++) {
            if (overlap(i, j, 10))
               break;
         }
         if (j == i)
            break;
      }
   }

   install_int_ex(ticker, BPS_TO_TIMER(30));
   /* do the scrolling */
   while (!keypressed()) {
      /* Animation. */
      while (counter <= ticks) {
         for (i = 0; i < NLANGUAGES; i++) {
            message[i].x += message[i].dx;
            if (message[i].x >= scroll_w)
               message[i].x -= scroll_w;
            if (message[i].x < 0)
               message[i].x += scroll_w;
            message[i].y += message[i].dy;
            if (message[i].y >= scroll_h)
               message[i].y -= scroll_h;
            if (message[i].y < 0)
               message[i].y += scroll_h;
         }
         counter++;
      }

      /* Draw current frame. */
      if (drawn < counter) {
         clear_to_color(buffer, background_color);
         for (i = 0; i < NLANGUAGES; i++) {
            char *str = message[i].str;
            int x = message[i].x;
            int y = message[i].y;
            int c = message[i].c;
            /* draw it 4 times to get the wrap-around effect */
            textout_ex(buffer, f, str, x, y, c, -1);
            textout_ex(buffer, f, str, x - scroll_w, y, c, -1);
            textout_ex(buffer, f, str, x, y - scroll_h, c, -1);
            textout_ex(buffer, f, str, x - scroll_w, y - scroll_h, c, -1);
         }
         blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
         drawn = counter;
      }
      else {
         rest(10); /* We are too fast, give time to the OS. */
      }
   }

   destroy_bitmap(buffer);

   unload_datafile(data);

   return 0;
}
END_OF_MAIN()
