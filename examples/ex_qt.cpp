/* This example demonstrate embed allegro in Qt.
*/

#include "alqtex.h"
#include <QtWidgets/QApplication>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_color.h>
#include <QAllegroCanvas.h>

class DerivedCanvas
   :public QAllegroCanvas
{
public:
   //You need derive render() to show your objects.
   virtual void render() override
   {
      al_draw_line(0, 0, width(), height(), al_map_rgb(255, 0, 0), 0);
      al_draw_circle(width() / 2, height() / 2, 100, al_map_rgb(0, 0, 255), 3);
      al_draw_filled_triangle(100, 100, width() - 200, height() / 2, 30, height() - 100, al_map_rgb(128, 0, 256));
   }

   //Derive this method to update something.
   virtual void update(float delta) override
   {
   }

};

int main(int argc, char *argv[])
{
   if (!al_init()) {
      return 0;
   }
   al_init_primitives_addon();

   QApplication a(argc, argv);
   DerivedCanvas* canvas = new DerivedCanvas();
   canvas->show();
   int ret = a.exec();
   delete canvas;
   return ret;
}
