/*
 *    Haiku - A Musical Instrument, by Mark Oates.
 *
 *    Allegro example version by Peter Wang.
 *
 *    It demonstrates use of the audio functions, and other things besides.
 */

/* This version leaves out some things from Mark's original version:
 * the nice title sequence, text labels and mouse cursors.
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_image.h>

#include "common.c"

const float PI    = ALLEGRO_PI;
const float TWOPI = ALLEGRO_PI * 2.0;

enum {
   TYPE_EARTH,
   TYPE_WIND,
   TYPE_WATER,
   TYPE_FIRE,
   NUM_TYPES,
   TYPE_NONE = NUM_TYPES
};

enum {
   IMG_EARTH = TYPE_EARTH,
   IMG_WIND  = TYPE_WIND,
   IMG_WATER = TYPE_WATER,
   IMG_FIRE  = TYPE_FIRE,
   IMG_BLACK,
   IMG_DROPSHADOW,
   IMG_GLOW,
   IMG_GLOW_OVERLAY,
   IMG_AIR_EFFECT,
   IMG_WATER_DROPS,
   IMG_FLAME,
   IMG_MAIN_FLAME,
   IMG_MAX
};

typedef enum {
   INTERP_LINEAR,
   INTERP_FAST,
   INTERP_DOUBLE_FAST,
   INTERP_SLOW,
   INTERP_DOUBLE_SLOW,
   INTERP_SLOW_IN_OUT,
   INTERP_BOUNCE
} Interp;

typedef struct Anim Anim;
typedef struct Token Token;
typedef struct Flair Flair;
typedef struct Sprite Sprite;

#define MAX_ANIMS 10

struct Anim {
   float       *lval;   /* NULL if unused */
   float       start_val;
   float       end_val;
   Interp      func;
   float       start_time;
   float       end_time;
};

struct Sprite {
   unsigned    image;   /* IMG_ */
   float       x, scale_x, align_x;
   float       y, scale_y, align_y;
   float       angle;
   float       r, g, b;
   float       opacity;
   Anim        anims[MAX_ANIMS]; /* keep it simple */
};

struct Token {
   unsigned    type;    /* TYPE_ */
   float       x;
   float       y;
   int         pitch;   /* [0, NUM_PITCH) */
   Sprite      bot;
   Sprite      top;
};

struct Flair {
   Flair       *next;
   float       end_time;
   Sprite      sprite;
};

/****************************************************************************/
/* Globals                                                                  */
/****************************************************************************/

enum {
   NUM_PITCH   = 8,
   TOKENS_X    = 16,
   TOKENS_Y    = NUM_PITCH,
   NUM_TOKENS  = TOKENS_X * TOKENS_Y,
};

ALLEGRO_DISPLAY *display;
ALLEGRO_TIMER  *refresh_timer;
ALLEGRO_TIMER  *playback_timer;

ALLEGRO_BITMAP *images[IMG_MAX];
ALLEGRO_SAMPLE *element_samples[NUM_TYPES][NUM_PITCH];
ALLEGRO_SAMPLE *select_sample;

Token          tokens[NUM_TOKENS];
Token          buttons[NUM_TYPES];
Sprite         glow;
Sprite         glow_overlay;
ALLEGRO_COLOR  glow_color[NUM_TYPES];
Flair          *flairs = NULL;
Token          *hover_token = NULL;
Token          *selected_button = NULL;
int            playback_column = 0;

const int      screen_w = 1024;
const int      screen_h = 600;
const float    game_board_x = 100.0;
const float    token_size = 64;
const float    token_scale = 0.8;
const float    button_size = 64;
const float    button_unsel_scale = 0.8;
const float    button_sel_scale = 1.1;
const float    dropshadow_unsel_scale = 0.6;
const float    dropshadow_sel_scale = 0.9;
const float    refresh_rate = 60.0;
const float    playback_period = 2.7333;

#define HAIKU_DATA "data/haiku/"

/****************************************************************************/
/* Init                                                                     */
/****************************************************************************/

static void load_images(void)
{
   int i;

   images[IMG_EARTH]       = al_load_bitmap(HAIKU_DATA "earth4.png");
   images[IMG_WIND]        = al_load_bitmap(HAIKU_DATA "wind3.png");
   images[IMG_WATER]       = al_load_bitmap(HAIKU_DATA "water.png");
   images[IMG_FIRE]        = al_load_bitmap(HAIKU_DATA "fire.png");
   images[IMG_BLACK]       = al_load_bitmap(HAIKU_DATA "black_bead_opaque_A.png");
   images[IMG_DROPSHADOW]  = al_load_bitmap(HAIKU_DATA "dropshadow.png");
   images[IMG_AIR_EFFECT]  = al_load_bitmap(HAIKU_DATA "air_effect.png");
   images[IMG_WATER_DROPS] = al_load_bitmap(HAIKU_DATA "water_droplets.png");
   images[IMG_FLAME]       = al_load_bitmap(HAIKU_DATA "flame2.png");
   images[IMG_MAIN_FLAME]  = al_load_bitmap(HAIKU_DATA "main_flame2.png");
   images[IMG_GLOW]        = al_load_bitmap(HAIKU_DATA "healthy_glow.png");
   images[IMG_GLOW_OVERLAY]= al_load_bitmap(HAIKU_DATA "overlay_pretty.png");

   for (i = 0; i < IMG_MAX; i++) {
      if (images[i] == NULL)
         abort_example("Error loading image.\n");
   }
}

static void load_samples(void)
{
   const char *base[NUM_TYPES] = {"earth", "air", "water", "fire"};
   char name[128];
   int t, p;

   for (t = 0; t < NUM_TYPES; t++) {
      for (p = 0; p < NUM_PITCH; p++) {
         sprintf(name, HAIKU_DATA "%s_%d.ogg", base[t], p);
         element_samples[t][p] = al_load_sample(name);
         if (!element_samples[t][p])
            abort_example("Error loading %s.\n", name);
      }
   }

   select_sample = al_load_sample(HAIKU_DATA "select.ogg");
   if (!select_sample)
      abort_example("Error loading select.ogg.\n");
}

static void init_sprite(Sprite *spr, int image, float x, float y, float scale,
   float opacity)
{
   int i;

   spr->image = image;
   spr->x = x;
   spr->y = y;
   spr->scale_x = spr->scale_y = scale;
   spr->align_x = spr->align_y = 0.5;
   spr->angle = 0.0;
   spr->r = spr->g = spr->b = 1.0;
   spr->opacity = opacity;
   for (i = 0; i < MAX_ANIMS; i++)
      spr->anims[i].lval = NULL;
}

static void init_tokens(void)
{
   const float token_w = token_size * token_scale;
   const float token_x = game_board_x + token_w/2.0;
   const float token_y = 80;
   int i;

   for (i = 0; i < NUM_TOKENS; i++) {
      int tx = i % TOKENS_X;
      int ty = i / TOKENS_X;
      float px = token_x + tx * token_w;
      float py = token_y + ty * token_w;

      tokens[i].type = TYPE_NONE;
      tokens[i].x = px;
      tokens[i].y = py;
      tokens[i].pitch = NUM_PITCH - 1 - ty;
      assert(tokens[i].pitch >= 0 && tokens[i].pitch < NUM_PITCH);
      init_sprite(&tokens[i].bot, IMG_BLACK, px, py, token_scale, 0.4);
      init_sprite(&tokens[i].top, IMG_BLACK, px, py, token_scale, 0.0);
   }
}

static void init_buttons(void)
{
   const float dist[NUM_TYPES] = {-1.5, -0.5, 0.5, 1.5};
   int i;

   for (i = 0; i < NUM_TYPES; i++) {
      float x = screen_w/2 + 150 * dist[i];
      float y = screen_h - 80;

      buttons[i].type = i;
      buttons[i].x = x;
      buttons[i].y = y;
      init_sprite(&buttons[i].bot, IMG_DROPSHADOW, x, y,
         dropshadow_unsel_scale, 0.4);
      buttons[i].bot.align_y = 0.0;
      init_sprite(&buttons[i].top, i, x, y, button_unsel_scale, 1.0);
   }
}

static void init_glow(void)
{
   init_sprite(&glow, IMG_GLOW, screen_w/2, screen_h, 1.0, 1.0);
   glow.align_y = 1.0;
   glow.r = glow.g = glow.b = 0.0;

   init_sprite(&glow_overlay, IMG_GLOW_OVERLAY, 0.0, 0.0, 1.0, 1.0);
   glow_overlay.align_x = 0.0;
   glow_overlay.align_y = 0.0;
   glow_overlay.r = glow_overlay.g = glow_overlay.b = 0.0;

   glow_color[TYPE_EARTH] = al_map_rgb(0x6b, 0x8e, 0x23); /* olivedrab */
   glow_color[TYPE_WIND]  = al_map_rgb(0xad, 0xd8, 0xe6); /* lightblue */
   glow_color[TYPE_WATER] = al_map_rgb(0x41, 0x69, 0xe1); /* royalblue */
   glow_color[TYPE_FIRE]  = al_map_rgb(0xff, 0x00, 0x00); /* red */
}

/****************************************************************************/
/* Flairs                                                                   */
/****************************************************************************/

static Sprite *make_flair(int image, float x, float y, float end_time)
{
   Flair *fl = malloc(sizeof *fl);
   init_sprite(&fl->sprite, image, x, y, 1.0, 1.0);
   fl->end_time = end_time;
   fl->next = flairs;
   flairs = fl;
   return &fl->sprite;
}

static void free_old_flairs(float now)
{
   Flair *prev, *fl, *next;

   prev = NULL;
   for (fl = flairs; fl != NULL; fl = next) {
      next = fl->next;
      if (fl->end_time > now)
         prev = fl;
      else {
         if (prev)
            prev->next = next;
         else
            flairs = next;
         free(fl);
      }
   }
}

static void free_all_flairs(void)
{
   Flair *next;

   for (; flairs != NULL; flairs = next) {
      next = flairs->next;
      free(flairs);
   }
}

/****************************************************************************/
/* Animations                                                               */
/****************************************************************************/

static Anim *get_next_anim(Sprite *spr)
{
   static Anim dummy_anim;
   unsigned i;

   for (i = 0; i < MAX_ANIMS; i++) {
      if (spr->anims[i].lval == NULL)
         return &spr->anims[i];
   }

   assert(false);
   return &dummy_anim;
}

static void fix_conflicting_anims(Sprite *grp, float *lval, float start_time,
   float start_val)
{
   unsigned i;

   for (i = 0; i < MAX_ANIMS; i++) {
      Anim *anim = &grp->anims[i];

      if (anim->lval != lval)
         continue;

      /* If an old animation would overlap with the new one, truncate it
       * and make it converge to the new animation's starting value.
       */
      if (anim->end_time > start_time) {
         anim->end_time = start_time;
         anim->end_val = start_val;
      }

      /* Cancel any old animations which are scheduled to start after the
       * new one, or which have been reduced to nothing.
       */
      if (anim->start_time >= start_time ||
         anim->start_time >= anim->end_time)
      {
         grp->anims[i].lval = NULL;
      }
   }
}

static void anim_full(Sprite *spr, float *lval, float start_val, float end_val,
   Interp func, float delay, float duration)
{
   float start_time;
   Anim *anim;

   start_time = al_get_time() + delay;
   fix_conflicting_anims(spr, lval, start_time, start_val);

   anim = get_next_anim(spr);
   anim->lval = lval;
   anim->start_val = start_val;
   anim->end_val = end_val;
   anim->func = func;
   anim->start_time = start_time;
   anim->end_time = start_time + duration;
}

static void anim(Sprite *spr, float *lval, float start_val, float end_val,
   Interp func, float duration)
{
   anim_full(spr, lval, start_val, end_val, func, 0.0, duration);
}

static void anim_to(Sprite *spr, float *lval, float end_val,
   Interp func, float duration)
{
   anim_full(spr, lval, *lval, end_val, func, 0.0, duration);
}

static void anim_delta(Sprite *spr, float *lval, float delta,
   Interp func, float duration)
{
   anim_full(spr, lval, *lval, *lval + delta, func, 0.0, duration);
}

static void anim_tint(Sprite *spr, const ALLEGRO_COLOR color, Interp func,
   float duration)
{
   float r, g, b;

   al_unmap_rgb_f(color, &r, &g, &b);
   anim_to(spr, &spr->r, r, func, duration);
   anim_to(spr, &spr->g, g, func, duration);
   anim_to(spr, &spr->b, b, func, duration);
}

static float interpolate(Interp func, float t)
{
   switch (func) {

   case INTERP_LINEAR:
      return t;

   case INTERP_FAST:
      return -t*(t-2);

   case INTERP_DOUBLE_FAST:
      t--;
      return t*t*t + 1;

   case INTERP_SLOW:
      return t*t;

   case INTERP_DOUBLE_SLOW:
      return t*t*t;

   case INTERP_SLOW_IN_OUT: {
      // Quadratic easing in/out - acceleration until halfway, then deceleration
      float b = 0;
      float c = 1;
      float d = 1;
      t /= d/2;
      if (t < 1) {
         return c/2 * t * t + b;
      } else {
         t--;
         return -c/2 * (t*(t-2) - 1) + b;
      }
   }

   case INTERP_BOUNCE: {
      // BOUNCE EASING: exponentially decaying parabolic bounce
      // t: current time, b: beginning value, c: change in position, d: duration
      // bounce easing out
      if (t < (1/2.75))
         return (7.5625*t*t);
      if (t < (2/2.75)) {
         t -= (1.5/2.75);
         return (7.5625*t*t + 0.75);
      }
      if (t < (2.5/2.75)) {
         t -= (2.25/2.75);
         return (7.5625*t*t + 0.9375);
      }
      t -= (2.625/2.75);
      return (7.5625*t*t + 0.984375);
   }

   default:
      assert(false);
      return 0.0;
   }
}

static void update_anim(Anim *anim, float now)
{
   float dt, t, range;

   if (!anim->lval)
      return;

   if (now < anim->start_time)
      return;

   dt = now - anim->start_time;
   t = dt / (anim->end_time - anim->start_time);

   if (t >= 1.0) {
      /* animation has run to completion */
      *anim->lval = anim->end_val;
      anim->lval = NULL;
      return;
   }

   range = anim->end_val - anim->start_val;
   *anim->lval = anim->start_val + interpolate(anim->func, t) * range;
}

static void update_sprite_anims(Sprite *spr, float now)
{
   int i;

   for (i = 0; i < MAX_ANIMS; i++)
      update_anim(&spr->anims[i], now);
}

static void update_token_anims(Token *token, float now)
{
   update_sprite_anims(&token->bot, now);
   update_sprite_anims(&token->top, now);
}

static void update_anims(float now)
{
   Flair *fl;
   int i;

   for (i = 0; i < NUM_TOKENS; i++)
      update_token_anims(&tokens[i], now);

   for (i = 0; i < NUM_TYPES; i++)
      update_token_anims(&buttons[i], now);

   update_sprite_anims(&glow, now);
   update_sprite_anims(&glow_overlay, now);

   for (fl = flairs; fl != NULL; fl = fl->next)
      update_sprite_anims(&fl->sprite, now);
}

/****************************************************************************/
/* Drawing                                                                  */
/****************************************************************************/

static void draw_sprite(const Sprite *spr)
{
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_COLOR tint;
   float cx, cy;

   bmp = images[spr->image];
   cx = spr->align_x * al_get_bitmap_width(bmp);
   cy = spr->align_y * al_get_bitmap_height(bmp);
   tint = al_map_rgba_f(spr->r, spr->g, spr->b, spr->opacity);

   al_draw_tinted_scaled_rotated_bitmap(bmp, tint, cx, cy,
      spr->x, spr->y, spr->scale_x, spr->scale_y, spr->angle, 0);
}

static void draw_token(const Token *token)
{
   draw_sprite(&token->bot);
   draw_sprite(&token->top);
}

static void draw_screen(void)
{
   Flair *fl;
   int i;

   al_clear_to_color(al_map_rgb(0, 0, 0));

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_ONE);

   draw_sprite(&glow);
   draw_sprite(&glow_overlay);

   for (i = 0; i < NUM_TOKENS; i++)
      draw_token(&tokens[i]);

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);

   for (i = 0; i < NUM_TYPES; i++)
      draw_token(&buttons[i]);

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_ONE);

   for (fl = flairs; fl != NULL; fl = fl->next)
      draw_sprite(&fl->sprite);

   al_flip_display();
}

/****************************************************************************/
/* Playback                                                                 */
/****************************************************************************/

static void spawn_wind_effects(float x, float y)
{
   const float now = al_get_time();
   Sprite *spr;

   spr = make_flair(IMG_AIR_EFFECT, x, y, now + 1.0);
   anim(spr, &spr->scale_x, 0.9, 1.3, INTERP_FAST, 1.0);
   anim(spr, &spr->scale_y, 0.9, 1.3, INTERP_FAST, 1.0);
   anim(spr, &spr->opacity, 1.0, 0.0, INTERP_FAST, 1.0);

   spr = make_flair(IMG_AIR_EFFECT, x, y, now + 1.2);
   anim(spr, &spr->opacity, 1.0, 0.0, INTERP_LINEAR, 1.2);
   anim(spr, &spr->scale_x, 1.1, 1.5, INTERP_FAST, 1.2);
   anim(spr, &spr->scale_y, 1.1, 0.5, INTERP_FAST, 1.2);
   anim_delta(spr, &spr->x, 10.0, INTERP_FAST, 1.2);
}

static void spawn_fire_effects(float x, float y)
{
   const float now = al_get_time();
   Sprite *spr;
   int i;

   spr = make_flair(IMG_MAIN_FLAME, x, y, now + 0.8);
   spr->align_y = 0.75;
   anim_full(spr, &spr->scale_x, 0.2, 1.3, INTERP_BOUNCE, 0.0, 0.4);
   anim_full(spr, &spr->scale_y, 0.2, 1.3, INTERP_BOUNCE, 0.0, 0.4);
   anim_full(spr, &spr->scale_x, 1.3, 1.4, INTERP_BOUNCE, 0.4, 0.5);
   anim_full(spr, &spr->scale_y, 1.3, 1.4, INTERP_BOUNCE, 0.4, 0.5);
   anim_full(spr, &spr->opacity, 1.0, 0.0, INTERP_FAST, 0.3, 0.5);

   for (i = 0; i < 3; i++) {
      spr = make_flair(IMG_FLAME, x, y, now + 0.7);
      spr->align_x = 1.3;
      spr->angle = TWOPI / 3 * i;

      anim_delta(spr, &spr->angle, -PI, INTERP_DOUBLE_FAST, 0.7);
      anim(spr, &spr->opacity, 1.0, 0.0, INTERP_SLOW, 0.7);
      anim(spr, &spr->scale_x, 0.2, 1.0, INTERP_FAST, 0.7);
      anim(spr, &spr->scale_y, 0.2, 1.0, INTERP_FAST, 0.7);
   }
}

static float random_sign(void)
{
   return (rand() % 2) ? -1.0 : 1.0;
}

static float random_float(float min, float max)
{
   return ((float) rand()/RAND_MAX)*(max-min) + min;
}

static void spawn_water_effects(float x, float y)
{
#define RAND(a, b)      (random_float((a), (b)))
#define MRAND(a, b)     (random_float((a), (b)) * max_duration)
#define SIGN            (random_sign())

   float    now = al_get_time();
   float    max_duration = 1.0;
   Sprite   *spr;
   int      i;

   spr = make_flair(IMG_WATER, x, y, now + max_duration);
   anim(spr, &spr->scale_x, 1.0, 2.0, INTERP_FAST, 0.5);
   anim(spr, &spr->scale_y, 1.0, 2.0, INTERP_FAST, 0.5);
   anim(spr, &spr->opacity, 0.5, 0.0, INTERP_FAST, 0.5);

   for (i = 0; i < 9; i++) {
      spr = make_flair(IMG_WATER_DROPS, x, y, now + max_duration);
      spr->scale_x = RAND(0.3, 1.2) * SIGN;
      spr->scale_y = RAND(0.3, 1.2) * SIGN;
      spr->angle = RAND(0.0, TWOPI);
      spr->r = RAND(0.0, 0.6);
      spr->g = RAND(0.4, 0.6);
      spr->b = 1.0;

      if (i == 0) {
         anim_to(spr, &spr->opacity, 0.0, INTERP_LINEAR, max_duration);
      } else {
         anim_to(spr, &spr->opacity, 0.0, INTERP_DOUBLE_SLOW, MRAND(0.7, 1.0));
      }
      anim_to(spr, &spr->scale_x, RAND(0.8, 3.0), INTERP_FAST, MRAND(0.7, 1.0));
      anim_to(spr, &spr->scale_y, RAND(0.8, 3.0), INTERP_FAST, MRAND(0.7, 1.0));
      anim_delta(spr, &spr->x, MRAND(0, 20.0)*SIGN, INTERP_FAST, MRAND(0.7, 1.0));
      anim_delta(spr, &spr->y, MRAND(0, 20.0)*SIGN, INTERP_FAST, MRAND(0.7, 1.0));
   }

#undef RAND
#undef MRAND
#undef SIGN
}

static void play_element(int type, int pitch, float vol, float pan)
{
   al_play_sample(element_samples[type][pitch], vol, pan, 1.0,
      ALLEGRO_PLAYMODE_ONCE, NULL);
}

static void activate_token(Token *token)
{
   const float sc = token_scale;
   Sprite *spr = &token->top;

   switch (token->type) {

   case TYPE_EARTH:
      play_element(TYPE_EARTH, token->pitch, 0.8, 0.0);
      anim(spr, &spr->scale_x, spr->scale_x+0.4, spr->scale_x, INTERP_FAST, 0.3);
      anim(spr, &spr->scale_y, spr->scale_y+0.4, spr->scale_y, INTERP_FAST, 0.3);
      break;

   case TYPE_WIND:
      play_element(TYPE_WIND, token->pitch, 0.8, 0.0);
      anim_full(spr, &spr->scale_x, sc*1.0, sc*0.8, INTERP_SLOW_IN_OUT, 0.0, 0.5);
      anim_full(spr, &spr->scale_x, sc*0.8, sc*1.0, INTERP_SLOW_IN_OUT, 0.5, 0.8);
      anim_full(spr, &spr->scale_y, sc*1.0, sc*0.8, INTERP_SLOW_IN_OUT, 0.0, 0.5);
      anim_full(spr, &spr->scale_y, sc*0.8, sc*1.0, INTERP_SLOW_IN_OUT, 0.5, 0.8);
      spawn_wind_effects(spr->x, spr->y);
      break;

   case TYPE_WATER:
      play_element(TYPE_WATER, token->pitch, 0.7, 0.5);
      anim_full(spr, &spr->scale_x, sc*1.3, sc*0.8, INTERP_BOUNCE, 0.0, 0.5);
      anim_full(spr, &spr->scale_x, sc*0.8, sc*1.0, INTERP_BOUNCE, 0.5, 0.5);
      anim_full(spr, &spr->scale_y, sc*0.8, sc*1.3, INTERP_BOUNCE, 0.0, 0.5);
      anim_full(spr, &spr->scale_y, sc*1.3, sc*1.0, INTERP_BOUNCE, 0.5, 0.5);
      spawn_water_effects(spr->x, spr->y);
      break;

   case TYPE_FIRE:
      play_element(TYPE_FIRE, token->pitch, 0.8, 0.0);
      anim(spr, &spr->scale_x, sc*1.3, sc, INTERP_SLOW_IN_OUT, 1.0);
      anim(spr, &spr->scale_y, sc*1.3, sc, INTERP_SLOW_IN_OUT, 1.0);
      spawn_fire_effects(spr->x, spr->y);
      break;
   }
}

static void update_playback(void)
{
   int y;

   for (y = 0; y < TOKENS_Y; y++)
      activate_token(&tokens[y * TOKENS_X + playback_column]);

   if (++playback_column >= TOKENS_X)
      playback_column = 0;
}

/****************************************************************************/
/* Control                                                                  */
/****************************************************************************/

static bool is_touched(Token *token, float size, float x, float y)
{
   float half = size/2.0;
   return (token->x - half <= x && x < token->x + half
       &&  token->y - half <= y && y < token->y + half);
}

static Token *get_touched_token(float x, float y)
{
   int i;

   for (i = 0; i < NUM_TOKENS; i++) {
      if (is_touched(&tokens[i], token_size, x, y))
         return &tokens[i];
   }

   return NULL;
}

static Token *get_touched_button(float x, float y)
{
   int i;

   for (i = 0; i < NUM_TYPES; i++) {
      if (is_touched(&buttons[i], button_size, x, y))
         return &buttons[i];
   }

   return NULL;
}

static void select_token(Token *token)
{
   if (token->type == TYPE_NONE && selected_button) {
      Sprite *spr = &token->top;
      spr->image = selected_button->type;
      anim_to(spr, &spr->opacity, 1.0, INTERP_FAST, 0.15);
      token->type = selected_button->type;
   }
}

static void unselect_token(Token *token)
{
   if (token->type != TYPE_NONE) {
      Sprite *spr = &token->top;
      anim_full(spr, &spr->opacity, spr->opacity, 0.0, INTERP_SLOW, 0.15, 0.15);
      token->type = TYPE_NONE;
   }
}

static void unselect_all_tokens(void)
{
   int i;

   for (i = 0; i < NUM_TOKENS; i++)
      unselect_token(&tokens[i]);
}

static void change_healthy_glow(int type, float x)
{
   anim_tint(&glow, glow_color[type], INTERP_SLOW_IN_OUT, 3.0);
   anim_to(&glow, &glow.x, x, INTERP_SLOW_IN_OUT, 3.0);

   anim_tint(&glow_overlay, glow_color[type], INTERP_SLOW_IN_OUT, 4.0);
   anim_to(&glow_overlay, &glow_overlay.opacity, 1.0, INTERP_SLOW_IN_OUT, 4.0);
}

static void select_button(Token *button)
{
   Sprite *spr;

   if (button == selected_button)
      return;

   if (selected_button) {
      spr = &selected_button->top;
      anim_to(spr, &spr->scale_x, button_unsel_scale, INTERP_SLOW, 0.3);
      anim_to(spr, &spr->scale_y, button_unsel_scale, INTERP_SLOW, 0.3);
      anim_to(spr, &spr->opacity, 0.5, INTERP_DOUBLE_SLOW, 0.2);

      spr = &selected_button->bot;
      anim_to(spr, &spr->scale_x, dropshadow_unsel_scale, INTERP_SLOW, 0.3);
      anim_to(spr, &spr->scale_y, dropshadow_unsel_scale, INTERP_SLOW, 0.3);
   }

   selected_button = button;

   spr = &button->top;
   anim_to(spr, &spr->scale_x, button_sel_scale, INTERP_FAST, 0.3);
   anim_to(spr, &spr->scale_y, button_sel_scale, INTERP_FAST, 0.3);
   anim_to(spr, &spr->opacity, 1.0, INTERP_FAST, 0.3);

   spr = &button->bot;
   anim_to(spr, &spr->scale_x, dropshadow_sel_scale, INTERP_FAST, 0.3);
   anim_to(spr, &spr->scale_y, dropshadow_sel_scale, INTERP_FAST, 0.3);

   change_healthy_glow(button->type, button->x);

   al_play_sample(select_sample, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
}

static void on_mouse_down(float x, float y, int mbut)
{
   Token *token;
   Token *button;

   if (mbut == 1) {
      if ((token = get_touched_token(x, y)))
         select_token(token);
      else if ((button = get_touched_button(x, y)))
         select_button(button);
   }
   else if (mbut == 2) {
      if ((token = get_touched_token(x, y)))
         unselect_token(token);
   }
}

static void on_mouse_axes(float x, float y)
{
   Token *token = get_touched_token(x, y);

   if (token == hover_token)
      return;

   if (hover_token) {
      Sprite *spr = &hover_token->bot;
      anim_to(spr, &spr->opacity, 0.4, INTERP_DOUBLE_SLOW, 0.2);
   }

   hover_token = token;

   if (hover_token) {
      Sprite *spr = &hover_token->bot;
      anim_to(spr, &spr->opacity, 0.7, INTERP_FAST, 0.2);
   }
}

static void main_loop(ALLEGRO_EVENT_QUEUE *queue)
{
   ALLEGRO_EVENT event;
   bool redraw = true;

   for (;;) {
      if (redraw && al_is_event_queue_empty(queue)) {
         float now = al_get_time();
         free_old_flairs(now);
         update_anims(now);
         draw_screen();
         redraw = false;
      }

      al_wait_for_event(queue, &event);

      if (event.timer.source == refresh_timer) {
         redraw = true;
         continue;
      }

      if (event.timer.source == playback_timer) {
         update_playback();
         continue;
      }

      if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
         on_mouse_axes(event.mouse.x, event.mouse.y);
         continue;
      }

      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
         on_mouse_down(event.mouse.x, event.mouse.y, event.mouse.button);
         continue;
      }

      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }

      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            break;

         if (event.keyboard.keycode == ALLEGRO_KEY_C)
            unselect_all_tokens();
      }
   }
}

int main(void)
{
   ALLEGRO_EVENT_QUEUE *queue;

   if (!al_init()) {
      abort_example("Error initialising Allegro.\n");
   }
   if (!al_install_audio() || !al_reserve_samples(128)) {
      abort_example("Error initialising audio.\n");
   }
   al_init_acodec_addon();
   al_init_image_addon();

   al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);

   display = al_create_display(screen_w, screen_h);
   if (!display) {
      abort_example("Error creating display.\n");
   }
   al_set_window_title(display, "Haiku - A Musical Instrument");

   load_images();
   load_samples();

   init_tokens();
   init_buttons();
   init_glow();
   select_button(&buttons[TYPE_EARTH]);

   al_install_keyboard();
   al_install_mouse();

   refresh_timer = al_create_timer(1.0 / refresh_rate);
   playback_timer = al_create_timer(playback_period / TOKENS_X);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_timer_event_source(refresh_timer));
   al_register_event_source(queue, al_get_timer_event_source(playback_timer));

   al_start_timer(refresh_timer);
   al_start_timer(playback_timer);

   main_loop(queue);

   free_all_flairs();

   return 0;
}

/* vim: set sts=3 sw=3 et: */
