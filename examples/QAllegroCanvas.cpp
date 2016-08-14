#include "QAllegroCanvas.h"
#include <QTimerEvent>
#include <allegro5/allegro_direct3d.h>

QAllegroCanvas::QAllegroCanvas(QWidget* parent /*= 0*/)
   :QWidget(parent)
{
   al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);
   al_set_new_display_option(ALLEGRO_USE_EXISTING_WINDOW, this->winId(), ALLEGRO_SUGGEST);

   m_display = al_create_display(width(), height());
   m_device = al_get_d3d_device(m_display);

   setBackColor(al_map_rgb(255, 255, 255));
}

QAllegroCanvas::~QAllegroCanvas()
{
	destroy();

   if (m_display)
   {
      al_destroy_display(m_display);
      m_device = nullptr;
      m_display = nullptr;
   }
}

void QAllegroCanvas::setBackColor(ALLEGRO_COLOR color)
{
   m_backcolor = color;
}

bool QAllegroCanvas::isDeviceLost()
{
   return m_devicelost;
}

ALLEGRO_DISPLAY* QAllegroCanvas::getDisplay()
{
   return m_display;
}

void QAllegroCanvas::timerEvent(QTimerEvent *event)
{
   if (event->timerId() == m_loopTimer)
      doLoop();
}

void QAllegroCanvas::resizeEvent(QResizeEvent *)
{
   if (m_display)
      al_acknowledge_resize(m_display);
   resize();
}

void QAllegroCanvas::showEvent(QShowEvent *)
{
   m_lasttime = al_get_time();
   m_loopTimer = startTimer(1);
   if (!m_init)
   {
	   m_init = true;
	   init();
	   emit initiated();
   }
}

void QAllegroCanvas::hideEvent(QHideEvent *)
{
   killTimer(m_loopTimer);
}

void QAllegroCanvas::doLoop()
{
   if (!m_display || !m_device)
      return;

   float currenttime = al_get_time();
   float delta = currenttime - m_lasttime;

   update(delta);

   HRESULT hr = m_device->TestCooperativeLevel();
   if (hr != D3D_OK)
   {
      if (hr == D3DERR_DEVICELOST)
      {
         m_devicelost = true;
         deviceLost();
      }
      else if (hr == D3DERR_DEVICENOTRESET)
      {
         al_acknowledge_resize(m_display);
         m_devicelost = false;
         deviceReset();
      }
   }
   else
   {
      al_set_target_backbuffer(m_display);
      al_clear_to_color(m_backcolor);
      render();
      al_flip_display();
   }

   m_lasttime = currenttime;
}
