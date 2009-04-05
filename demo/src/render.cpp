#include "a5teroids.hpp"

static float waveAngle = 0.0f;
static ALLEGRO_BITMAP *waveBitmap = 0;
static float bgx = 0;
static float bgy = 0;
static int shakeUpdateCount = 0;
static int shakeCount = 0;
const int SHAKE_TIME = 100;
const int SHAKE_TIMES = 10;

static void renderWave(void)
{
   int w = al_get_bitmap_width(waveBitmap);
   int h = al_get_bitmap_height(waveBitmap);

   float a = -(waveAngle + ALLEGRO_PI/2);

   int x = (int)(BB_W/2 + 64*cos(a));
   int y = (int)(BB_H/2 + 64*sin(a));

   al_draw_rotated_bitmap(waveBitmap, w/2, h,
      x, y, waveAngle, 0);
}

static void stopWave(void)
{
   waveAngle = 0.0f;
   al_destroy_bitmap(waveBitmap);
   waveBitmap = 0;
}

void showWave(int num)
{
   if (waveBitmap)
      stopWave();

   ResourceManager& rm = ResourceManager::getInstance();

   ALLEGRO_FONT *myfont = (ALLEGRO_FONT *)rm.getData(RES_LARGEFONT);
   
   char text[20];
   sprintf(text, "WAVE %d", num);

   int w = al_get_text_width(myfont, text, 0, 0);
   int h = al_get_font_line_height(myfont);

   waveBitmap = al_create_bitmap(w, h);
   ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
   al_set_target_bitmap(waveBitmap);
   al_clear(al_map_rgba(0, 0, 0, 0));
   al_draw_textf(myfont, 0, 0, 0, "%s", text);
   al_set_target_bitmap(old_target);

   waveAngle = (ALLEGRO_PI*2);
}

void shake(void)
{
   shakeUpdateCount = SHAKE_TIME;
   bgx = randf(0.0f, 8.0f);
   bgy = randf(0.0f, 8.0f);
   if (rand() % 2) bgx = -bgx;
   if (rand() % 2) bgy = -bgy;
}

void render(int step)
{
   ResourceManager& rm = ResourceManager::getInstance();

   ALLEGRO_BITMAP *bg = (ALLEGRO_BITMAP *)rm.getData(RES_BACKGROUND);

   if (shakeUpdateCount > 0) {
      shakeUpdateCount -= step;
      if (shakeUpdateCount <= 0) {
         shakeCount++;
         if (shakeCount >= SHAKE_TIMES) {
            shakeCount = 0;
            shakeUpdateCount = 0;
            bgx = bgy = 0;
         }
         else {
            bgx = randf(0.0f, 8.0f);
            bgy = randf(0.0f, 8.0f);
            if (rand() % 2) bgx = -bgx;
            if (rand() % 2) bgy = -bgy;
            shakeUpdateCount = SHAKE_TIME;
         }
      }
   }

   al_draw_scaled_bitmap(bg, 0, 0, 
      al_get_bitmap_width(bg), al_get_bitmap_height(bg), 
      bgx, bgy, BB_W, BB_H, 0);

   int rendered = 0;

   std::list<Entity *>::iterator it;
   for (it = entities.begin(); it != entities.end(); it++) {
      Entity *e = *it;
      e->render_four();
      if (e->isHighlighted()) {
         al_set_blender(ALLEGRO_ALPHA, ALLEGRO_ONE, al_map_rgb(150, 150, 150));
         e->render_four();
         al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
            al_map_rgb(255, 255, 255));
      }
      rendered++;
   }

   Player *player = (Player *)rm.getData(RES_PLAYER);
   player->render_four();
   player->render_extra();

   if (waveAngle > 0.0f) {
      renderWave();
      waveAngle -= 0.003f * step;
      if (waveAngle <= 0.0f) {
         stopWave();
      }
   }

   al_flip_display();
}

