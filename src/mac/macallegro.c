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
 *      Some others mac stuff.
 *
 *      By Ronaldo Hideki Yamada.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/aintern.h"
#include "macalleg.h"
#include "allegro/aintmac.h"
#include <string.h>

/*The our QuickDraw globals */
QDGlobals qd;



/*
 * an strdup function to mac
 */
char *strdup(const char *p){
   char *tmp=malloc(strlen(p));
   if(tmp){
      strcpy(tmp,p);
   }
   return tmp;
}



/*
 * convert pascal strings to c strings
 */
void ptoc(StringPtr pstr,char *cstr)
{
   BlockMove(pstr+1,cstr,pstr[0]);
   cstr[pstr[0]]='\0';
}



/*
 *  Simple function that returns TRUE if we're running on a Mac thatÉ
 *  is running Color Quickdraw.
 */
static Boolean DoWeHaveColor (void)
{
   SysEnvRec      thisWorld;
   
   SysEnvirons(2, &thisWorld);      // Call the old SysEnvirons() function.
   return (thisWorld.hasColorQD);   // And return whether it has Color QuickDraw.
}



/*  Another simple "environment" function.Returns TRUE if the Mac we're runningÉ
 *  on has System 6.0.5 or more recent.(6.0.5 introduced the ability to switchÉ
 *  color depths.)
 */
static Boolean DoWeHaveSystem605 (void)
{
   SysEnvRec      thisWorld;
   Boolean         haveIt;
   
   SysEnvirons(2, &thisWorld);      // Call the old SysEnvirons() function.
   if (thisWorld.systemVersion >= 0x0605)
      haveIt = TRUE;            // Check the System version for 6.0.5É
   else                     // or more recent
      haveIt = FALSE;
   return (haveIt);
}



/*
 *
 */
Boolean RTrapAvailable(short tNumber,TrapType tType)
{
   return NGetTrapAddress( tNumber, tType ) != NGetTrapAddress( _Unimplemented, ToolTrap);      
}



/*
 * Executs an cleanup before return to system called from atexit()
 */
static void mac_cleanup(){
   _tm_sys_exit();
   _dspr_sys_exit();
   FlushEvents(nullEvent|mouseDown|mouseUp|keyDown|keyUp|autoKey|updateEvt,0); 
}
 
/*
 *
 *The entry point for MacAllegro programs setup mac toolbox application memory 
 *This routine should be called before main() this is do by -main MacEntry option in final Link
 *if not called then the program can crash!!!
 */
void MacEntry(){
   char argbuf[512];
   char *argv[64];
   int argc;
   int i, q;
   SysEnvRec envRec;long   heapSize;
   OSErr e;

   InitGraf((Ptr) &qd.thePort);      /*init the normal mac toolbox*/
   InitFonts();
   InitWindows();
   InitMenus();
   TEInit();
   InitDialogs(nil);
   InitCursor();
   InitPalettes();

/*memory setup*/
   SysEnvirons( curSysEnvVers, &envRec );
   if (envRec.machineType < 0 )
      ExitToShell();
   if (kStackNeeded > StackSpace()){
      SetApplLimit((Ptr) ((long) GetApplLimit() - kStackNeeded + StackSpace()));
   }
   heapSize = (long) GetApplLimit() - (long) ApplicationZone();
   if ( heapSize < kHeapNeeded){
      _mac_message("no enough memory");
      ExitToShell();
   }
   MaxApplZone();
   if(!(RTrapAvailable(_WaitNextEvent, ToolTrap)))ExitToShell();

   GetDateTime((unsigned long*) &qd.randSeed);

   if(_dspr_sys_init()){
      _mac_message("no could start drawsprocket");
	  return;
   }
   
   _mac_file_sys_init();
   
   _mac_init_system_bitmap();           /*init our system bitmap vtable*/

   atexit(&mac_cleanup);

   _mac_get_executable_name(argbuf,511);/*get command line*/

   argc = 0;
   i = 0;

   /* parse commandline into argc/argv format */
   while (argbuf[i]) {
      while ((argbuf[i]) && (uisspace(argbuf[i])))i++;
      if (argbuf[i]) {
         if ((argbuf[i] == '\'') || (argbuf[i] == '"')){
            q = argbuf[i++];
            if (!argbuf[i])break;
         }
         else q = 0;
         argv[argc++] = &argbuf[i];
         while ((argbuf[i]) && ((q) ? (argbuf[i] != q) : (!uisspace(argbuf[i]))))i++;
         if (argbuf[i])
		    {
            argbuf[i] = 0;
            i++;
         }
      }
   }
   
#if (TRACE_MAC_SYSTEM)
   fprintf(stdout,"MacEntry Done\n");
   fflush(stdout);
#endif
   main(argc, argv);
   FlushEvents(everyEvent,0);
}
