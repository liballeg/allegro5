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
 *      Datafile editing functions, for use by the datafile tools.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DATEDIT_H
#define DATEDIT_H


#define DAT_INFO  DAT_ID('i','n','f','o')
#define DAT_ORIG  DAT_ID('O','R','I','G')
#define DAT_DATE  DAT_ID('D','A','T','E')
#define DAT_XPOS  DAT_ID('X','P','O','S')
#define DAT_YPOS  DAT_ID('Y','P','O','S')
#define DAT_XSIZ  DAT_ID('X','S','I','Z')
#define DAT_YSIZ  DAT_ID('Y','S','I','Z')
#define DAT_PACK  DAT_ID('P','A','C','K')
#define DAT_HNAM  DAT_ID('H','N','A','M')
#define DAT_HPRE  DAT_ID('H','P','R','E')
#define DAT_BACK  DAT_ID('B','A','C','K')
#define DAT_DITH  DAT_ID('D','I','T','H')
#define DAT_XGRD  DAT_ID('X','G','R','D')
#define DAT_YGRD  DAT_ID('Y','G','R','D')



typedef struct DATEDIT_OBJECT_INFO
{
   int type;
   char *desc;
   void (*get_desc)(DATAFILE *dat, char *s);
   void *(*makenew)(long *size);
   void (*save)(DATAFILE *dat, int packed, int packkids, int strip, int verbose, int extra, PACKFILE *f);
   void (*plot)(DATAFILE *dat, int x, int y);
   int (*dclick)(DATAFILE *dat);
   void (*dat2s)(DATAFILE *dat, char *name, FILE *file, FILE *header);
} DATEDIT_OBJECT_INFO;



typedef struct DATEDIT_GRABBER_INFO
{
   int type;
   char *grab_ext;
   char *export_ext;
   void *(*grab)(char *filename, long *size, int x, int y, int w, int h, int depth);
   int (*export)(DATAFILE *dat, char *filename);
} DATEDIT_GRABBER_INFO;



typedef struct DATEDIT_MENU_INFO
{
   MENU *menu;
   int (*query)(int popup);
   int flags;
   int key;
} DATEDIT_MENU_INFO;



#define DATEDIT_MENU_FILE        1
#define DATEDIT_MENU_OBJECT      2
#define DATEDIT_MENU_HELP        4
#define DATEDIT_MENU_POPUP       8
#define DATEDIT_MENU_TOP         16


extern DATEDIT_OBJECT_INFO *datedit_object_info[];
extern DATEDIT_GRABBER_INFO *datedit_grabber_info[];
extern DATEDIT_MENU_INFO *datedit_menu_info[];


void datedit_register_object(DATEDIT_OBJECT_INFO *info);
void datedit_register_grabber(DATEDIT_GRABBER_INFO *info);
void datedit_register_menu(DATEDIT_MENU_INFO *info);


extern PALETTE datedit_current_palette;
extern PALETTE datedit_last_read_pal;
extern DATAFILE datedit_info;


void datedit_init();

void datedit_msg(char *fmt, ...);
void datedit_startmsg(char *fmt, ...);
void datedit_endmsg(char *fmt, ...);
void datedit_error(char *fmt, ...);
int datedit_ask(char *fmt, ...);

char *datedit_pretty_name(char *name, char *ext, int force_ext);
int datedit_clean_typename(char *type);
void datedit_set_property(DATAFILE *dat, int type, char *value);
void datedit_find_character(BITMAP *bmp, int *x, int *y, int *w, int *h);
char *datedit_desc(DATAFILE *dat);
void datedit_sort_datafile(DATAFILE *dat);
void datedit_sort_properties(DATAFILE_PROPERTY *prop);
long datedit_asc2ftime(char *time);
char *datedit_ftime2asc(long time);
char *datedit_ftime2asc_int(long time);
int datedit_numprop(DATAFILE *dat, int type);
char *datedit_grab_ext(int type);
char *datedit_export_ext(int type);

DATAFILE *datedit_load_datafile(char *name, int compile_sprites, char *password);
int datedit_save_datafile(DATAFILE *dat, char *name, int strip, int pack, int verbose, int write_msg, int backup, char *password);
int datedit_save_header(DATAFILE *dat, char *name, char *headername, char *progname, char *prefix, int verbose);

void datedit_export_name(DATAFILE *dat, char *name, char *ext, char *buf);
int datedit_export(DATAFILE *dat, char *name);
DATAFILE *datedit_delete(DATAFILE *dat, int i);
DATAFILE *datedit_grab(char *filename, char *name, int type, int x, int y, int w, int h, int colordepth);
int datedit_grabreplace(DATAFILE *dat, char *filename, char *name, char *type, int colordepth, int x, int y, int w, int h);
int datedit_grabupdate(DATAFILE *dat, char *filename, int x, int y, int w, int h);
DATAFILE *datedit_grabnew(DATAFILE *dat, char *filename, char *name, char *type, int colordepth, int x, int y, int w, int h);
DATAFILE *datedit_insert(DATAFILE *dat, DATAFILE **ret, char *name, int type, void *v, long size);
int datedit_update(DATAFILE *dat, int verbose, int *changed);
int datedit_force_update(DATAFILE *dat, int verbose, int *changed);

extern void (*grabber_sel_palette)(PALETTE pal);
extern void (*grabber_select_property)(int type);
extern void (*grabber_get_grid_size)(int *x, int *y);
extern void (*grabber_rebuild_list)(void *old, int clear);
extern void (*grabber_get_selection_info)(DATAFILE **dat, DATAFILE ***parent);
extern int (*grabber_foreach_selection)(int (*proc)(DATAFILE *, int *, int), int *count, int *param, int param2);
extern DATAFILE *(*grabber_single_selection)();
extern void (*grabber_set_selection)(void *object);
extern void (*grabber_busy_mouse)(int busy);

extern BITMAP *grabber_graphic;
extern PALETTE grabber_palette;

extern char grabber_import_file[];
extern char grabber_graphic_origin[];
extern char grabber_graphic_date[];


#endif
