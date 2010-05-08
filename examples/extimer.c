/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to use the timer routines.
 *    These can be a bit of a pain, because you have to be sure
 *    you lock all the memory that is used inside your interrupt
 *    handlers.  The first part of the example shows a basic use of
 *    timing using the blocking function rest(). The second part
 *    shows how to use three timers with different frequencies in
 *    a non blocking way.
 */


#include <allegro.h>



/* these must be declared volatile so the optimiser doesn't mess up */
volatile int x = 0;
volatile int y = 0;
volatile int z = 0;



/* timer interrupt handler */
void inc_x(void)
{
   x++;
}

END_OF_FUNCTION(inc_x)



/* timer interrupt handler */
void inc_y(void)
{
   y++;
}

END_OF_FUNCTION(inc_y)



/* timer interrupt handler */
void inc_z(void)
{
   z++;
}

END_OF_FUNCTION(inc_z)



int main(void)
{
   int c;

   if (allegro_init() != 0)
      return 1;
   install_keyboard(); 
   install_timer();

   if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
	 return 1;
      }
   }

   set_palette(desktop_palette);
   clear_to_color(screen, makecol(255, 255, 255));

   textprintf_centre_ex(screen, font, SCREEN_W/2, 8, makecol(0, 0, 0),
			makecol(255, 255, 255), "Driver: %s",
			timer_driver->name);

   /* use rest() to delay for a specified number of milliseconds */
   textprintf_centre_ex(screen, font, SCREEN_W/2, 48, makecol(0, 0, 0),
			makecol(255, 255, 255), "Timing five seconds:");

   for (c=1; c<=5; c++) {
      textprintf_centre_ex(screen, font, SCREEN_W/2, 62+c*10,
			   makecol(0, 0, 0), makecol(255, 255, 255), "%d", c);
      rest(1000);
      if (keypressed()) {
	 return 0;
      }
   }

   textprintf_centre_ex(screen, font, SCREEN_W/2, 142, makecol(0, 0, 0),
			makecol(255, 255, 255),
			"Press a key to set up interrupts");
   readkey();

   /* all variables and code used inside interrupt handlers must be locked */
   LOCK_VARIABLE(x);
   LOCK_VARIABLE(y);
   LOCK_VARIABLE(z);
   LOCK_FUNCTION(inc_x);
   LOCK_FUNCTION(inc_y);
   LOCK_FUNCTION(inc_z);

   /* the speed can be specified in milliseconds (this is once a second) */
   install_int(inc_x, 1000);

   /* or in beats per second (this is 10 ticks a second) */
   install_int_ex(inc_y, BPS_TO_TIMER(10));

   /* or in seconds (this is 10 seconds a tick) */
   install_int_ex(inc_z, SECS_TO_TIMER(10));

   /* the interrupts are now active... */
   while (!keypressed()) {
      textprintf_centre_ex(screen, font, SCREEN_W/2, 176, makecol(0, 0, 0),
			   makecol(255, 255, 255), "x=%d, y=%d, z=%d", x, y, z);
      rest(1);
   }

   return 0;
}

END_OF_MAIN()
