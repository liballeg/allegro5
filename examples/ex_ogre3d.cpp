/*
 *    Example program for the Allegro library, by Peter Wang.
 *    Windows support by AMCerasoli.
 *
 *    This is a test program to see if Allegro can be used alongside the OGRE
 *    graphics library. It currently only works on Linux/GLX and Windows.
 *    To run, you will need to have OGRE plugins.cfg and resources.cfg files in
 *    the current directory.
 *
 *    Inputs: W A S D, mouse
 *
 *    This code is based on the samples in the OGRE distribution.
 */

#include <Ogre.h>
#include <allegro5/allegro.h>
#ifdef ALLEGRO_WINDOWS
#include <allegro5/allegro_windows.h>
#endif
#include <allegro5/allegro_opengl.h>

/*
 * Ogre 1.7 (and, optionally, earlier versions) uses the FreeImage library to
 * handle image loading.  FreeImage bundles its own copies of common libraries
 * like libjpeg and libpng, which can conflict with the system copies of those
 * libraries that allegro_image uses.  That means we can't use allegro_image
 * safely, nor any of the addons which depend on it.
 *
 * One solution would be to write a codec for Ogre that avoids FreeImage, or
 * write allegro_image handlers using FreeImage.  The latter would probably be
 * easier and useful for other reasons.
 */

using namespace Ogre;

const int WIDTH = 640;
const int HEIGHT = 480;
const float MOVE_SPEED = 1500.0;

class Application
{
protected:
   Root *mRoot;
   RenderWindow *mWindow;
   SceneManager *mSceneMgr;
   Camera *mCamera;

public:
   void setup(ALLEGRO_DISPLAY *display)
   {
      createRoot();
      defineResources();
      setupRenderSystem();
      createRenderWindow(display);
      initializeResourceGroups();
      chooseSceneManager();
      createCamera();
      createViewports();
      createScene();
   }
   
   Application()
   {
      mRoot = NULL;
   }

   ~Application()
   {
      delete mRoot;
   }

private:
   void createRoot()
   {
      mRoot = new Root();
   }

   void defineResources()
   {
      String secName, typeName, archName;
      ConfigFile cf;
      cf.load("resources.cfg");
      ConfigFile::SectionIterator seci = cf.getSectionIterator();
      while (seci.hasMoreElements()) {
         secName = seci.peekNextKey();
         ConfigFile::SettingsMultiMap *settings = seci.getNext();
         ConfigFile::SettingsMultiMap::iterator i;
         for (i = settings->begin(); i != settings->end(); ++i) {
            typeName = i->first;
            archName = i->second;
            ResourceGroupManager::getSingleton().addResourceLocation(archName,
               typeName, secName);
         }
      }
   }

   void setupRenderSystem()
   {
      if (!mRoot->restoreConfig() && !mRoot->showConfigDialog()) {
         throw Exception(52, "User canceled the config dialog!",
            "Application::setupRenderSystem()");
      }
   }

   void createRenderWindow(ALLEGRO_DISPLAY *display)
   {
      int w = al_get_display_width(display);
      int h = al_get_display_height(display);

      // Initialise Ogre without creating a window.
      mRoot->initialise(false);

      Ogre::NameValuePairList misc;
      #ifdef ALLEGRO_WINDOWS
      unsigned long winHandle      = reinterpret_cast<size_t>(al_get_win_window_handle(display));
      unsigned long winGlContext   = reinterpret_cast<size_t>(wglGetCurrentContext());
      misc["externalWindowHandle"] = StringConverter::toString(winHandle);
      misc["externalGLContext"]    = StringConverter::toString(winGlContext);
      misc["externalGLControl"]    = String("True");
      #else
      misc["currentGLContext"] = String("True");
      #endif

      mWindow = mRoot->createRenderWindow("MainRenderWindow", w, h, false,
         &misc);
   }

   void initializeResourceGroups()
   {
      TextureManager::getSingleton().setDefaultNumMipmaps(5);
      ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
   }

   virtual void chooseSceneManager()
   {
      mSceneMgr = mRoot->createSceneManager(ST_GENERIC, "Default SceneManager");
   }

   virtual void createCamera()
   {
      mCamera = mSceneMgr->createCamera("PlayerCam");
      mCamera->setPosition(Vector3(-300, 300, -300));
      mCamera->lookAt(Vector3(0, 0, 0));
      mCamera->setNearClipDistance(5);
   }

   virtual void createViewports()
   {
      // Create one viewport, entire window.
      Viewport *vp = mWindow->addViewport(mCamera);
      vp->setBackgroundColour(ColourValue(0, 0.25, 0.5));

      // Alter the camera aspect ratio to match the viewport.
      mCamera->setAspectRatio(
         Real(vp->getActualWidth()) / Real(vp->getActualHeight()));
   }

   virtual void createScene() = 0;

public:

   void render()
   {
      const bool swap_buffers = false;
      mWindow->update(swap_buffers);
      mRoot->renderOneFrame();
      al_flip_display();
   }
};

class Example : public Application
{
private:
   ALLEGRO_DISPLAY *display;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_TIMER *timer;

   double startTime;
   double lastRenderTime;
   double lastMoveTime;
   bool lmb;
   bool rmb;
   bool forward;
   bool back;
   bool left;
   bool right;
   float current_speed;
   Vector3 last_translate;

public:
   Example(ALLEGRO_DISPLAY *display);
   ~Example();
   void setup();
   void mainLoop();

private:
   void createScene();
   void moveCamera(double timestamp, Radian rot_x, Radian rot_y,
         Vector3 & translate);
   void animate(double now);
   void nextFrame();
};

Example::Example(ALLEGRO_DISPLAY *display) :
   display(display),
   queue(NULL),
   timer(NULL),
   startTime(0.0),
   lastRenderTime(0.0),
   lastMoveTime(0.0),
   lmb(false),
   rmb(false),
   forward(false),
   back(false),
   left(false),
   right(false),
   current_speed(0),
   last_translate(Vector3::ZERO)
{
}

Example::~Example()
{
   if (timer) al_destroy_timer(timer);
   if (queue) al_destroy_event_queue(queue);
}

void Example::createScene()
{
   // Enable shadows.
   mSceneMgr->setAmbientLight(ColourValue(0.5, 0.25, 0.0));
   //mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_ADDITIVE);  // slower
   mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_MODULATIVE);  // faster

   // Create the character.
   Entity *ent1 = mSceneMgr->createEntity("Ninja", "ninja.mesh");
   ent1->setCastShadows(true);
   mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(ent1);

   AnimationState *anim1 = ent1->getAnimationState("Walk");
   anim1->setLoop(true);
   anim1->setEnabled(true);

   // Create the ground.
   Plane plane(Vector3::UNIT_Y, 0);
   MeshManager::getSingleton().createPlane("ground",
      ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, plane,
      1500, 1500, 20, 20, true, 1, 5, 5, Vector3::UNIT_Z);

   Entity *ent2 = mSceneMgr->createEntity("GroundEntity", "ground");
   ent2->setMaterialName("Examples/Rockwall");
   ent2->setCastShadows(false);
   mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(ent2);

   // Create a light.
   Light *light = mSceneMgr->createLight("Light1");
   light->setType(Light::LT_POINT);
   light->setPosition(Vector3(0, 150, 250));
   light->setDiffuseColour(1.0, 1.0, 1.0);
   light->setSpecularColour(1.0, 0.0, 0.0);
}

void Example::setup()
{
   Application::setup(display);

   const double BPS = 60.0;
   timer = al_create_timer(1.0 / BPS);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_register_event_source(queue, al_get_display_event_source(display));
}

void Example::mainLoop()
{
   bool redraw = true;

   startTime = lastMoveTime = al_get_time();
   al_start_timer(timer);

   for (;;) {
      ALLEGRO_EVENT event;

      if (al_is_event_queue_empty(queue) && redraw) {
         nextFrame();
         redraw = false;
      }

      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
            event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
         break;
      }
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }

      Radian rot_x(0);
      Radian rot_y(0);
      Vector3 translate(Vector3::ZERO);

      switch (event.type) {
         case ALLEGRO_EVENT_TIMER:
            redraw = true;
            break;

         case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            if (event.mouse.button == 1)
               lmb = true;
            if (event.mouse.button == 2)
               rmb = true;
            break;

         case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            if (event.mouse.button == 1)
               lmb = false;
            if (event.mouse.button == 2)
               rmb = false;
            if (!lmb && !rmb)
               al_show_mouse_cursor(display);
            break;

         case ALLEGRO_EVENT_MOUSE_AXES:
            if (lmb) {
               rot_x = Degree(-event.mouse.dx * 0.13);
               rot_y = Degree(-event.mouse.dy * 0.13);
            }
            if (rmb) {
               translate.x = event.mouse.dx * 0.5;
               translate.y = event.mouse.dy * -0.5;
            }
            if (lmb || rmb) {
               al_hide_mouse_cursor(display);
               al_set_mouse_xy(display,
                  al_get_display_width(display)/2,
                  al_get_display_height(display)/2);
            }
            break;

         case ALLEGRO_EVENT_KEY_DOWN:
         case ALLEGRO_EVENT_KEY_UP: {
            const bool is_down = (event.type == ALLEGRO_EVENT_KEY_DOWN);
            if (event.keyboard.keycode == ALLEGRO_KEY_W)
               forward = is_down;
            if (event.keyboard.keycode == ALLEGRO_KEY_S)
               back = is_down;
            if (event.keyboard.keycode == ALLEGRO_KEY_A)
               left = is_down;
            if (event.keyboard.keycode == ALLEGRO_KEY_D)
               right = is_down;
            break;
         }

         case ALLEGRO_EVENT_DISPLAY_RESIZE: {
            al_acknowledge_resize(event.display.source);
            int w = al_get_display_width(display);
            int h = al_get_display_height(display);
            mWindow->resize(w, h);
            mCamera->setAspectRatio(Real(w) / Real(h));
            redraw = true;
            break;
         }
      }

      moveCamera(event.any.timestamp, rot_x, rot_y, translate);
   }
}

void Example::moveCamera(double timestamp, Radian rot_x, Radian rot_y,
   Vector3 & translate)
{
   const double time_since_move = timestamp - lastMoveTime;
   const float move_scale = MOVE_SPEED * time_since_move;

   if (forward) {
      translate.z = -move_scale;
   }
   if (back) {
      translate.z = move_scale;
   }
   if (left) {
      translate.x = -move_scale;
   }
   if (right) {
      translate.x = move_scale;
   }

   if (translate == Vector3::ZERO) {
      // Continue previous motion but dampen.
      translate = last_translate;
      current_speed -= time_since_move * 0.3;
   }
   else {
      // Ramp up.
      current_speed += time_since_move;
   }
   if (current_speed > 1.0)
      current_speed = 1.0;
   if (current_speed < 0.0)
      current_speed = 0.0;

   translate *= current_speed;

   mCamera->yaw(rot_x);
   mCamera->pitch(rot_y);
   mCamera->moveRelative(translate);

   last_translate = translate;
   lastMoveTime = timestamp;
}

void Example::animate(double now)
{
   const double dt0 = now - startTime;
   const double dt = now - lastRenderTime;

   // Animate the character.
   Entity *ent = mSceneMgr->getEntity("Ninja");
   AnimationState *anim = ent->getAnimationState("Walk");
   anim->addTime(dt);

   // Move the light around.
   Light *light = mSceneMgr->getLight("Light1");
   light->setPosition(Vector3(300 * cos(dt0), 300, 300 * sin(dt0)));
}

void Example::nextFrame()
{
   const double now = al_get_time();
   animate(now);
   render();
   lastRenderTime = now;
}

int main(int argc, char *argv[])
{
   (void)argc;
   (void)argv;
   ALLEGRO_DISPLAY *display;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_install_keyboard();
   al_install_mouse();

   al_set_new_display_flags(ALLEGRO_OPENGL | ALLEGRO_RESIZABLE);
   display = al_create_display(WIDTH, HEIGHT);
   if (!display) {
      abort_example("Error creating display\n");
   }
   al_set_window_title(display, "My window");

   {
      Example app(display);
      app.setup();
      app.mainLoop();
   }

   al_uninstall_system();

   return 0;
}

/* vim: set sts=3 sw=3 et: */
