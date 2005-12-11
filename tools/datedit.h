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

#ifdef __cplusplus
   extern "C" {
#endif


#define DAT_INFO  DAT_ID('i','n','f','o')
#define DAT_ORIG  DAT_ID('O','R','I','G')
#define DAT_DATE  DAT_ID('D','A','T','E')
#define DAT_XPOS  DAT_ID('X','P','O','S')
#define DAT_YPOS  DAT_ID('Y','P','O','S')
#define DAT_XSIZ  DAT_ID('X','S','I','Z')
#define DAT_YSIZ  DAT_ID('Y','S','I','Z')
#define DAT_PACK  DAT_ID('P','A','C','K')
#define DAT_SORT  DAT_ID('S','O','R','T')
#define DAT_HNAM  DAT_ID('H','N','A','M')
#define DAT_HPRE  DAT_ID('H','P','R','E')
#define DAT_BACK  DAT_ID('B','A','C','K')
#define DAT_DITH  DAT_ID('D','I','T','H')
#define DAT_TRAN  DAT_ID('T','R','A','N')
#define DAT_XGRD  DAT_ID('X','G','R','D')
#define DAT_YGRD  DAT_ID('Y','G','R','D')
#define DAT_XCRP  DAT_ID('X','C','R','P')
#define DAT_YCRP  DAT_ID('Y','C','R','P')
#define DAT_RELF  DAT_ID('R','E','L','F')



typedef struct DATEDIT_OBJECT_INFO
{
   int type;
   char *desc;
   void (*get_desc)(AL_CONST DATAFILE *dat, char *s);
   void *(*makenew)(long *size);
   int (*save)(DATAFILE *dat, AL_CONST int *fixed_prop, int pack, int pack_kids, int strip, int sort, int verbose, int extra, PACKFILE *f);
   void (*plot)(AL_CONST DATAFILE *dat, int x, int y);
   int (*dclick)(DATAFILE *dat);
   void (*dat2s)(DATAFILE *dat, AL_CONST char *name, FILE *file, FILE *header);
} DATEDIT_OBJECT_INFO;



typedef struct DATEDIT_GRABBER_INFO
{
   int type;
   char *grab_ext;
   char *export_ext;
   DATAFILE *(*grab)(int type, AL_CONST char *filename, DATAFILE_PROPERTY **prop, int depth);  /* TODO: get rid of 'depth' */
   int (*exporter)(AL_CONST DATAFILE *dat, AL_CONST char *filename);
   char *prop_types;
} DATEDIT_GRABBER_INFO;



typedef struct DATEDIT_MENU_INFO
{
   MENU *menu;
   int (*query)(int popup);
   int flags;
   int key;
   char *prop_types;
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


void datedit_init(void);

void datedit_msg(AL_CONST char *fmt, ...);
void datedit_startmsg(AL_CONST char *fmt, ...);
void datedit_endmsg(AL_CONST char *fmt, ...);
void datedit_error(AL_CONST char *fmt, ...);
int datedit_ask(AL_CONST char *fmt, ...);
int datedit_select(AL_CONST char *(*list_getter)(int index, int *list_size), AL_CONST char *fmt, ...);

char *datedit_pretty_name(AL_CONST char *name, AL_CONST char *ext, int force_ext);
int datedit_clean_typename(AL_CONST char *type);
AL_CONST char *datedit_get_property(DATAFILE_PROPERTY **prop, int type);
int datedit_numprop(DATAFILE_PROPERTY **prop, int type);
void datedit_insert_property(DATAFILE_PROPERTY **prop, int type, AL_CONST char *value);
void datedit_set_property(DATAFILE *dat, int type, AL_CONST char *value);
void datedit_find_character(BITMAP *bmp, int *x, int *y, int *w, int *h);
DATAFILE *datedit_construct(int type, void *dat, long size, DATAFILE_PROPERTY **prop);
AL_CONST char *datedit_desc(AL_CONST DATAFILE *dat);
void datedit_sort_datafile(DATAFILE *dat);
void datedit_sort_properties(DATAFILE_PROPERTY *prop);
long datedit_asc2ftime(AL_CONST char *time);
AL_CONST char *datedit_ftime2asc(long time);
AL_CONST char *datedit_ftime2asc_int(long time);
AL_CONST char *datedit_grab_ext(int type);
AL_CONST char *datedit_export_ext(int type);



typedef struct DATEDIT_SAVE_DATAFILE_OPTIONS {
   int pack;
   int strip;
   int sort;
   int verbose;
   int write_msg;
   int backup;
   int relative;
} DATEDIT_SAVE_DATAFILE_OPTIONS;



DATAFILE *datedit_load_datafile(AL_CONST char *name, int compile_sprites, AL_CONST char *password);
int datedit_save_datafile(DATAFILE *dat, AL_CONST char *name, AL_CONST int *fixed_prop, AL_CONST DATEDIT_SAVE_DATAFILE_OPTIONS *options, AL_CONST char *password);
int datedit_save_header(AL_CONST DATAFILE *dat, AL_CONST char *name, AL_CONST char *headername, AL_CONST char *progname, AL_CONST char *prefix, int verbose);

int datedit_export(AL_CONST DATAFILE *dat, AL_CONST char *name);
void datedit_export_name(AL_CONST DATAFILE *dat, AL_CONST char *name, AL_CONST char *ext, char *buf);

DATAFILE *datedit_insert(DATAFILE *dat, DATAFILE **ret, AL_CONST char *name, int type, void *v, long size);
DATAFILE *datedit_delete(DATAFILE *dat, int i);

void datedit_swap(DATAFILE *dat, int i1, int i2);


typedef struct DATEDIT_GRAB_PARAMETERS {
   AL_CONST char *datafile;  /* absolute filename of the datafile              */
   AL_CONST char *filename;  /* absolute filename of the original file         */
   AL_CONST char *name;      /* name of the object                             */
   int type;                 /* type of the object                             */
   int x, y, w, h;           /* area to grab within the bitmap                 */
   int colordepth;           /* color depth to grab to                         */
   int relative;             /* whether to use relative filenames for DAT_ORIG */
} DATEDIT_GRAB_PARAMETERS;

DATAFILE *datedit_grabnew(DATAFILE *dat, AL_CONST DATEDIT_GRAB_PARAMETERS *params);
int datedit_grabreplace(DATAFILE *dat, AL_CONST DATEDIT_GRAB_PARAMETERS *params);
int datedit_grabupdate(DATAFILE *dat, DATEDIT_GRAB_PARAMETERS *params);
DATAFILE *datedit_grab(DATAFILE_PROPERTY *prop, AL_CONST DATEDIT_GRAB_PARAMETERS *params);

int datedit_update(DATAFILE *dat, AL_CONST char *datafile, int force, int verbose, int *changed);

extern void (*grabber_sel_palette)(PALETTE pal);
extern void (*grabber_select_property)(int type);
extern void (*grabber_get_grid_size)(int *x, int *y);
extern void (*grabber_rebuild_list)(void *old, int clear);
extern void (*grabber_get_selection_info)(DATAFILE **dat, DATAFILE ***parent);
extern int (*grabber_foreach_selection)(int (*proc)(DATAFILE *, int *, int), int *count, int *param, int param2);
extern DATAFILE *(*grabber_single_selection)(void);
extern void (*grabber_set_selection)(void *object);
extern void (*grabber_busy_mouse)(int busy);
extern void (*grabber_modified)(int modified);


#define FILENAME_LENGTH      1024
#define MAX_BYTES_PER_CHAR   6

extern char grabber_data_file[FILENAME_LENGTH];
extern BITMAP *grabber_graphic;
extern PALETTE grabber_palette;

#define GRABBER_GRAPHIC_ORIGIN_SIZE  256

extern char grabber_import_file[];
extern char grabber_graphic_origin[];
extern char grabber_graphic_date[];


#ifdef __cplusplus
   }
#endif

#endif
