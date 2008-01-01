#include <allegro.h>
#include "alleggl.h"

volatile int chrono = 0;

void the_timer(void) {
	chrono++;
} END_OF_FUNCTION(the_timer);


int poly_coords[12] = {150, 400,   200, 400,   225, 425,   200, 450,   175, 450,   175, 425};


/* Some common colours */
#define CYAN makecol(0,255,255)
#define BLACK makecol(0,0,0)
int main()
{
	float rotate = 0;
	BITMAP *save_bmp;
	BITMAP *sub_screen, *sub_screen2, *vid_bitmap, *mem_bitmap, *vid_bitmap2,
	       *trans_mem_bitmap, *trans_vid_bitmap, *trans_vid_bitmap2,
	       *large_vid_bitmap, *proxy_vid_bitmap, *vid_pattern;
	RLE_SPRITE *rle_sprite, *trans_rle_sprite;
	int keypress;
	int frame_count = 0, frame_count_time = 0;
	int x, y;
	float fps_rate = 0.0;

	allegro_init();
	install_allegro_gl();
	install_timer();

	allegro_gl_set(AGL_DOUBLEBUFFER, 1);
	allegro_gl_set(AGL_WINDOWED, TRUE);
	allegro_gl_set(AGL_COLOR_DEPTH, 32);
	allegro_gl_set(AGL_SUGGEST, AGL_DOUBLEBUFFER | AGL_WINDOWED
	             | AGL_COLOR_DEPTH);

	if(set_gfx_mode(GFX_OPENGL, 640, 480, 0, 0)<0) {
		allegro_message ("Error setting OpenGL graphics mode:\n%s\n"
		                 "Allegro GL error : %s\n",
		                 allegro_error, allegro_gl_error);
		exit(0);
	}

	/* Needed because of a bug. */
	allegro_gl_set_allegro_mode();
	allegro_gl_unset_allegro_mode();

	save_bmp = create_bitmap (640, 480);
	ASSERT (save_bmp);

	sub_screen  = create_sub_bitmap(screen, 320, 0, 128, 96);
	sub_screen2 = create_sub_bitmap(screen, 448, 0, 128, 96);
	ASSERT(sub_screen);
	ASSERT(sub_screen2);

	vid_bitmap = create_video_bitmap(50, 50);
	clear_to_color(vid_bitmap, makecol(255,0,255));
	rect(vid_bitmap, 10, 10, 39, 39, makecol(255, 255, 255));
	rectfill(vid_bitmap, 12, 12, 37, 37, makecol(255, 0, 0));
	triangle(vid_bitmap, 9, 9, 46, 3, 0, 16, makecol(0, 255, 0));

	mem_bitmap = create_bitmap(50, 50);
	clear(mem_bitmap);
	blit(vid_bitmap, mem_bitmap, 25, 25, 10, 10, 50, 50);

	vid_pattern = create_video_bitmap(32, 32);
	clear_bitmap(vid_pattern);
	blit(vid_bitmap, vid_pattern, 0, 0, 0, 0, 32, 32);

	TRACE("Starting main loop\n");
	save_bitmap("dumbtest.bmp", mem_bitmap, NULL);

	rle_sprite = get_rle_sprite(mem_bitmap);

	vid_bitmap2 = create_video_bitmap(64, 64);
	clear_to_color(vid_bitmap2, makecol(255,0,255));
	blit(mem_bitmap, vid_bitmap2, 0, 0, 10, 10, 50, 50);

	trans_mem_bitmap = create_bitmap_ex(32, mem_bitmap->w, mem_bitmap->h);
	for (y = 0; y < mem_bitmap->h; ++y) {
		for (x = 0; x < mem_bitmap->w; ++x) {
			int col = getpixel(mem_bitmap, x, y);
			putpixel(trans_mem_bitmap, x, y, makeacol32(getr(col), getg(col), getb(col), y*4));
		}
	}

	allegro_gl_set_video_bitmap_color_depth(32);

	trans_vid_bitmap2 = create_video_bitmap(64, 64);
	clear_bitmap(trans_vid_bitmap2);
	blit(trans_mem_bitmap, trans_vid_bitmap2, 0, 0, 10, 10, 50, 50);

	trans_vid_bitmap = create_video_bitmap(50, 50);
	clear_to_color(trans_mem_bitmap, makeacol32(255, 0, 255, 127));
	rect(trans_mem_bitmap, 10, 10, 39, 39, makeacol32(255, 255, 255, 127));
	rectfill(trans_mem_bitmap, 12, 12, 37, 37, makeacol32(255, 0, 0, 192));
	triangle(trans_mem_bitmap, 9, 9, 46, 3, 0, 16, makeacol32(0, 255, 0, 192));
	blit(trans_mem_bitmap, trans_vid_bitmap, 0, 0, 0, 0, 50, 50);

	trans_rle_sprite = get_rle_sprite(trans_mem_bitmap);

	allegro_gl_set_video_bitmap_color_depth(-1);

	large_vid_bitmap = create_video_bitmap(120, 120);
	clear_to_color(large_vid_bitmap, BLACK);
	rect(large_vid_bitmap, 0, 0, large_vid_bitmap->w-1, large_vid_bitmap->h-1, makecol(255, 255, 0));
	line(large_vid_bitmap, 0, 0, large_vid_bitmap->w, 30, makecol(255, 0, 0));
	line(large_vid_bitmap, large_vid_bitmap->w, 0, 0, large_vid_bitmap->h, makecol(255, 0, 0));
	
	proxy_vid_bitmap = create_video_bitmap(100, 100);
	clear_to_color(proxy_vid_bitmap, CYAN);
	line(proxy_vid_bitmap, 0, 0, proxy_vid_bitmap->w, proxy_vid_bitmap->h, makecol(255, 0, 0));

	/* memory -> video */
	blit(mem_bitmap, proxy_vid_bitmap, 0, 0, 5, 5, mem_bitmap->w, mem_bitmap->h);

	set_alpha_blender();
	/* memory -> video trans */
	draw_trans_sprite(proxy_vid_bitmap, trans_mem_bitmap, mem_bitmap->w, 5);
	/* video -> video trans */
	draw_trans_sprite(proxy_vid_bitmap, trans_vid_bitmap, 5, mem_bitmap->h);

{	/* textured 3d polygon -> video bitmap */
	V3D_f v1, v2, v3, v4;
	v1.x = 50; v1.y = 50; v1.u = 0; v1.v = 0;
	v2.x = 98; v2.y = 50; v2.u = 70; v2.v = 0;
	v3.x = 98; v3.y = 98; v3.u = 70; v3.v = 70;
	v4.x = 50; v4.y = 98; v4.u = 0; v4.v = 70;
	quad3d_f(proxy_vid_bitmap, POLYTYPE_ATEX, vid_pattern, &v1, &v2, &v3, &v4);
}

    /* video -> video */
	draw_sprite(large_vid_bitmap, proxy_vid_bitmap, 5, 5);

	textout_ex(large_vid_bitmap, font, "some text", 15, 35, -1, -1);

	install_keyboard();

	LOCK_VARIABLE(chrono);
	LOCK_FUNCTION(the_timer);

	install_int(the_timer, 5);

	/* Setup OpenGL like we want */
	glEnable(GL_TEXTURE_2D);


	do {
		if (keypressed())
			keypress = readkey();
		else
			keypress = 0;

		rotate+=1.0;
		if(rotate>360.0) rotate=0;

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		glPushMatrix();
		glRotatef(45, 1, 0, 0);
		glRotatef(rotate, 0.0, 1.0, 0.0);
		glBegin(GL_QUADS);
			glColor3f(1.0, 0.0, 1.0);
			glVertex3f(0.1, 0.1, 0.1);
			glVertex3f(0.1, -0.1, 0.1);
			glVertex3f(-0.1, -0.1, 0.1);
			glVertex3f(-0.1, 0.1, 0.1);
			glColor3f(1.0, 0.0, 0.0);
			glVertex3f(0.1, 0.1, -0.1);
			glVertex3f(0.1, -0.1, -0.1);
			glVertex3f(-0.1, -0.1, -0.1);
			glVertex3f(-0.1, 0.1, -0.1);
			glColor3f(0.0, 1.0, 0.0);
			glVertex3f(0.1, 0.1, 0.1);
			glVertex3f(0.1, -0.1, 0.1);
			glVertex3f(0.1, -0.1, -0.1);
			glVertex3f(0.1, 0.1, -0.1);
		glEnd();
		glPopMatrix();

		frame_count++;

		if (frame_count >= 20) {
			if (chrono - frame_count_time)
				fps_rate = frame_count * 200.0 / (chrono - frame_count_time);
			else
				fps_rate = frame_count * 200.0;
			frame_count_time = chrono;
			frame_count = 0;
		}

		allegro_gl_set_allegro_mode();
		line(screen, 0, 0, 640, 480, makecol(0xff, 0, 0xff));
		triangle(screen, 115, 21, 150, 350, 5, 90, makecol(0xff, 0, 0));
		triangle(screen, 145, 21, 190, 350, 45, 90, makecol(0, 0, 0xff));
		triangle(screen, 205, 21, 240, 350, 95, 90, makecol(0, 0xff, 0xff));
		triangle(screen, 245, 21, 290, 350, 145, 90, makecol(0xff, 0xff, 0));
		rect(screen, 0, 0, 639, 479, makecol(0xff, 0xbf, 0x00));
		putpixel(screen, 0, 0, makecol(0xff, 0xff, 0xff));
		putpixel(screen, 639, 0, makecol(0xff, 0xff, 0xff));
		putpixel(screen, 0, 479, makecol(0xff, 0xff, 0xff));
		putpixel(screen, 639, 479, makecol(0xff, 0xff, 0xff));
		putpixel(screen, 0, 478, makecol(0xff, 0xff, 0xff));
		putpixel(screen, 639, 478, makecol(0xff, 0xff, 0xff));
		rectfill(screen, 16, 350, 256, 440, makecol(0xff, 0xff, 0xff));

		textout_ex(screen, font, "Partially cut off text", -3, 450,
		        makecol(255,255,255), -1);	
		textout_ex(screen, font, "Hello World!", 40, 380, BLACK, -1);
		textprintf_ex(screen, font, 40, 400, makecol(0, 0, 0), -1,
		           "FPS: %.2f", fps_rate);

		/* Now here's the interesting part. This section of code tests
		 * the various blit combinations supported by AllegroGL. That is:
		 *                 screen -> sub-bitmap of screen
		 *   sub-bitmap of screen -> sub-bitmap of screen
		 *                 screen -> screen
		 *           video bitmap -> screen
		 *                 screen -> memory bitmap
		 *          memory bitmap -> screen
		 *             rle sprite -> screen
		 *
		 *   Other combinations are tested either at the init.
		 */

		blit (screen, sub_screen, 256, 192, 0, 0, 128, 96);
		blit (sub_screen, sub_screen2, 0, 0, 0, 0, 128, 96);
		blit (screen, screen, 0, 0, 400, 150, 90, 90);
		blit (vid_bitmap, screen, 0, 0, 0, 0, 50, 50);
		blit (vid_bitmap2, screen, 0, 0, 50, 0, 50, 50);
		blit (screen, mem_bitmap, 0, 0, 0, 0, 50, 50);
		blit (mem_bitmap, screen, 0, 0, 100, 0, 50, 50);

		masked_blit (vid_bitmap, screen, 0, 0, 0, 50, 50, 50);
		masked_blit (vid_bitmap2, screen, 0, 0, 50, 50, 50, 50);
		masked_blit (mem_bitmap, screen, 0, 0, 100, 50, 50, 50);

		draw_sprite(screen, vid_bitmap, 0, 100);
		draw_sprite(screen, vid_bitmap2, 50, 100);
		draw_sprite(screen, mem_bitmap, 100, 100);
		draw_rle_sprite(screen, rle_sprite, 150, 100);

		set_alpha_blender();
		draw_trans_sprite(screen, trans_vid_bitmap, 0, 150);
		draw_trans_sprite(screen, trans_vid_bitmap2, 50, 150);
		draw_trans_sprite(screen, trans_mem_bitmap, 100, 150);
		draw_trans_rle_sprite(screen, trans_rle_sprite, 150, 150);

		pivot_scaled_sprite(screen, vid_bitmap, 0, 200, 0, 0, itofix(6), ftofix(1.1));
		pivot_scaled_sprite(screen, vid_bitmap2, 50, 200, 0, 0, itofix(6), ftofix(1.1));
		pivot_scaled_sprite(screen, mem_bitmap, 100, 200, 0, 0, itofix(6), ftofix(1.1));

		draw_sprite_ex(screen, trans_vid_bitmap, 0, 250, DRAW_SPRITE_TRANS, DRAW_SPRITE_VH_FLIP);
		draw_sprite_ex(screen, trans_vid_bitmap2, 50, 250, DRAW_SPRITE_TRANS, DRAW_SPRITE_VH_FLIP);
		draw_sprite_ex(screen, trans_mem_bitmap, 100, 250, DRAW_SPRITE_TRANS, DRAW_SPRITE_VH_FLIP);

		/* Write the captions for each image */
		textout_ex(screen, font, "blit", 200, 25, CYAN, BLACK);
		textout_ex(screen, font, "masked_blit", 200, 75, CYAN, BLACK);
		textout_ex(screen, font, "draw_sprite", 200, 125, CYAN, BLACK);
		textout_ex(screen, font, "draw_trans_sprite", 200, 175, CYAN, BLACK);
		textout_ex(screen, font, "pivot_scaled_sprite", 200, 225, CYAN, BLACK);
		textout_ex(screen, font, "draw_sprite_ex", 200, 275, CYAN, BLACK);
		textout_centre_ex(screen, font, "vid", 25, 325, CYAN, BLACK);
		textout_centre_ex(screen, font, "NPOT", 25, 335, CYAN, BLACK);
		textout_centre_ex(screen, font, "vid", 75, 325, CYAN, BLACK);
		textout_centre_ex(screen, font, "POT", 75, 325, CYAN, BLACK);
		textout_centre_ex(screen, font, "mem",125, 325, CYAN, BLACK);
		textout_centre_ex(screen, font, "rle",175, 325, CYAN, BLACK);

		drawing_mode(DRAW_MODE_SOLID, 0, 0, 0);
		rectfill(screen, 250, 400, 300, 450, CYAN);
		drawing_mode(DRAW_MODE_XOR, NULL, 0, 0);
		rect(screen, 270, 420, 280, 430, CYAN);
		solid_mode();
		
		polygon(screen, 6, poly_coords, CYAN);
		drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
		set_trans_blender(305, 255, 305, 128);
		rectfill(screen, 170, 400, 200, 450, BLACK);

		drawing_mode(DRAW_MODE_COPY_PATTERN, vid_pattern, 50, 50);
		triangle(screen, 320, 350, 430, 350, 320, 480, 0);
		rectfill(screen, 400, 400, 450, 450, 0);
		solid_mode();

		/* video -> screen stertching */
		stretch_blit(large_vid_bitmap, screen, 0, 0, large_vid_bitmap->w, large_vid_bitmap->h,
		             460, 350, large_vid_bitmap->w+50, large_vid_bitmap->h);

		allegro_gl_unset_allegro_mode();

		if ((keypress & 0xdf) == 'S')
			blit (screen, save_bmp, 0, 0, 0, 0, 640, 480);

		glFlush();
		allegro_gl_flip();

		if ((keypress & 0xdf) == 'S') {
		  save_bitmap ("screen.bmp", save_bmp, desktop_palette);
		}

		rest(2);

	}while(!key[KEY_ESC]);

	allegro_gl_set_allegro_mode();
	glDrawBuffer(GL_FRONT);
	glReadBuffer(GL_FRONT);

	textout_ex(screen, font, "Press any key to exit...", 0, 470,
	        makecol(255, 255, 255), -1);

	/* check the dialog saved the background OK */
	readkey();

	destroy_bitmap(save_bmp);
	destroy_bitmap(sub_screen);
	destroy_bitmap(sub_screen2);
	destroy_bitmap(mem_bitmap);
	destroy_bitmap(vid_bitmap);
	destroy_bitmap(vid_bitmap2);
	destroy_bitmap(trans_mem_bitmap);
	destroy_bitmap(trans_vid_bitmap);
	destroy_bitmap(trans_vid_bitmap2);
	destroy_bitmap(proxy_vid_bitmap);
	destroy_bitmap(large_vid_bitmap);
	destroy_bitmap(vid_pattern);
	destroy_rle_sprite(rle_sprite);
	destroy_rle_sprite(trans_rle_sprite);
	return 0;
}

END_OF_MAIN()

