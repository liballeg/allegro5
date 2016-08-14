#pragma once

#include <QWidget>
#include <allegro5/allegro.h>
#include <d3d9.h>

class QAllegroCanvas
   :public QWidget
{
   Q_OBJECT

public:
   QAllegroCanvas(QWidget* parent = 0);
   ~QAllegroCanvas();

   void setBackColor(ALLEGRO_COLOR color);
   bool isDeviceLost();
   ALLEGRO_DISPLAY* getDisplay();

signals:
   void initiated();

private:
   virtual void timerEvent(QTimerEvent *) override;
   virtual void resizeEvent(QResizeEvent *) override;
   virtual void showEvent(QShowEvent *) override;
   virtual void hideEvent(QHideEvent *) override;

   virtual void init(){}
   virtual void destroy(){}
   virtual void update(float delta){}
   virtual void render(){}
   virtual void deviceLost(){}
   virtual void deviceReset(){}
   virtual void resize(){}
   void doLoop();


private:
   int m_loopTimer{ 0 };
   ALLEGRO_DISPLAY* m_display{ nullptr };
   ALLEGRO_COLOR m_backcolor;
   float m_lasttime{ 0 };
   LPDIRECT3DDEVICE9 m_device{ nullptr };
   bool m_devicelost{ false };
   bool m_init{ false };
};
