/*
 *   Example program for the Allegro library, by Jon Rafkind.
 *
 *   This program demonstrates how to draw trans and lit sprites and flip them
 *   at the same time, using draw_sprite_ex() function.
 *   It displays several images moving around using different drawing modes
 *   while you can press space key to change the flipping orientation.
 */

#include <stdio.h>

#include <allegro.h>

/* define the number of sprites that will be displayed */
#define SPRITE_COUNT 20

/* define directions in which the sprites can move */
enum DIRECTION { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };


/* define our sprite */
typedef struct {
   int x;
   int y;

   enum DIRECTION direction;

   /* what order to draw this sprite at, lower before higher */
   int level;

   /* SPRITE_NORMAL, SPRITE_TRANS, or SPRITE_LIT */
   int draw_type;
} SPRITE;


/* puts the sprite somewhere on the screen and sets its initial state */
void setup_sprite(SPRITE *s, int i)
{
   switch (i % 4) {
      case 0:
         s->x = -100 + i * 50;
         s->y = 40 + (i % 3) * 100;
         s->direction = DIR_RIGHT;
         s->draw_type = DRAW_SPRITE_NORMAL;
         s->level = 0;
         break;

      case 1:
         s->x = 640 + 100 - i * 70;
         s->y = 50 + (i % 3) * 110;
         s->direction = DIR_LEFT;
         s->draw_type = DRAW_SPRITE_TRANS;
         s->level = 1;
         break;

      case 2:
         s->x = 90 + (i % 3) * 200;
         s->y = -100 + i * 70;
         s->direction = DIR_DOWN;
         s->draw_type = DRAW_SPRITE_LIT;
         s->level = 2;
         break;

      case 3:
         s->x = 50 + (i % 3) * 200;
         s->y = 480 + 100 - i * 70;
         s->direction = DIR_UP;
         s->draw_type = DRAW_SPRITE_TRANS;
         s->level = 3;
         break;
   }
}


/* used by sort function to compare sprites */
int sprite_compare(AL_CONST void * a, AL_CONST void * b)
{
   SPRITE *s1 = (SPRITE *)a;
   SPRITE *s2 = (SPRITE *)b;

   return s1->level - s2->level;
}


/* moves the sprite by one pixel acording to its direction */
void move_sprite(SPRITE *s)
{
   switch (s->direction) {
      case DIR_UP:
         s->y--;
         if (s->y < -64) {
            s->y = 480;
         }
         break;

      case DIR_DOWN:
         s->y++;
         if (s->y > 480) {
            s->y = -64;
         }
         break;

      case DIR_RIGHT:
         s->x++;
         if (s->x > 640) {
            s->x = -64;
         }
         break;

      case DIR_LEFT:
         s->x--;
         if (s->x < -64) {
            s->x = 640;
         }
         break;
   }
}


int main(int argc, char **argv)
{
   SPRITE sprites[SPRITE_COUNT];
   BITMAP *buffer;
   BITMAP *pic;
   BITMAP *tmp;
   char buf[1024];
   char *filename;
   int mode = 0;
   int hold_space = 0;
   char *msg;
   int i;

   /* standard init */
   if (allegro_init() != 0)
      return 1;
   install_timer();
   install_keyboard();

   /* setting graphics mode */
   set_color_depth(16);
   if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) != 0) {
         set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
         allegro_message("Unable to set any graphic mode\n%s\n",
                 allegro_error);
         return 1;
      }
   }

   /* set initial position, direction and drawing for all sprites */
   for (i = 0; i < SPRITE_COUNT; i++)
      setup_sprite(&sprites[i], i);

   /* sort the sprites by drawing level */
   qsort(sprites, SPRITE_COUNT, sizeof(SPRITE), sprite_compare);

   /* locate the bitmap we will use */
   replace_filename(buf, argv[0], "allegro.pcx", sizeof(buf));
   filename = buf;

   /* load the bitmap and stretch it */
   tmp = load_bitmap(filename, NULL);
   pic = create_bitmap(64, 64);
   stretch_blit(tmp, pic, 0, 0, tmp->w, tmp->h, 0, 0, pic->w, pic->h);
   destroy_bitmap(tmp);

   /* we are using double buffer mode, so create the back buffer */
   buffer = create_bitmap(screen->w, screen->h);

   set_trans_blender(128, 0, 64, 128);

   /* exit on Esc key */
   while (!key[KEY_ESC]) {
      /* move every sprite and draw it on the back buffer */
      for (i = 0; i < SPRITE_COUNT; i++) {
         SPRITE *s = &sprites[i];
         move_sprite(s);
         draw_sprite_ex(buffer, pic, s->x, s->y, s->draw_type, mode);
      }

      /* handle the space key */
      if (key[KEY_SPACE] && !hold_space) {
         hold_space = 1;
         /* switch to next flipping mode */
         switch (mode) {
            case DRAW_SPRITE_H_FLIP:
               mode = DRAW_SPRITE_V_FLIP;
               break;
            case DRAW_SPRITE_V_FLIP:
               mode = DRAW_SPRITE_VH_FLIP;
               break;
            case DRAW_SPRITE_VH_FLIP:
               mode = DRAW_SPRITE_NO_FLIP;
               break;
            case DRAW_SPRITE_NO_FLIP:
               mode = DRAW_SPRITE_H_FLIP;
               break;
         }
      }
      if (!key[KEY_SPACE]) {
         hold_space = 0;
      }

      /* set the title according to the flipping mode used */
      if (mode == DRAW_SPRITE_VH_FLIP) {
         msg = "horizontal and vertical flip";
      } else if (mode == DRAW_SPRITE_H_FLIP) {
         msg = "horizontal flip";
      } else if (mode == DRAW_SPRITE_V_FLIP) {
         msg = "vertical flip";
      } else {
         msg = "no flipping";
      }
      textout_ex(buffer, font, msg, 1, 1, makecol(255, 255, 255), -1);

      /* finally blit the back buffer on the screen */
      blit(buffer, screen, 0, 0, 0, 0, buffer->w, buffer->h);
      clear_bitmap(buffer);

      /* reduce CPU usage */
      rest(20);
   }

   /* clean up */
   destroy_bitmap(pic);
   destroy_bitmap(buffer);

   return 0;
}

END_OF_MAIN()

