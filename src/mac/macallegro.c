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
 *      Mac drivers.
 *
 *      By Ronaldo H Yamada.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/aintern.h"
#include "macalleg.h"
#include "allegro/aintmac.h"
#include <string.h>



#pragma mark Debug control

const RGBColor ForeDef={0,0,0};
const RGBColor BackDef={0xFFFF,0xFFFF,0xFFFF};

/*
Used for Debug proposites. log the calls to stdout
0 Disable
1 Enable
*/

#define TRACE_MAC_KBRD 0
#define TRACE_MAC_MOUSE 0
#define TRACE_MAC_SYSTEM 0
/*The our QuickDraw globals */
QDGlobals qd;


char volname[32];
/*main volume number*/
short MainVol;

/*main directory ID*/
long MainDir;

/**/
short _mackeyboardinstalled=FALSE;
/*Used for our keyboard driver*/
volatile KeyMap KeyNow;
volatile KeyMap KeyOld;
/**/
short _mactimerinstalled=FALSE;
/**/
short _macmouseinstalled=FALSE;

/*Our TimerManager task entry queue*/
TMTask   theTMTask;      
/*Our TM task is running*/
short _timer_running=0;

/*Our window title ???*/
char macwindowtitle[256];

/*char buffer for Trace*/
char macsecuremsg[256];

static pascal void MyTask(TMTaskPtr tmTaskPtr);
static int mac_timer_init(void);
static void mac_timer_exit(void);
static int mouse_mac_init(void);
static void mouse_mac_exit(void);
static int key_mac_init(void);
static void key_mac_exit(void);


#pragma mark Key tables
/*
The LookUp table to translate AppleKeyboard scan codes to Allegro KEY constants
*/

const unsigned char key_apple_to_allegro[128]=
{
/*00*/   KEY_X,      KEY_Z,      KEY_G,      KEY_H,
/*04*/   KEY_F,      KEY_D,      KEY_S,      KEY_A,   
/*08*/   KEY_R,      KEY_E,      KEY_W,      KEY_Q,
/*0C*/   KEY_B,      KEY_TILDE,  KEY_V,      KEY_C,
/*10*/   KEY_5,      KEY_6,      KEY_4,      KEY_3,
/*14*/   KEY_2,      KEY_1,      KEY_T,      KEY_Y,
/*18*/   KEY_O,      KEY_CLOSEBRACE,KEY_0,   KEY_8,
/*1C*/   KEY_MINUS,  KEY_7,      KEY_9,      KEY_EQUALS,
/*20*/   KEY_QUOTE,  KEY_J,      KEY_L,      KEY_ENTER,
/*24*/   KEY_P,      KEY_I,      KEY_OPENBRACE,KEY_U,
/*28*/   KEY_STOP,   KEY_M,      KEY_N,      KEY_SLASH,
/*2C*/   KEY_COMMA,  KEY_BACKSLASH,KEY_COLON,KEY_K,
/*30*/   KEY_LWIN,   0,          KEY_ESC,    0,   
/*34*/   KEY_BACKSPACE,KEY_BACKSLASH2,KEY_SPACE,KEY_TAB,
/*38*/   0,          0,          0,          0,   
/*3C*/   KEY_LCONTROL,KEY_ALT,   KEY_CAPSLOCK,KEY_LSHIFT,
/*40*/   KEY_NUMLOCK,0,          KEY_PLUS_PAD,0,   
/*44*/   KEY_ASTERISK,0,         0,          0,   
/*48*/   0,          KEY_MINUS_PAD,0,        0,   
/*4C*/   KEY_SLASH_PAD,0,        0,          0,   
/*50*/   KEY_5_PAD,  KEY_4_PAD,  KEY_3_PAD,   KEY_2_PAD,
/*54*/   KEY_1_PAD,  KEY_0_PAD,  0,          0,   
/*58*/   0,          0,          0,          KEY_9_PAD,
/*5C*/   KEY_8_PAD,  0,          0,          0,   
/*60*/   KEY_F11,    0,          KEY_F9,     KEY_F8,
/*64*/   KEY_F3,     KEY_F7,     KEY_F6,     KEY_F5,
/*68*/   KEY_F12,    0,          KEY_F10,    0,   
/*6C*/   KEY_SCRLOCK,0,          0,          0,   
/*70*/   KEY_END,    KEY_F4,     KEY_DEL,    KEY_PGUP,
/*74*/   KEY_HOME,   KEY_INSERT, 0,          0,   
/*78*/   0,          KEY_UP,     KEY_DOWN,   KEY_RIGHT,
/*7C*/   KEY_LEFT,   KEY_F1,     KEY_PGDN,   KEY_F2,
};

/*
The LookUp table to translate AppleKeyboard scan codes to lowcase ASCII chars
*/

const unsigned char key_apple_to_char[128]=
{
/*00*/   'x',      'z',      'g',      'h',
/*04*/   'f',      'd',      's',      'a',   
/*08*/   'r',      'e',      'w',      'q',
/*0C*/   'b',      '~',      'v',      'c',
/*10*/   '5',      '6',      '4',      '3',
/*14*/   '2',      '1',      't',      'y',
/*18*/   'o',      ']',      '0',      '8',
/*1C*/   '-',      '7',      '9',      '=',
/*20*/   ' ',      'j',      'l',      0xD,
/*24*/   'p',      'i',      '[',      'u',
/*28*/   '.',      'm',      'n',      '/',
/*2C*/   ',',      '\\',      ':',      'k',
/*30*/   0,         0,        27,         0,   
/*34*/   8,         '\\',     ' ',       9,
/*38*/   0,         0,         0,         0,   
/*3C*/   0,         0,         0,         0,
/*40*/   0,         0,         '+',       0,   
/*44*/   '*',      0,          0,         0,   
/*48*/   0,         '-',       0,         0,   
/*4C*/   '/',      0,          0,         0,   
/*50*/   '5',      '4',       '3',       '2',
/*54*/   '1',      '0',        0,         0,   
/*58*/   0,         0,         0,         '9',
/*5C*/   '8',       0,         0,         0,   
/*60*/   0,         0,         0,         0,
/*64*/   0,         0,         0,         0,
/*68*/   0,         0,         0,         0,   
/*6C*/   0,         0,         0,         0,   
/*70*/   0,         0,         127,       0,
/*74*/   0,         0,         0,         0,   
/*78*/   0,         0,         0,         0,
/*7C*/   0,         0,         0,         0,
};

#pragma mark todo
/*
Todo LookUp table to translate AppleKeyboard scan codes to upcase ASCII chars
   and diacrilic chars
*/

#pragma mark driverlists

_DRIVER_INFO _system_driver_list[2] ={
      {SYSTEM_MACOS, &system_macos, TRUE},
      {0, NULL, 0}
};
_DRIVER_INFO _timer_driver_list[] ={
      {TIMER_MACOS, &timer_macos, TRUE},
      {0, NULL, 0}
};
_DRIVER_INFO _mouse_driver_list[] ={
      {MOUSE_MACOS, &mouse_macos, TRUE},
      {0, NULL, 0}
};
_DRIVER_INFO _keyboard_driver_list[] ={
      {KEYBOARD_MACOS, &keyboard_macos, TRUE},
      {0, NULL, 0}
};
_DRIVER_INFO _gfx_driver_list[] ={
      {GFX_DRAWSPROCKET, &gfx_drawsprocket, TRUE},
      {0, NULL, 0}
};
_DRIVER_INFO _digi_driver_list[]={
      {DIGI_MACOS,&digi_macos,TRUE},
      {0,NULL,0}
};
_DRIVER_INFO _midi_driver_list[]={
   {MIDI_NONE,&midi_none,TRUE},
   {0,NULL,0}
};
_DRIVER_INFO _joystick_driver_list[]={
   {JOY_TYPE_NONE,&joystick_none,TRUE},
   {0,NULL,0}};

#pragma mark SYSTEM_DRIVER
SYSTEM_DRIVER system_macos ={
   SYSTEM_MACOS,
   empty_string,
   empty_string,
   "MacOs",
   _mac_init,
   _mac_exit,
   _mac_get_executable_name,
   NULL,
   _mac_set_window_title,
   _mac_message,
   _mac_assert,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   _mac_desktop_color_depth,
   _mac_yield_timeslice,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
};

#pragma mark MOUSE_DRIVER
MOUSE_DRIVER mouse_macos ={
      MOUSE_MACOS,
      empty_string,
      empty_string,
      "MacOs Mouse",
      mouse_mac_init,
      mouse_mac_exit,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
};

#pragma mark KEYBOARD_DRIVER
KEYBOARD_DRIVER keyboard_macos ={
   KEYBOARD_MACOS,
   0,
   0,
   "MacOs Key",
   0,
   key_mac_init,
   key_mac_exit,
   NULL, NULL, NULL,
   NULL,
   NULL,
   NULL
};

#pragma mark TIMER_DRIVER
TIMER_DRIVER timer_macos ={
      TIMER_MACOS,
      empty_string,
      empty_string,
      "MacOs Timer",
      mac_timer_init,
      mac_timer_exit,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
};


#pragma mark system driver routines
int _mac_init()
{
   os_type = 'MAC ';
   register_trace_handler(_mac_trace_handler);
#if (TRACE_MAC_SYSTEM)
   fprintf(stdout,"mac_init()::OK\n");
   fflush(stdout);
#endif
   _rgb_r_shift_15 = 10;         /*Ours truecolor pixel format */
   _rgb_g_shift_15 = 5;
   _rgb_b_shift_15 = 0;
   _rgb_r_shift_24 = 16;
   _rgb_g_shift_24 = 8;
   _rgb_b_shift_24 = 0;
   return 0;
}

void _mac_exit()
{
#if (TRACE_MAC_SYSTEM)
   fprintf(stdout,"mac_exit()\n");
   fflush(stdout);
#endif
}

void _mac_get_executable_name(char *output, int size){
   Str255 appName;
   OSErr            err;
   ProcessInfoRec      info;
   ProcessSerialNumber   curPSN;   

   err = GetCurrentProcess(&curPSN);

   info.processInfoLength = sizeof(ProcessInfoRec);
   info.processName = appName;
   info.processAppSpec = NULL;

   err = GetProcessInformation(&curPSN, &info);

   size=MIN(size,(unsigned char)appName[0]);
   strncpy(output,(char const *)appName+1,size);
   output[size]=0;
}

void _mac_set_window_title(const char *name){
#if (TRACE_MAC_SYSTEM)
   fprintf(stdout,"mac_set_window_title(%s)\n",name);
   fflush(stdout);
#endif
   memcpy(macwindowtitle,name,254);
   macwindowtitle[254]=0;
}

void _mac_message(const char *msg){

#if (TRACE_MAC_SYSTEM)
   fprintf(stdout,"mac_message(%s)\n",msg);
   fflush(stdout);
#endif
   memcpy(macsecuremsg,msg,254);
   macsecuremsg[254]=0;
   paramtext(macsecuremsg,"\0","\0","\0");
   Alert(rmac_message,nil);
}

void _mac_assert(const char *msg){
   debugstr(msg);
}
int _mac_desktop_color_depth(void){
   return 0;
}

void _mac_yield_timeslice(void){
   SystemTask();   
}

int _mac_trace_handler(const char *msg){
   debugstr(msg);
   return 0;
}


#pragma mark timer routines
static int mac_timer_init(void){
   _mactimerinstalled=1;
   return 0;
};
void mac_timer_exit(void){
   _mactimerinstalled=0;
};
#pragma mark mouse routines
static int mouse_mac_init(void){
#if (TRACE_MAC_MOUSE)
   
   fprintf(stdout,"mouse_mac_init()\n");
   fflush(stdout);
#endif
   _macmouseinstalled=1;
   return 1;
};
static void mouse_mac_exit(void){
#if (TRACE_MAC_MOUSE)   
   fprintf(stdout,"mouse_mac_exit()\n");
   fflush(stdout);
#endif
   _macmouseinstalled=0;
};
#pragma mark keyboard routines
static int key_mac_init(void){
#if (TRACE_MAC_KBRD)
   fprintf(stdout,"key_mac_init()\n");
   fflush(stdout);
#endif
   GetKeys(KeyNow);
   BlockMove(KeyNow,KeyOld,sizeof(KeyMap));
   _mackeyboardinstalled=1;
   return 0;
};
static void key_mac_exit(void){
#if (TRACE_MAC_KBRD)
   fprintf(stdout,"key_mac_exit()\n");
   fflush(stdout);
#endif
   _mackeyboardinstalled=0;
};
/*EXTERN_API( SInt16 ) LMGetKeyThresh(void)                                       TWOWORDINLINE(0x3EB8, 0x018E);*/
/*EXTERN_API( void ) LMSetKeyThresh(SInt16 value)                                    TWOWORDINLINE(0x31DF, 0x018E);*/
/*EXTERN_API( SInt16 ) LMGetKeyRepThresh(void)                                    TWOWORDINLINE(0x3EB8, 0x0190);*/
/*EXTERN_API( void ) LMSetKeyRepThresh(SInt16 value)                                 TWOWORDINLINE(0x31DF, 0x0190);*/

#pragma mark mac suport
char *strdup(const char *p){
   char *tmp=malloc(strlen(p));
   if(tmp){
      strcpy(tmp,p);
   };
   return tmp;
};
//--------------------------------------------------------------DoWeHaveColor
// Simple function that returns TRUE if we're running on a Mac thatÉ
// is running Color Quickdraw.
Boolean DoWeHaveColor (void)
{
   SysEnvRec      thisWorld;
   
   SysEnvirons(2, &thisWorld);      // Call the old SysEnvirons() function.
   return (thisWorld.hasColorQD);   // And return whether it has Color QuickDraw.
}

//--------------------------------------------------------------DoWeHaveSystem605

// Another simple "environment" function.Returns TRUE if the Mac we're runningÉ
// on has System 6.0.5 or more recent.(6.0.5 introduced the ability to switchÉ
// color depths.)

Boolean DoWeHaveSystem605 (void)
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


void ptoc(StringPtr pstr,char *cstr)
{
   BlockMove(pstr+1,cstr,pstr[0]);
   cstr[pstr[0]]='\0';
};
short FindMainDevice (void)
{
   MainGDevice = GetMainDevice();
   if (MainGDevice == 0L)return 1;
   return 0;
}
Boolean RTrapAvailable(short tNumber,TrapType tType)
{
   return NGetTrapAddress( tNumber, tType ) != NGetTrapAddress( _Unimplemented, ToolTrap);      
};
pascal void   MyTask(TMTaskPtr tmTaskPtr)
{
   if(_mactimerinstalled)_handle_timer_tick(MSEC_TO_TIMER(50));
   if(_macmouseinstalled)
   {
      Point pt;
      pt=(*((volatile Point *)0x082C));/*LMGetMouseLocation();*/
      _mouse_b = (*((volatile UInt8 *)0x0172))&0x80?0:1;
      _mouse_x = pt.h;
      _mouse_y = pt.v;
      _handle_mouse_input();
   };
   if(_mackeyboardinstalled)
   {
      UInt32 xKeys;
      int a,row,base,keycode;
      KeyNow[0]=(*(UInt32*)0x174);//GetKeys cannot be called at interrupt time
      KeyNow[1]=(*(UInt32*)0x178);
      KeyNow[2]=(*(UInt32*)0x17C);
      KeyNow[3]=(*(UInt32*)0x180);
      for(row=0,base=0;row<4;row++,base+=32){
         xKeys=KeyOld[row]^KeyNow[row];
         if(xKeys){
            for(a=0;a<32;a++){
               if(BitTst(&xKeys,a)){
                  keycode=key_apple_to_allegro[a+base];
                  if(keycode){
                     if(BitTst(&KeyNow[row],a)){
                        _handle_key_press(key_apple_to_char[a+base],keycode);
                     }
                     else{
                        _handle_key_release(keycode);
                     };
                  };
               };
            };
         };
         KeyOld[row]=KeyNow[row];
      };      
   };
   PrimeTime((QElemPtr)tmTaskPtr, 50);
};

/*
 *Executs the needed clean before return to system
 */
void maccleanup(){
   if(_timer_running)RmvTime((QElemPtr)&theTMTask);
   
   DSpShutdown();
   FlushEvents(nullEvent|mouseDown|mouseUp|keyDown|keyUp|autoKey|updateEvt,0);
   ShowCursor();
   
};
   
/*
 *
 *The entry point for MacAllegro programs setup mac toolbox application memory 
 *This routine should be called before main() this is do by -main MacEntry option in final Link
 *if not called then the program can crash!!!
 *
 */

void MacEntry(){
   char argbuf[512];
   char *argv[64];
   int argc;
   int i, q;
   SysEnvRec envRec;long   heapSize;
   Str255 name;
   OSErr theError;

   InitGraf((Ptr) &qd.thePort);      /*init the normal mac toolbox*/
   InitFonts();
   InitWindows();
   InitMenus();
   TEInit();
   InitDialogs(nil);
   InitCursor();
   InitPalettes();

/*memory setup*/
   (void) SysEnvirons( curSysEnvVers, &envRec );if (envRec.machineType < 0 )ExitToShell();
   if (kStackNeeded > StackSpace()){
      SetApplLimit((Ptr) ((long) GetApplLimit() - kStackNeeded + StackSpace()));
   }
   heapSize = (long) GetApplLimit() - (long) ApplicationZone();
   if ( heapSize < kHeapNeeded){
      _mac_message("no enough memory");
      ExitToShell();
   };
   MaxApplZone();
   if(!(RTrapAvailable(_WaitNextEvent, ToolTrap)))ExitToShell();


   if ((Ptr) DSpStartup == (Ptr) kUnresolvedCFragSymbolAddress)return;
   theError=DSpStartup();
   if(theError!=noErr)return;
   State=0;

   GetDateTime((unsigned long*) &qd.randSeed);
   
   HGetVol(name,&MainVol,&MainDir);   /*inicializa volume e directory*/
   ptoc(name,volname);
   UppercaseStripDiacritics(volname,strlen(volname),smSystemScript);


   if(MainCTable==NULL){            /*get our ColorTable*/
      init_mypalette();
   };
   
   _mac_init_system_bitmap();         /*init our system bitmap vtable*/

   FindMainDevice ();               /*get our main display*/

   atexit(&maccleanup);            /**/

//   _al_getdcwd(0, argbuf, 511);

   /* can't use parameter because it doesn't include the executable name */
//   i=strlen(argbuf);
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
   theTMTask.tmAddr = NewTimerProc(MyTask); /*install our TMtask where are hooked the mouse,keyboard,timer polls*/
   theTMTask.tmWakeUp = 0;
   theTMTask.tmReserved = 0;
   InsTime((QElemPtr)&theTMTask);
   PrimeTime((QElemPtr)&theTMTask, 40);
   _timer_running=TRUE;

   HideCursor();
#if (TRACE_MAC_SYSTEM)
   fprintf(stdout,"MacEntry Done\n");
   fflush(stdout);
#endif
   main(argc, argv);
   FlushEvents(everyEvent,0);
}
