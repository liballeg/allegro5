#include <allegro.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "../include/credits.h"
#include "../include/global.h"

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
   struct CREDIT_NAME *next;
} CREDIT_NAME;


static CREDIT_NAME *credits = NULL;

/* text messages (loaded from readme.txt) */
static char *title_text;
static int title_size;
static int title_alloced;
static char *end_text;

/* for the text scroller */
static int text_char;
static int text_width;
static int text_pix;
static int text_scroll;

/* for the credits display */
static CREDIT_NAME *cred;
static int credit_width;
static int credit_scroll;
static int credit_age;
static int credit_speed;
static int credit_skip;



/* helper to find a file or directory in the Allegro tree */
static int find_relative_path(char buf[], size_t bufsize, const char *name)
{
   static const char *locations[] = {
      "",
      "../",
      "../../",         /* Allegro root, no build directory */
      "../../../",      /* Allegro root, with build directory */
      "../../../../",   /* Allegro root, MSVC build configuration directory */
      NULL
   };
   char exe[256];
   char dir[256];
   int i;

   get_executable_name(exe, sizeof(exe));
   for (i = 0; locations[i] != NULL; i++) {
      replace_filename(dir, exe, locations[i], sizeof(dir));
      append_filename(buf, dir, name, bufsize);
      if (file_exists(buf, FA_ALL, NULL)) {
         return 1;
      }
   }

   return 0;
}



/* helper to open readme.txt and thanks._tx */
static PACKFILE *open_relative_file(char buf[], size_t size, const char *name)
{
   if (find_relative_path(buf, size, name))
      return pack_fopen(buf, F_READ);

   return NULL;
}



/* formats a list of TEXT_LIST structure into a single string */
char *format_text(TEXT_LIST * head, char *eol, char *gap)
{
   TEXT_LIST *l;
   int size = 0;
   char *s;

   l = head;
   while (l) {
      if (l->text[0])
         size += strlen(l->text) + strlen(eol);
      else
         size += strlen(gap) + strlen(eol);
      l = l->next;
   }

   s = malloc(size + 1);
   s[0] = 0;

   l = head;
   while (l) {
      if (l->text[0])
         strcat(s, l->text);
      else
         strcat(s, gap);
      strcat(s, eol);
      l = l->next;
   }

   return s;
}



/* loads the scroller message from readme.txt */
void load_text(void)
{
   README_SECTION sect[] = {
      {NULL, NULL, NULL, "Introduction"},
      {NULL, NULL, NULL, "Features"},
      {NULL, NULL, NULL, "Copyright"},
      {NULL, NULL, NULL, "Contact info"}
   };

#define SPLITTER  "                                "

   static char intro_msg[] =
      "Welcome to the Allegro demonstration game, by Miran Amon, Nick Davies, Elias, Thomas Harte & Jakub Wasilewski."
      SPLITTER
      "Help skateboarding Ted collect various shop items - see \"About\" for more information!"
      SPLITTER;

   static char splitter[] = SPLITTER;
   static char marker[] = "--------";
   char buf[256];
   README_SECTION *sec = NULL;
   TEXT_LIST *l, *p;
   PACKFILE *f;
   int inblank = TRUE;
   char *s;
   int i;

   f = open_relative_file(buf, sizeof(buf), "docs/txt/readme.txt");
   if (!f) {
      title_text =
         "Can't find readme.txt, so this scroller is empty.                ";
      title_size = strlen(title_text);
      title_alloced = FALSE;
      end_text = NULL;
      return;
   }

   while (pack_fgets(buf, sizeof(buf) - 1, f) != 0) {
      if (buf[0] == '=') {
         s = strchr(buf, ' ');
         if (s) {
            for (i = strlen(s) - 1; (uisspace(s[i])) || (s[i] == '='); i--)
               s[i] = 0;

            s++;

            sec = NULL;
            inblank = TRUE;

            for (i = 0; i < (int)(sizeof(sect) / sizeof(sect[0])); i++) {
               if (stricmp(s, sect[i].desc) == 0) {
                  sec = &sect[i];
                  break;
               }
            }
         }
      } else if (sec) {
         s = buf;

         while ((*s) && (uisspace(*s)))
            s++;

         for (i = strlen(s) - 1; (i >= 0) && (uisspace(s[i])); i--)
            s[i] = 0;

         if ((s[0]) || (!inblank)) {
            l = malloc(sizeof(TEXT_LIST));
            l->next = NULL;
            l->text = malloc(strlen(s) + 1);
            strcpy(l->text, s);

            if (sec->tail)
               sec->tail->next = l;
            else
               sec->head = l;

            sec->tail = l;
         }

         inblank = (s[0] == 0);
      }
   }

   pack_fclose(f);

   if (sect[2].head)
      end_text = format_text(sect[2].head, "\n", "");
   else
      end_text = NULL;

   title_size = strlen(intro_msg);

   for (i = 0; i < (int)(sizeof(sect) / sizeof(sect[0])); i++) {
      if (sect[i].head) {
         sect[i].flat = format_text(sect[i].head, " ", splitter);
         title_size +=
            strlen(sect[i].flat) + strlen(sect[i].desc) +
            strlen(splitter) + strlen(marker) * 2 + 2;
      }
   }

   title_text = malloc(title_size + 1);
   title_alloced = TRUE;

   strcpy(title_text, intro_msg);

   for (i = 0; i < (int)(sizeof(sect) / sizeof(sect[0])); i++) {
      if (sect[i].flat) {
         strcat(title_text, marker);
         strcat(title_text, " ");
         strcat(title_text, sect[i].desc);
         strcat(title_text, " ");
         strcat(title_text, marker);
         strcat(title_text, splitter);
         strcat(title_text, sect[i].flat);
      }
   }

   for (i = 0; i < (int)(sizeof(sect) / sizeof(sect[0])); i++) {
      l = sect[i].head;
      while (l) {
         free(l->text);
         p = l;
         l = l->next;
         free(p);
      }
      if (sect[i].flat)
         free(sect[i].flat);
   }
}


/* sorts a list of credit strings */
void sort_credit_list(void)
{
   CREDIT_NAME **prev, *p;
   int n, done;

   do {
      done = TRUE;

      prev = &credits;
      p = credits;

      while ((p) && (p->next)) {
         n = 0;

         if (n == 0) {
            n = stricmp(p->name, p->next->name);
         }

         if (n > 0) {
            *prev = p->next;
            p->next = p->next->next;
            (*prev)->next = p;
            p = *prev;

            done = FALSE;
         }

         prev = &p->next;
         p = p->next;
      }
   } while (!done);
}


/* reads credit info from various places */
void load_credits(void)
{
   char buf[256], *p, *p2;
   CREDIT_NAME *c = NULL;
   PACKFILE *f;

   /* parse thanks._tx */
   f = open_relative_file(buf, sizeof(buf), "docs/src/thanks._tx");
   if (!f)
      return;

   while (pack_fgets(buf, sizeof(buf) - 1, f) != 0) {
      if (stricmp(buf, "Thanks!") == 0)
         break;

      while ((p = strstr(buf, "&lt")) != NULL) {
         *p = '<';
         memmove(p + 1, p + 3, strlen(p + 2));
      }

      while ((p = strstr(buf, "&gt")) != NULL) {
         *p = '>';
         memmove(p + 1, p + 3, strlen(p + 2));
      }

      p = buf;

      while ((*p) && (uisspace(*p)))
         p++;

      p2 = p;

      while ((*p2) && ((!uisspace(*p2)) || (*(p2 + 1) != '(')))
         p2++;

      if ((strncmp(p2, " (<email>", 9) == 0) ||
          (strncmp(p2, " (email", 7) == 0)) {
         *p2 = 0;

         c = malloc(sizeof(CREDIT_NAME));

         c->name = malloc(strlen(p) + 1);
         strcpy(c->name, p);

         c->text = NULL;

         c->next = credits;
         credits = c;
      } else if (*p) {
         if (c) {
            p2 = p + strlen(p) - 1;
            while ((p2 > p) && (uisspace(*p2)))
               *(p2--) = 0;

            if (c->text) {
               c->text = realloc(c->text, strlen(c->text) + strlen(p) + 2);
               strcat(c->text, " ");
               strcat(c->text, p);
            } else {
               c->text = malloc(strlen(p) + 1);
               strcpy(c->text, p);
            }
         }
      } else
         c = NULL;
   }

   pack_fclose(f);

   /* sort the lists */
   sort_credit_list();
}


CREDIT_NAME *next_credit(CREDIT_NAME * cred)
{
   int max, i;

   if (!cred) {
      cred = credits;
      if (!cred)
         return NULL;
   }

   max = rand() % 1000;
   for (i = 0; i < max; i++) {
      if (cred->next) {
         cred = cred->next;
      } else {
         cred = credits;
      }
   }

   return cred;
}


void init_credits(void)
{
   /* for the text scroller */
   text_char = 0xFFFF;
   text_width = 0;
   text_pix = 0;
   text_scroll = 0;

   /* for the credits display */
   cred = NULL;
   credit_width = 0;
   credit_scroll = 0;
   credit_age = 0;
   credit_speed = 32;
   credit_skip = 1;

   load_text();
   load_credits();
}


void update_credits(void)
{
   /* move the scroller */
   text_scroll++;

   /* update the credits position */
   if (credit_scroll <= 0) {
      cred = next_credit(cred);

      if (cred) {
         credit_width = text_length(demo_font, cred->name) + 24;

         if (cred->text) {
            credit_scroll =
               text_length(plain_font, cred->text) + SCREEN_W - credit_width;
         } else {
            credit_scroll = 256;
         }

         credit_age = 0;
      }
   } else {
      credit_scroll--;
      credit_age++;
   }
}


void draw_credits(BITMAP *canvas)
{
   int c, c2;
   int y2;
   int col_back, col_font;

   /* for the text scroller */
   char buf[2] = " ";

   /* for the credits display */
   char cbuf[2] = " ";
   char *p;

   col_back = makecol(222, 222, 222);
   col_font = makecol(96, 96, 96);

   /* draw the text scroller */
   y2 = SCREEN_H - text_height(demo_font);
   text_pix = -(text_scroll / 1);
   hline(canvas, 0, y2 - 2, SCREEN_W - 1, col_font);
   hline(canvas, 0, y2 - 1, SCREEN_W - 1, col_font);
   rectfill(canvas, 0, y2, SCREEN_W - 1, SCREEN_H - 1, col_back);
   for (text_char = 0; text_char < title_size; text_char++) {
      buf[0] = title_text[text_char];
      c = text_length(demo_font, buf);

      if (text_pix + c > 0) {
         demo_textout(canvas, demo_font, buf, text_pix, y2, col_font, -1);
      }

      text_pix += c;

      if (text_pix >= SCREEN_W) {
         break;
      }
   }

   /* draw author name/desc credits */
   y2 = text_height(demo_font);
   rectfill(canvas, 0, 0, SCREEN_W - 1, y2, col_back);
   hline(canvas, 0, y2 + 1, SCREEN_W - 1, col_font);
   hline(canvas, 0, y2 + 2, SCREEN_W - 1, col_font);
   y2 = (text_height(demo_font) - 8) / 2;

   if (cred && cred->text) {
      c = credit_scroll;
      p = cred->text;
      c2 = strlen(p);

      if (c > 0) {
         if (c2 > c / 8) {
            p += c2 - c / 8;
            c &= 7;
         } else {
            c -= c2 * 8;
         }

         c += credit_width;

         while ((*p) && (c < SCREEN_W - 32)) {
            if (c < credit_width + 96) {
               c2 = 128 + (c - credit_width - 32) * 127 / 64;
            } else if (c > SCREEN_W - 96) {
               c2 = 128 + (SCREEN_W - 32 - c) * 127 / 64;
            } else {
               c2 = 255;
            }

            if ((c2 > 128) && (c2 <= 255)) {
               c2 = 255 - c2 + 64;
               cbuf[0] = *p;
               demo_textout(canvas, plain_font, cbuf, c, y2, makecol(c2, c2, c2), -1);
            }

            p++;
            c += 8;
         }
      }
   }

   c = 4;

   if (credit_age < 100) {
      c -= (100 - credit_age) * (100 - credit_age) * credit_width / 10000;
   }

   if (credit_scroll < 150) {
      c += (150 - credit_scroll) * (150 - credit_scroll) * SCREEN_W / 22500;
   }

   if (cred)
      demo_textprintf(canvas, demo_font, c, 0, col_font, -1, "%s:", cred->name);
   else
      demo_textprintf(canvas, demo_font, 0, 0, col_font, -1,
                    "thanks._tx not found!");
}


void destroy_credits(void)
{
   if ((title_text) && (title_alloced)) {
      free(title_text);
   }

   while (credits) {
      cred = credits;
      credits = cred->next;

      if (cred->name) {
         free(cred->name);
      }

      if (cred->text) {
         free(cred->text);
      }

      free(cred);
   }
}
