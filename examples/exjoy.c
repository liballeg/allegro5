/*
 *    Example program for the Allegro library, by Grzegorz Adam Hankiewicz
 *
 *    This program uses the Allegro library to detect and read the value
 *    of a joystick. The output of the program is a small target sight
 *    on the screen which you can move. At the same time the program will
 *    tell you what you are doing with the joystick (moving or firing).
 */


#include <allegro.h>



int main(void)
{
   BITMAP *bmp;            /* we create a pointer to a virtual screen */
   int x=160, y=100;       /* these will be used to show the target sight */
   int analogmode;
   AL_CONST char *msg;
   int c;

   if (allegro_init() != 0)/* you NEED this man! ;-) */
      return 1;

   install_keyboard();     /* ahh... read the docs. I will explain only
			    * joystick specific routines
			    */

   if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
	 return 1;
      }
   }

   set_palette(default_palette);
   clear_bitmap(screen);
   textout_centre_ex(screen, font, "Please center the joystick", SCREEN_W/2,
		     SCREEN_H/2 - 36, palette_color[255], 0);
   textout_centre_ex(screen, font, "and press a key.", SCREEN_W/2,
		     SCREEN_H/2 - 20, palette_color[255], 0);

   if ((readkey()&0xFF) == 27)
      return 0;

   /* the first thing is to initialise the joystick driver */
   if (install_joystick(JOY_TYPE_AUTODETECT) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error initialising joystick\n%s\n", allegro_error);
      return 1;
   }

   /* make sure that we really do have a joystick */
   if (!num_joysticks) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error: joystick not found\n");
      return 1;
   }

   /* before using the joystick, we have to calibrate it. This loop only
    * calibrates joystick number 0, but you could do the same thing for
    * other sticks if they are present (the num_joysticks variable will 
    * tell you how many there are).
    */
   while (joy[0].flags & JOYFLAG_CALIBRATE) {
      msg = calibrate_joystick_name(0);

      clear_bitmap(screen);
      textout_centre_ex(screen, font, msg, SCREEN_W/2, 64, palette_color[255], 0);
      textout_centre_ex(screen, font, "and press a key.", SCREEN_W/2, 80, palette_color[255], 0);

      if ((readkey()&0xFF) == 27)
	 return 0;

      if (calibrate_joystick(0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Error calibrating joystick!\n");
	 return 1;
      }
   }

   /* if this joystick supports analogue input, ask the user whether to
    * use digital or analogue mode. If it is only a digital pad, we don't
    * bother with this question.
   */
   if (joy[0].stick[0].flags & JOYFLAG_ANALOGUE) {
      clear_bitmap(screen);
      textout_centre_ex(screen, font, "Now press 'D' to use a digital",
			SCREEN_W/2, 64, palette_color[255], 0);
      textout_centre_ex(screen, font, "joystick or 'A' for analogue mode.",
			SCREEN_W/2, 80, palette_color[255], 0);

      for (;;) {
	 c = readkey()&0xFF;

	 if ((c=='d') || (c=='D')) {
	    analogmode = FALSE;
	    break;
	 }
	 else if ((c=='a') || (c=='A')) {
	    analogmode = TRUE;
	    break;
	 }
	 else if (c == 27)
	    return 0;
      }
   }
   else
      analogmode = FALSE;

   drawing_mode(DRAW_MODE_XOR, 0, 0, 0);
   clear_keybuf();

   bmp = create_bitmap(320, 200);
   clear_bitmap(bmp);

   do {
      poll_joystick();     /* we HAVE to do this to read the joystick */

      clear_bitmap(bmp);

      textout_centre_ex(bmp, font, joystick_driver->name, 160, 150, palette_color[255], 0);

      if (analogmode)
	 textout_centre_ex(bmp, font, "Analog mode selected", 160, 160, palette_color[255], 0);
      else
	 textout_centre_ex(bmp, font, "Digital mode selected", 160, 160, palette_color[255], 0);

      textout_centre_ex(bmp, font, "Move the joystick all around", 160, 170, palette_color[255], 0);
      textout_centre_ex(bmp, font, "Press any key to exit", 160, 180, palette_color[255], 0);
      textout_centre_ex(bmp, font, "Made by Grzegorz Adam Hankiewicz", 160, 190, palette_color[255], 0);

      /* if we detect any buttons, we print a message on the screen */
      for (c=0; c<joy[0].num_buttons; c++) {
	 if (joy[0].button[c].b)
	    textprintf_centre_ex(bmp, font, 160, c*10, palette_color[15], 0, "%s pressed", joy[0].button[c].name);
      }

      if (!analogmode) {
	 /* now we have to check individually every possible movement
	  * and actualize the coordinates of the target sight.
	  */
	 if (joy[0].stick[0].axis[0].d1) {
	    if (x > 0)
	       x--;
	    textout_centre_ex(bmp, font, "Left", 120, 100, palette_color[255], 0);
	 }
	 if (joy[0].stick[0].axis[0].d2) {
	    if (x < 319)
	       x++;
	    textout_centre_ex(bmp, font, "Right", 200, 100, palette_color[255], 0);
	 }
	 if (joy[0].stick[0].axis[1].d1) {
	    if (y > 0)
	       y--;
	    textout_centre_ex(bmp, font, "Up", 160, 70, palette_color[255], 0);
	 }
	 if (joy[0].stick[0].axis[1].d2) {
	    if (y < 199)
	       y++;
	    textout_centre_ex(bmp, font, "Down", 160, 130, palette_color[255], 0);
	 }
      }
      else {
	 /* yeah! Remember the 'ifs' of the digital part? This looks
	  * much better, only 2 lines.
	  */
	 x += joy[0].stick[0].axis[0].pos/40;
	 y += joy[0].stick[0].axis[1].pos/40;

	 /* for informational purposes, show the input values on screen */
         textprintf_ex(bmp, font, 0,  0, palette_color[255], 0, "Axis 0: %d", joy[0].stick[0].axis[0].pos);
         textprintf_ex(bmp, font, 0, 10, palette_color[255], 0, "Axis 1: %d", joy[0].stick[0].axis[1].pos);

	 /* by checking if the values were positive or negative, we
	  * can know in which the direction the user pulled the joy.
	  */
	 if (joy[0].stick[0].axis[0].pos/40 < 0) 
	    textout_centre_ex(bmp, font, "Left", 120, 100, palette_color[255], 0);

	 if (joy[0].stick[0].axis[0].pos/40 > 0) 
	    textout_centre_ex(bmp, font, "Right", 200, 100, palette_color[255], 0);

	 if (joy[0].stick[0].axis[1].pos/40 < 0) 
	    textout_centre_ex(bmp, font, "Up", 160, 70, palette_color[255], 0);

	 if (joy[0].stick[0].axis[1].pos/40 > 0) 
	    textout_centre_ex(bmp, font, "Down", 160, 130, palette_color[255], 0);

	 /* WARNING! An analog joystick can move more than 1 pixel at
	  * a time and the checks we did with the digital part don't
	  * work any longer because the steps of the target sight could
	  * 'jump' over the limits.
	  * To avoid this, we just check if the target sight has gone
	  * out of the screen. If yes, we put it back at the border.
	  */
	 if (x > 319)
	    x = 319;

	 if (x < 0) 
	    x = 0;

	 if (y < 0) 
	    y = 0;

	 if (y > 199) 
	    y = 199;
      }

      /* this draws the target sight. */
      circle(bmp, x, y, 5, palette_color[255]);
      putpixel(bmp, x, y, palette_color[255]);
      putpixel(bmp, x+1, y, palette_color[255]);
      putpixel(bmp, x, y+1, palette_color[255]);
      putpixel(bmp, x-1, y, palette_color[255]);
      putpixel(bmp, x, y-1, palette_color[255]);
      putpixel(bmp, x+5, y, palette_color[255]);
      putpixel(bmp, x, y+5, palette_color[255]);
      putpixel(bmp, x-5, y, palette_color[255]);
      putpixel(bmp, x, y-5, palette_color[255]);

      blit(bmp, screen, 0, 0, SCREEN_W/2 - 160, SCREEN_H/2 - 100, 320, 200);

   } while (!keypressed());

   destroy_bitmap(bmp);

   return 0;
}

END_OF_MAIN()
