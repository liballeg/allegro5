#include "title.h"
#include "data.h"
#include "star.h"

/* for parsing readme.txt */
typedef struct TEXT_LIST {
   char *text;
   struct TEXT_LIST *next;
} TEXT_LIST;

typedef struct README_SECTION {
   TEXT_LIST *head;
   TEXT_LIST *tail;
   char *flat;
   char *desc;
} README_SECTION;

/* for parsing thanks._tx and the various source files */
typedef struct CREDIT_NAME {
   char *name;
   char *text;
   TEXT_LIST *files;
   struct CREDIT_NAME *next;
} CREDIT_NAME;

/* text messages (loaded from readme.txt) */
static char *title_text;
static int title_size;

static PALETTE title_palette;

/* author credits scroller */
static int credit_width = 0;
static int credit_scroll = 0;
static int credit_offset = 0;
static int credit_age = 0;
static int credit_speed = 32;
static int credit_skip = 1;

/* text scroller at the bottom */
static int text_scroll = 0;
static int text_char;
static int text_pix;
static int text_width;

CREDIT_NAME *credit_name = NULL;

static CREDIT_NAME *credits = NULL;
static ALLEGRO_CONFIG *text;


/* reads credit info from various places */
static void load_credits(void)
{
   // exported the result from the original scanning which scanned all
   // source files to a static text.ini with the ancient A4 credits :)
   // (the original only worked when running from the A4 source folder)
   text = al_load_config_file("data/text.ini");
   CREDIT_NAME *pc = NULL;
   for (int i = 0; ; i++) {
      char k[100];
      sprintf(k, "%d", i);
      char const* vn = al_get_config_value(text, "credits", k);
      if (!vn) break;
      CREDIT_NAME *c = al_calloc(1, sizeof *c);
      c->name = strdup(vn);
      sprintf(k, "%d_0", i);
      char const* v0 = al_get_config_value(text, "credits", k);
      c->text = strdup(v0);
      TEXT_LIST *ptl = NULL;
      for (int j = 1; ;j++) {
         sprintf(k, "%d_%d", i, j);
         char const* v = al_get_config_value(text, "credits", k);
         if (!v) break;
         TEXT_LIST *tl = al_calloc(1, sizeof *tl);
         tl->text = strdup(v);
         if (ptl)
            ptl->next = tl;
         else
            c->files = tl;
         ptl = tl;
      }
      if (pc)
         pc->next = c;
      else
         credits = c;
      pc = c;
   }
   title_text = strdup(al_get_config_value(text, "readme", "text"));
   title_size = strlen(title_text);
}



static void scroller(void)
{
   int n;
   TEXT_LIST *tl;

   starfield_3d();

   /* move the scroller at the bottom */
   text_scroll += 4;

   /* update the credits position */
   if (credit_scroll <= 0) {
      if (credit_name)
         credit_name = credit_name->next;

      if (!credit_name)
         credit_name = credits;

      if (credit_name) {
         credit_width =
             text_length(data[END_FONT].dat, credit_name->name) + 24;

         if (credit_name->text)
            credit_scroll = al_get_text_width(data[TITLE_FONT].dat, credit_name->text) + SCREEN_W -
               credit_width;
         else
            credit_scroll = 256;

         tl = credit_name->files;
         n = 0;

         while (tl) {
            n++;
            tl = tl->next;
         }

         credit_offset = SCREEN_W - credit_scroll;

         credit_age = 0;
      }
   }
   else {
      credit_scroll-=4;
      credit_age+=4;
   }
}



static void draw_scroller(void)
{
   TEXT_LIST *tl;
   int n, n2, c, c2, c3;
   char *p;
   FONT *bigfont = data[TITLE_FONT].dat;
   int th = text_height(bigfont);

   /* draw the text scroller at the bottom */
   textout(bigfont, title_text,
           SCREEN_W - text_scroll, SCREEN_H - th, makecol(255, 255, 255));

   /* draw author file credits */
   if (credit_name) {
      int x, y, z;
      int ix, iy;
      tl = credit_name->files;
      n = credit_width;
      n2 = 0;

      while (tl) {
         c = 1024 + n2 * credit_speed - credit_age;

         if ((c > 0) && (c < 1024) && ((n2 % credit_skip) == 0)) {
            x = itofix(SCREEN_W / 2);
            y = itofix(SCREEN_H / 2 - 32);

            c2 = c * ((n / 13) % 17 + 1) / 32;
            if (n & 1)
               c2 = -c2;

            c2 -= 96;

            c3 = (32 +
                  fixtoi(ABS(fixsin(itofix(c / (15 + n % 42) + n))) *
                         128)) * SCREEN_W / 640;

            x += fixsin(itofix(c2)) * c3;
            y += fixcos(itofix(c2)) * c3;

            if (c < 512) {
               z = fixsqrt(itofix(c) / 512);

               x = fixmul(itofix(32), itofix(1) - z) + fixmul(x, z);
               y = fixmul(itofix(16), itofix(1) - z) + fixmul(y, z);
            }
            else if (c > 768) {
               z = fixsqrt(itofix(1024 - c) / 256);

               if (n & 2)
                  x = fixmul(itofix(128), itofix(1) - z) + fixmul(x, z);
               else
                  x = fixmul(itofix(SCREEN_W - 128),
                             itofix(1) - z) + fixmul(x, z);

               y = fixmul(itofix(SCREEN_H - 128),
                          itofix(1) - z) + fixmul(y, z);
            }

            c = 128 + (512 - ABS(512 - c)) / 24;
            c = MIN(255, c * 1.25);

            ix = fixtoi(x);
            iy = fixtoi(y);

            c2 = strlen(tl->text);
            ix -= c2 * 4;

            textout(bigfont, tl->text, ix, iy, get_palette(c));
         }

         tl = tl->next;
         n += 1234567;
         n2++;
      }
   }

   draw_starfield_3d();

   /* draw author name/desc credits */
   if (credit_name) {
      if (credit_name->text) {
         c = credit_scroll + credit_offset;
         p = credit_name->text;
         c2 = strlen(p);

         textout(bigfont, p, c, 16, get_palette(255));
      }

      c = 4;

      if (credit_age < 100)
         c -= (100 - credit_age) * (100 -
                                    credit_age) * credit_width / 10000;

      if (credit_scroll < 150)
         c += (150 - credit_scroll) * (150 -
                                       credit_scroll) * SCREEN_W /
             22500;

      rectfill(0, 0, credit_width, 60, makecol(0, 0, 0));
      textprintf(data[END_FONT].dat, c, 4, makecol(255, 255, 255), "%s:",
                    credit_name->name);
   }

   /* draw the Allegro logo over the top */
   draw_sprite(data[TITLE_BMP].dat, SCREEN_W / 2 - 160,
               SCREEN_H / 2 - 96);
}



/* displays the title screen */
int title_screen(void)
{
   static int color = 0;
   int c;
   RGB rgb;
   int updated;
   int scroll_pos = 0;

   text_scroll = 0;
   credit_width = 0;
   credit_scroll = 0;
   credit_offset = 0;
   credit_age = 0;
   credit_speed = 32;
   credit_skip = 1;

   text_char = -1;
   text_pix = 0;
   text_width = 0;

   play_midi(data[TITLE_MUSIC].dat, TRUE);
   play_sample(data[WELCOME_SPL].dat, 255, 127, 1000, FALSE);

   load_credits();

   init_starfield_3d();

   for (c = 0; c < 8; c++)
      title_palette.rgb[c] = ((PALETTE*)(data[TITLE_PAL].dat))->rgb[c];

   ///* set up the colors differently each time we display the title screen */
   for (c = 8; c < PAL_SIZE / 2; c++) {
      rgb = ((RGB *) data[TITLE_PAL].dat)[c];
      switch (color) {
         case 0:
            rgb.b = rgb.r;
            rgb.r = 0;
            break;
         case 1:
            rgb.g = rgb.r;
            rgb.r = 0;
            break;
         case 3:
            rgb.g = rgb.r;
            break;
      }
      title_palette.rgb[c] = rgb;
   }

   for (c = PAL_SIZE / 2; c < PAL_SIZE; c++)
      title_palette.rgb[c] = ((PALETTE *)(data[TITLE_PAL].dat))->rgb[c];

   color++;
   if (color > 3)
      color = 0;

   al_clear_to_color(makecol(0, 0, 0));

   set_palette(&title_palette);

   c = 1;
   while (c < 160) {
      stretch_blit(data[TITLE_BMP].dat, 0, 0, 320, 128,
                   SCREEN_W / 2 - c, SCREEN_H / 2 - c * 64 / 160 - 32,
                   c * 2, c * 128 / 160);
      rest(5);
      c++;
   }

   blit(data[TITLE_BMP].dat, 0, 0, SCREEN_W / 2 - 160,
        SCREEN_H / 2 - 96, 320, 128);

   clear_keybuf();

   do {
      updated = 0;

      scroller();
      scroll_pos++;
      updated = 1;

      if (max_fps || updated) {
         al_clear_to_color(makecol(0, 0, 0));
         draw_scroller();
         al_flip_display();
      }

      poll_input();
      rest(1);
   } while (!keypressed()); // && (!joy[0].button[0].b) && (!joy[0].button[1].b));


   fade_out(5);

   while (keypressed())
      if ((readkey() & 0xff) == 27)
         return FALSE;

   return TRUE;
}


