#include "cosmic_protector.hpp"
#include "joypad_c.h"

#include <sys/stat.h>
#include <ctime>

#ifdef ALLEGRO_MSVC
   #define snprintf _snprintf
#endif

static int do_gui(const std::vector<Widget *>& widgets, unsigned int selected)
{
   ResourceManager& rm = ResourceManager::getInstance();
   ALLEGRO_BITMAP *bg = (ALLEGRO_BITMAP *)rm.getData(RES_BACKGROUND);
   Input *input = (Input *)rm.getData(RES_INPUT);
   ALLEGRO_BITMAP *logo = (ALLEGRO_BITMAP *)rm.getData(RES_LOGO);
   int lw = al_get_bitmap_width(logo);
   int lh = al_get_bitmap_height(logo);
#ifndef ALLEGRO_IPHONE
   ALLEGRO_FONT *myfont = (ALLEGRO_FONT *)rm.getData(RES_SMALLFONT);
#endif

   bool redraw = true;

   for (;;) {
      /* Catch close button presses */
      ALLEGRO_EVENT_QUEUE *events = ((DisplayResource *)rm.getResource(RES_DISPLAY))->getEventQueue();
      while (!al_is_event_queue_empty(events)) {
         ALLEGRO_EVENT event;
         al_get_next_event(events, &event);
#ifdef ALLEGRO_IPHONE
         if (event.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING || event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
            switch_game_out(event.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING);
         }
         else if (event.type == ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING || event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN) {
            switch_game_in();
         }
#else
         if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            exit(0);
#endif
      }

      input->poll();
#if defined ALLEGRO_IPHONE
      float ud;
      if (is_joypad_connected()) {
         ud = input->ud();
      }
      else {
         ud = input->lr();
      }
#else
      float ud = input->ud();
#endif
      if (ud < 0 && selected) {
         selected--;
         my_play_sample(RES_FIRELARGE);
         al_rest(0.200);
      }
      else if (ud > 0 && selected < (widgets.size()-1)) {
         selected++;
         my_play_sample(RES_FIRELARGE);
         al_rest(0.200);
      }
      if (input->b1()) {
         if (!widgets[selected]->activate())
            return selected;
      }
#ifndef ALLEGRO_IPHONE
      if (input->esc())
         return -1;
#endif

      redraw = true;

#ifdef ALLEGRO_IPHONE
      if (switched_out) {
         redraw = false;
      }
#endif

      al_rest(0.010);

      if (!redraw) {
         continue;
      }

      al_clear_to_color(al_map_rgb_f(0, 0, 0));

      /* draw */
      float h = al_get_bitmap_height(bg);
      float w = al_get_bitmap_width(bg);
      al_draw_bitmap(bg, (BB_W-w)/2, (BB_H-h)/2, 0);

      al_draw_bitmap(logo, (BB_W-lw)/2, (BB_H-lh)/4, 0);

      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
#ifndef ALLEGRO_IPHONE
      al_draw_textf(myfont, al_map_rgb(255, 255, 0), BB_W/2, BB_H/2, ALLEGRO_ALIGN_CENTRE, "z/y to start");
#endif
      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);

      for (unsigned int i = 0; i < widgets.size(); i++) {
         widgets[i]->render(i == selected);
      }

#ifdef ALLEGRO_IPHONE
        input->draw();
#endif

      al_flip_display();
   }
}

int do_menu(void)
{
   ButtonWidget play (BB_W/2, BB_H/4*3-16, true, "PLAY");
   ButtonWidget scores (BB_W/2, BB_H/4*3+16, true, "SCORES");
   ButtonWidget end (BB_W/2, BB_H/4*3+48, true, "EXIT");
   std::vector<Widget *> widgets;
   widgets.push_back(&play);
   widgets.push_back(&scores);
   widgets.push_back(&end);

   return do_gui(widgets, 0);
}

struct HighScore {
   char name[4];
   int score;
};

#define NUM_SCORES 5

static HighScore highScores[NUM_SCORES] = {
   { "AAA", 2000 },
   { "BAA", 1750 },
   { "AAC", 1500 },
   { "DAD", 1250 },
   { "AYE", 1000 }
};

static void insert_score(char *name, int score)
{
   int i;
   for (i = NUM_SCORES-1; i >= 0; i--) {
      if (highScores[i].score > score) {
         i++;
         break;
      }
   }
   if (i < 0) i = 0;
   if (i > (NUM_SCORES-1)) return; // yes, this is possible

   for (int j = NUM_SCORES-1; j > i; j--) {
      strcpy(highScores[j].name, highScores[j-1].name);
      highScores[j].score = highScores[j-1].score;
   }

   strcpy(highScores[i].name, name);
   highScores[i].score = score;
}

static ALLEGRO_PATH *userResourcePath()
{
#ifdef ALLEGRO_IPHONE
   return al_get_standard_path(ALLEGRO_USER_DOCUMENTS_PATH);
#else
   return al_get_standard_path(ALLEGRO_USER_SETTINGS_PATH);
#endif
}

static void read_scores(void)
{
   ALLEGRO_PATH *fn = userResourcePath();

   if (al_make_directory(al_path_cstr(fn, ALLEGRO_NATIVE_PATH_SEP))) {
      al_set_path_filename(fn, "scores.cfg");
      ALLEGRO_CONFIG *cfg = al_load_config_file(al_path_cstr(fn, ALLEGRO_NATIVE_PATH_SEP));
      if (cfg) {
         for (int i = 0; i < NUM_SCORES; i++) {
            char name[]  = {'n', (char)('0'+i), '\0'};
            char score[] = {'s', (char)('0'+i), '\0'};
            const char *v;

            v = al_get_config_value(cfg, "scores", name);
            if (v && strlen(v) <= 3) {
               strcpy(highScores[i].name, v);
            }
            v = al_get_config_value(cfg, "scores", score);
            if (v) {
               highScores[i].score = atoi(v);
            }
         }

         al_destroy_config(cfg);
      }
   }

   al_destroy_path(fn);
}

static void write_scores(void)
{
   ALLEGRO_CONFIG *cfg = al_create_config();
   for (int i = 0; i < NUM_SCORES; i++) {
      char name[]  = {'n', (char)('0'+i), '\0'};
      char score[] = {'s', (char)('0'+i), '\0'};
      char sc[32];

      al_set_config_value(cfg, "scores", name, highScores[i].name);
      snprintf(sc, sizeof(sc), "%d", highScores[i].score);
      al_set_config_value(cfg, "scores", score, sc);
   }

   ALLEGRO_PATH *fn = userResourcePath();
   al_set_path_filename(fn, "scores.cfg");
   al_save_config_file(al_path_cstr(fn, ALLEGRO_NATIVE_PATH_SEP), cfg);
   al_destroy_path(fn);

   al_destroy_config(cfg);
}

void do_highscores(int score)
{
   read_scores();

   ResourceManager& rm = ResourceManager::getInstance();
   Input *input = (Input *)rm.getData(RES_INPUT);
   ALLEGRO_FONT *sm_font = (ALLEGRO_FONT *)rm.getData(RES_SMALLFONT);
   ALLEGRO_FONT *big_font = (ALLEGRO_FONT *)rm.getData(RES_LARGEFONT);

   bool is_high = score >= highScores[NUM_SCORES-1].score;
   bool entering = is_high;
   double bail_time = al_get_time() + 8;
   int letter_num = 0;
   char name[4] = "   ";
   int character = 0;
   double next_input = al_get_time();
   double spin_start = 0;
   int spin_dir = 1;

   for (;;) {
      /* Catch close button presses */
      ALLEGRO_EVENT_QUEUE *events = ((DisplayResource *)rm.getResource(RES_DISPLAY))->getEventQueue();
      while (!al_is_event_queue_empty(events)) {
         ALLEGRO_EVENT event;
         al_get_next_event(events, &event);
#ifdef ALLEGRO_IPHONE
         if (event.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING || event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
            switch_game_out(event.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING);
         }
         else if (event.type == ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING || event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN) {
            switch_game_in();
         }
#else
         if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            exit(0);
#endif
      }

      input->poll();

      if (entering && al_get_time() > next_input) {
         float lr = input->lr();

         if (lr != 0) {
            if (lr < 0) {
               character--;
               if (character < 0)
                  character = 25;
               spin_dir = 1;
            }
            else if (lr > 0) {
               character++;
               if (character >= 26)
                  character = 0;
               spin_dir = -1;
            }
            next_input = al_get_time()+0.2;
            my_play_sample(RES_FIRESMALL);
            spin_start = al_get_time();
         }

         if (input->b1() && letter_num < 3) {
            name[letter_num] = 'A' + character;
            letter_num++;
            if (letter_num >= 3) {
               entering = false;
               bail_time = al_get_time()+8;
               insert_score(name, score);
               write_scores();
            }
            next_input = al_get_time()+0.2;
            my_play_sample(RES_FIRELARGE);
         }
      }
      else if (!entering) {
         if (al_get_time() > bail_time) {
            return;
         }
         else if (input->b1() && al_get_time() > next_input) {
            al_rest(0.250);
            return;
         }
      }

      al_rest(0.010);

#ifdef ALLEGRO_IPHONE
      if (switched_out) {
         continue;
      }
#endif

      al_clear_to_color(al_map_rgb(0, 0, 0));

      if (entering) {
         float a = ALLEGRO_PI*3/2;
         float ainc = ALLEGRO_PI*2 / 26;
         double elapsed = al_get_time() - spin_start;
         if (elapsed < 0.1) {
            a += (elapsed / 0.1) * ainc * spin_dir;
         }
         float scrh = BB_H / 2 - 32;
         float h = al_get_font_line_height(sm_font);
         for (int i = 0; i < 26; i++) {
            int c = character + i;
            if (c >= 26)
               c -= 26;
            char s[2];
            s[1] = 0;
            s[0] = 'A' +  c;
            int x = BB_W/2 + (cos(a) * scrh) - al_get_text_width(sm_font, s);
            int y = BB_H/2 + (sin(a) * scrh) - h/2;
            al_draw_textf(sm_font, i == 0 ? al_map_rgb(255, 255, 0) : al_map_rgb(200, 200, 200), x, y, 0, "%s", s);
            a += ainc;
         }
         char tmp[4] = { 0, };
         for (int i = 0; i < 3 && name[i] != ' '; i++) {
            tmp[i] = name[i];
         }
         al_draw_textf(big_font, al_map_rgb(0, 255, 0), BB_W/2, BB_H/2-20, ALLEGRO_ALIGN_CENTRE, "%s", tmp);
         al_draw_text(sm_font, al_map_rgb(200, 200, 200), BB_W/2, BB_H/2-20+5+al_get_font_line_height(big_font), ALLEGRO_ALIGN_CENTRE, "high score!");
      }
      else {
         int yy = BB_H/2 - al_get_font_line_height(big_font)*NUM_SCORES/2;
         for (int i = 0; i < NUM_SCORES; i++) {
            al_draw_textf(big_font, al_map_rgb(255, 255, 255), BB_W/2-10, yy, ALLEGRO_ALIGN_RIGHT, "%s", highScores[i].name);
            al_draw_textf(big_font, al_map_rgb(255, 255, 0), BB_W/2+10, yy, ALLEGRO_ALIGN_LEFT, "%d", highScores[i].score);
            yy += al_get_font_line_height(big_font);
         }
      }

#ifdef ALLEGRO_IPHONE
      input->draw();
#endif

      al_flip_display();
   }
}
