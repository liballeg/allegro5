/* This example demonstrate embed Allegro in Qt.
*/

#include <QtWidgets/QApplication>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_color.h>
#include <QAllegroCanvas.h>

class DerivedCanvas
   :public QAllegroCanvas
{
public:
   //Init your resources.
   virtual void init() override
   {
      if (!bitmap)
         bitmap = al_load_bitmap("mysha.pcx");
      if (!font)
         font = al_load_font("fixed_font.tga", 0, 0);
   }

   //Destroy your resources.
   virtual void destroy() override
   {
      if (bitmap)
         al_destroy_bitmap(bitmap);
      if (font)
         al_destroy_font(font);
   }

   //You need derive render() to show your objects.
   virtual void render() override
   {
      al_draw_bitmap(bitmap, 0, 0, 0);

      al_draw_line(0, 0, width(), height(), al_map_rgb(255, 0, 0), 0);
      al_draw_circle(width() / 2, height() / 2, 100, al_map_rgb(0, 0, 255), 3);
      al_draw_filled_triangle(100, 100, width() - 200, height() / 2, 30, height() - 100, al_map_rgb(128, 0, 256));

      al_draw_text(font, al_map_rgba_f(1, 1, 1, 1.0), 0, 0, 0, "Embed allegro in qt!");
   }

   //Derive this method to update something.
   virtual void update(float delta) override
   {
   }

private:
   ALLEGRO_BITMAP *bitmap{ nullptr };
   ALLEGRO_FONT *font{ nullptr };
};

int main(int argc, char *argv[])
{
   if (!al_init()) {
      return 0;
   }
   al_init_image_addon();
   al_init_primitives_addon();
   al_init_font_addon();

   QApplication a(argc, argv);
   DerivedCanvas* canvas = new DerivedCanvas();
   canvas->show();
   int ret = a.exec();
   delete canvas;
   return ret;
}
