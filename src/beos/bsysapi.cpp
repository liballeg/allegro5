/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Stuff for BeOS.
 *
 *      By Jason Wilkins.
 *
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/aintern.h"
#include "allegro/aintbeos.h"

#ifndef ALLEGRO_BEOS
#error something is wrong with the makefile
#endif              

#define SYS_THREAD_PRIORITY B_NORMAL_PRIORITY
#define SYS_THREAD_NAME     "system driver"

#define EXE_NAME_UNKNOWN "./UNKNOWN"



status_t ignore_result = 0;

volatile int32 focus_count = 0;

BeAllegroApp *be_allegro_app = NULL;

static thread_id system_thread_id = -1;
static thread_id main_thread_id = -1;
static sem_id system_started = -1;


static bool      using_custom_allegro_app;


/* BeAllegroApp::BeAllegroApp:
 */
BeAllegroApp::BeAllegroApp(const char *signature)
   : BApplication(signature)
{
}
                 


/* BeAllegroApp::QuitRequested:
 */
bool BeAllegroApp::QuitRequested(void)
{
   return false;
}



/* BeAllegroApp::ReadyToRun:
 */
void BeAllegroApp::ReadyToRun()
{
   release_sem(system_started);
}    



/* _be_sys_get_executable_name:
 */
extern "C" void _be_sys_get_executable_name(char *output, int size)
{
   image_info info;
   int32 cookie;

   cookie = 0;

   if (get_next_image_info(0, &cookie, &info) == B_NO_ERROR) {
      strncpy(output, info.name, size);
   }
   else {
      strncpy(output, EXE_NAME_UNKNOWN, size);
   }

   output[size-1] = '\0';
}



/* system_thread:
 */
static int32 system_thread(void *data)
{
   (void)data;

   if (be_allegro_app == NULL) {
      char sig[MAXPATHLEN] = "application/x-vnd.Allegro-";
      char exe[MAXPATHLEN];

      _be_sys_get_executable_name(exe, sizeof(exe));

      strncat(sig, get_filename(exe), sizeof(sig));
      sig[sizeof(sig)-1] = '\0';

      be_allegro_app = new BeAllegroApp(sig);

      using_custom_allegro_app = false;
   }
   else {
      using_custom_allegro_app = true;
   }

   be_allegro_app->Run();

   AL_TRACE("system thread exited\n");

   return 0;
}



/* be_sys_init:
 */
extern "C" int be_sys_init(void)
{
   int32       cookie;
   thread_info info;

   cookie = 0;

   if (get_next_thread_info(0, &cookie, &info) == B_OK) {
      main_thread_id = info.thread;
   }
   else {
      goto cleanup;
   }

   be_mouse_view_attached = create_sem(0, "waiting for mouse view attach...");

   if (be_mouse_view_attached < 0) {
      goto cleanup;
   }  

   system_started = create_sem(0, "starting system driver...");

   if(system_started < 0) {
      goto cleanup;
   }

   system_thread_id = spawn_thread(system_thread, SYS_THREAD_NAME,
                         SYS_THREAD_PRIORITY, NULL);

   if (system_thread_id < 0) {
      goto cleanup;
   }

   resume_thread(system_thread_id);
   acquire_sem(system_started);
   delete_sem(system_started);

   return 0;

   cleanup: {
      be_sys_exit();
      return 1;
   }
}



/* be_sys_exit:
 */
extern "C" void be_sys_exit(void)
{
   if (main_thread_id > 0) {
      main_thread_id = -1;
   }

   if (system_started > 0) {
      delete_sem(system_started);
      system_started = -1;
   }

   if (system_thread_id > 0) {
      ASSERT(be_allegro_app != NULL);
      be_allegro_app->Lock();
      be_allegro_app->Quit();
      be_allegro_app->Unlock();

      wait_for_thread(system_thread_id, &ignore_result);
      system_thread_id = -1;
   }

   if (be_mouse_view_attached > 0) {
      delete_sem(be_mouse_view_attached);
      be_mouse_view_attached = -1;
   }

   if (!using_custom_allegro_app) {
      delete be_allegro_app;
      be_allegro_app = NULL;
   }
}



/* be_sys_get_executable_name:
 */
extern "C" void be_sys_get_executable_name(char *output, int size)
{
   char *buffer;

   buffer = (char *)malloc(size);

   if (buffer != NULL) {
      _be_sys_get_executable_name(buffer, size);
      do_uconvert(buffer, U_UTF8, output, U_CURRENT, size);
      free(buffer);
   }
   else {
      do_uconvert(EXE_NAME_UNKNOWN, U_ASCII, output, U_CURRENT, size);
   }
}



extern "C" void be_sys_set_window_title(char *name)
{
   char uname[256];

   do_uconvert(name, U_CURRENT, uname, U_UTF8, sizeof(uname));
   if (be_allegro_window != NULL) {
      be_allegro_window->SetTitle(uname);
   }
   else {
      be_allegro_screen->SetTitle(uname);
   }
}



extern "C" void be_sys_message(char *msg)
{
   char  filename[MAXPATHLEN];
   char *title;

   get_executable_name(filename, sizeof(filename));
   title = get_filename(filename);

   fprintf(stderr, "%s: %s", title, msg);

   BAlert *alert = new BAlert(title, msg, "Okay");
   alert->SetShortcut(0, B_ESCAPE);
   alert->Go();
}



extern "C" void be_sys_suspend(void)
{
   if (system_thread_id > 0) {
      suspend_thread(system_thread_id);
   }
}



extern "C" void be_sys_resume(void)
{
   if (system_thread_id > 0) {
      resume_thread(system_thread_id);
   }
}
                       

extern "C" void be_main_suspend(void)
{
   if (main_thread_id > 0) {
      suspend_thread(main_thread_id);
   }
}



extern "C" void be_main_resume(void)
{
   if (main_thread_id > 0) {
      resume_thread(main_thread_id);
   }
}
                              


int32 killer_thread(void *data)
{
   int32     cookie;
   team_info info;

   kill_thread(main_thread_id);
   allegro_exit();
   cookie = 0;
   get_next_team_info(&cookie, &info);
   kill_team(info.team);

   return 1;
}



void be_terminate(thread_id caller)
{
   thread_id killer;

   killer = spawn_thread(killer_thread, "son of sam", 120, NULL);
   resume_thread(killer);

   exit_thread(1);
}
