#ifndef gf_included_allglint_h
#define gf_included_allglint_h

#include "alleggl.h"


struct allegro_gl_info {
	float version;          /* OpenGL version */
	int num_texture_units;  /* Number of texture units */
	int max_texture_size;   /* Maximum texture size */
	int is_voodoo3_and_under; /* Special cases for Voodoo 1-3 */
	int is_voodoo;          /* Special cases for Voodoo cards */
	int is_matrox_g200;     /* Special cases for Matrox G200 boards */
	int is_ati_rage_pro;    /* Special cases for ATI Rage Pro boards */
	int is_ati_radeon_7000; /* Special cases for ATI Radeon 7000 */
	int is_ati_r200_chip;	/* Special cases for ATI card with chip R200 */
	int is_mesa_driver;     /* Special cases for MESA */
};



struct allegro_gl_driver {
	void (*flip) (void);
	void (*gl_on) (void);
	void (*gl_off) (void);
	void (*screen_masked_blit)(struct BITMAP *source, int source_x,
	                           int source_y, int dest_x, int dest_y,
	                           int width, int height, int flip_dir,
	                           int blit_type);
};



struct allegro_gl_rgba_size {
	int r, g, b, a;
};



struct allegro_gl_indexed_size {
	int bpp;
};



union allegro_gl_pixel_size {
	struct allegro_gl_rgba_size rgba;
	struct allegro_gl_indexed_size indexed;
};



struct allegro_gl_display_info {
	int allegro_format;
	union allegro_gl_pixel_size pixel_size;
	int colour_depth;
	union allegro_gl_pixel_size accum_size;
	int doublebuffered;
	int stereo;
	int aux_buffers;
	int depth_size;
	int stencil_size;
	int w, h, x, y;
	int r_shift, g_shift, b_shift, a_shift;
	int packed_pixel_type, packed_pixel_format;
	int rmethod; /* type of rendering, 0: software; 1: hardware, 2: unknown (X Win) */
	int fullscreen;
	int vidmem_policy;
	int sample_buffers;
	int samples;
	int float_color;
	int float_depth;
};

extern struct allegro_gl_display_info allegro_gl_display_info;
extern struct allegro_gl_driver *__allegro_gl_driver;
extern struct allegro_gl_info allegro_gl_info;
extern int __allegro_gl_required_settings, __allegro_gl_suggested_settings;
extern int __allegro_gl_valid_context;
extern int __allegro_gl_use_alpha;
extern GLint __allegro_gl_texture_read_format[5];
extern GLint __allegro_gl_texture_components[5];

#ifdef ALLEGRO_WINDOWS
extern HDC __allegro_gl_hdc;
#endif

int __allegro_gl_make_power_of_2(int x);

void __allegro_gl_reset_scorer (void);
int __allegro_gl_score_config (int refnum, struct allegro_gl_display_info *dinfo);
int __allegro_gl_best_config (void);

void __allegro_gl_set_allegro_image_format (int big_endian);

void __allegro_gl_fill_in_info(void);

void __allegro_gl_manage_extensions(void);
void __allegro_gl_unmanage_extensions(void);
void __allegro_gl_print_extensions(const char * extension);
int __allegro_gl_look_for_an_extension(AL_CONST char *name,
                                                  AL_CONST GLubyte * extensions);


extern _DRIVER_INFO *(*saved_gfx_drivers) (void);

typedef void (* BLIT_BETWEEN_FORMATS_FUNC) (struct BITMAP*, struct BITMAP*, int, int, int, int, int, int);
extern BLIT_BETWEEN_FORMATS_FUNC __blit_between_formats8;
extern BLIT_BETWEEN_FORMATS_FUNC __blit_between_formats15;
extern BLIT_BETWEEN_FORMATS_FUNC __blit_between_formats16;
extern BLIT_BETWEEN_FORMATS_FUNC __blit_between_formats24;
extern BLIT_BETWEEN_FORMATS_FUNC __blit_between_formats32;

void allegro_gl_memory_blit_between_formats(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);

int allegro_gl_set_mouse_sprite(BITMAP *sprite, int xfocus, int yfocus);
int allegro_gl_show_mouse(BITMAP* bmp, int x, int y);
void allegro_gl_hide_mouse(void);
void allegro_gl_move_mouse(int x, int y);


extern char const *__allegro_gl_get_format_description(GLint format);
extern int __allegro_gl_get_num_channels(GLenum format);
extern GLenum __allegro_gl_get_bitmap_type(BITMAP *bmp, int flags);
extern GLenum __allegro_gl_get_bitmap_color_format(BITMAP *bmp, int flags);
extern GLint __allegro_gl_get_texture_format_ex(BITMAP *bmp, int flags);
extern BITMAP *__allegro_gl_munge_bitmap(int flags, BITMAP *bmp, int x, int y,
                                      int w, int h, GLint *type, GLint *format);

extern int __allegro_gl_blit_operation;


/** \internal
 * Stores info about a glyph before rendering 
 */
typedef struct AGL_GLYPH {
	int glyph_num;
	int x, y, w, h;
	int offset_x, offset_y, offset_w, offset_h;
} AGL_GLYPH;


/*   <ofs_x>
     +--------------------+
     |                    |
     |    +-------+       |
     |    | glyph |       |
     |    +-------+       |
     |                    |
     +--------------------+
          <   w   ><ofs_w >
     <      polygon       >
*/


/** \internal
 * Part of the Allegro font vtable's data field.
 * Contains all the info needed to manage the 3D font.
 */
typedef struct FONT_AGL_DATA {
	int type;
	int start, end;
	int is_free_chunk;

	float scale;
	GLint format;

	void *data;
	AGL_GLYPH *glyph_coords;
	GLuint list_base;
	GLuint texture;

	struct FONT_AGL_DATA *next;

        int has_alpha;
} FONT_AGL_DATA;

extern struct FONT_VTABLE *font_vtable_agl;


/** \internal
 *  Force the creation of an alpha channel in the texture that will be generated
 *  by allegro_gl_make_texture_ex()
 */
#define AGL_TEXTURE_FORCE_ALPHA_INTERNAL 0x80000000

/** \internal
 *  Checks for texture validity only. Used by allegro_gl_check_texture() only.
 */
#define AGL_TEXTURE_CHECK_VALID_INTERNAL 0x40000000


#define AGL_OP_LOGIC_OP			0x0
#define AGL_OP_BLEND			0x1

#define AGL_H_FLIP		1	/* Flag to request horizontal flipping */
#define AGL_V_FLIP		2	/* Flag to request vertical flipping */
#define AGL_REGULAR_BMP	1	/* Must be set for bitmaps that are not sprites.
							   Otherwise the clipping routine will not test
							   if source_x and source_y are legal values */
#define AGL_NO_ROTATION	2	/* If not set the clipping routine is skipped
							   This is needed for pivot_scaled_x() in order
							   not to clip rotated bitmaps (in such a case
							   OpenGL will take care of it) */


#define AGL_LOG(level,str)

#ifdef DEBUGMODE
#ifdef LOGLEVEL

void __allegro_gl_log (int level, const char *str);
#undef AGL_LOG
#define AGL_LOG(level,str) __allegro_gl_log (level, str)


#endif
#endif


#define GET_ALLEGRO_VERSION() MAKE_VER(ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, \
                                       ALLEGRO_WIP_VERSION)
#define MAKE_VER(a, b, c) (((a) << 16) | ((b) << 8) | (c))


#endif

