#ifndef __A4_AUX_H__
#define __A4_AUX_H__

#ifndef TRUE
   #define TRUE         -1
   #define FALSE        0
#endif

#undef MIN
#undef MAX
#undef MID

#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))
#define CLAMP(x,y,z) MAX(x,MIN(z,y))

#undef ABS
#define ABS(x)       (((x) >= 0) ? (x) : (-(x)))

#undef SGN
#define SGN(x)       (((x) >= 0) ? 1 : -1)


typedef struct MATRIX_f          /* transformation matrix (floating point) */
{
   float v[3][3];                /* scaling and rotation */
   float t[3];                   /* translation */
} MATRIX_f;


/* global variables */
extern int key[ALLEGRO_KEY_MAX];
extern int joy_left;
extern int joy_right;
extern int joy_b1;
extern struct ALLEGRO_DISPLAY *screen;
extern struct ALLEGRO_FONT *font;
extern struct ALLEGRO_FONT *font_video;


typedef struct DATAFILE DATAFILE;
struct DATAFILE {
    void* dat;
};
typedef ALLEGRO_BITMAP RLE_SPRITE;
typedef ALLEGRO_BITMAP BITMAP;
typedef ALLEGRO_COLOR RGB;
typedef ALLEGRO_FILE PACKFILE;

#define allegro_init al_init
#define install_keyboard al_install_keyboard
#define install_mouse al_install_mouse
#define allegro_message printf
#define allegro_error ""
#define uisspace(x) ((x) == ' ' || (x) == '\n')
#define text_length(f, x) al_get_text_width(f, x)
#define text_height(f) al_get_font_line_height(f)
#define itofix al_itofix
#define fixtoi al_fixtoi
#define fixdiv al_fixdiv
#define fixsin al_fixsin
#define fixcos al_fixcos
#define fixsqrt al_fixsqrt
#define fixmul al_fixmul
#define stricmp strcmp
#define create_bitmap al_create_bitmap

const char *get_config_string(const ALLEGRO_CONFIG *cfg, const char *section, const char *name, const char *def);
int get_config_int(const ALLEGRO_CONFIG *cfg, const char *section, const char *name, int def);
void set_config_int(ALLEGRO_CONFIG *cfg, const char *section, const char *name, int val);

void init_input(void);
void shutdown_input(void);
void poll_input(void);
void poll_input_wait(void);
int keypressed(void);
int readkey(void);
void clear_keybuf(void);

ALLEGRO_BITMAP *create_memory_bitmap(int w, int h);
ALLEGRO_BITMAP *replace_bitmap(ALLEGRO_BITMAP *bmp);
void stretch_blit(BITMAP *bitmap, int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
void blit(BITMAP *bitmap, int x1, int y1, int x2, int y2, int w, int h);
void draw_sprite(BITMAP *bitmap, int x, int y);
void rotate_sprite(BITMAP *bitmap, int x, int y, int angle);
void solid_mode(void);
ALLEGRO_COLOR makecol(int r, int g, int b);
void hline(int x1, int y, int x2, ALLEGRO_COLOR c);
void vline(int x, int y1, int y2, ALLEGRO_COLOR c);
void line(int x1, int y1, int x2, int y2, ALLEGRO_COLOR color);
void rectfill(int x1, int y1, int x2, int y2, ALLEGRO_COLOR color);
void circle(int x, int y, int radius, ALLEGRO_COLOR color);
void circlefill(int x, int y, int radius, ALLEGRO_COLOR color);
void stretch_sprite(ALLEGRO_BITMAP *bmp, ALLEGRO_BITMAP *sprite, int x, int y, int w, int h);
void polygon(int vertices, const int *points, ALLEGRO_COLOR color);
void textout(struct ALLEGRO_FONT const *font, const char *s, int x, int y, ALLEGRO_COLOR c);
void textout_centre(struct ALLEGRO_FONT const *font, const char *s, int x, int y, ALLEGRO_COLOR c);
void textprintf(struct ALLEGRO_FONT const *f, int x, int y, ALLEGRO_COLOR color, const char *fmt, ...);

void get_scaling_matrix_f(MATRIX_f *m, float x, float y, float z);
void get_z_rotate_matrix_f(MATRIX_f *m, float r);
void qtranslate_matrix_f(MATRIX_f *m, float x, float y, float z);
void matrix_mul_f(const MATRIX_f *m1, const MATRIX_f *m2, MATRIX_f *out);
void apply_matrix_f(const MATRIX_f *m, float x, float y, float z, float *xout, float *yout, float *zout);

void start_retrace_count(void);
void stop_retrace_count(void);
int64_t retrace_count(void);
void rest(unsigned int time);
void install_timer(void);

struct ALLEGRO_SAMPLE *create_sample_u8(int freq, int len);
void play_sample(struct ALLEGRO_SAMPLE *spl, int vol, int pan, int freq, int loop);
void play_midi(char const *filename, int loop);
void stop_midi(void);
int install_sound(void);

int AL_RAND(void);


#endif   /* __A4_AUX_H__ */
