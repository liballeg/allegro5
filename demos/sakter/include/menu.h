#ifndef		__DEMO_MENU_H__
#define		__DEMO_MENU_H__

#include <allegro.h>
#include "global.h"

#define		DEMO_MENU_CONTINUE		1000
#define		DEMO_MENU_BACK			1001
#define		DEMO_MENU_LOCK			1002

#define		DEMO_MENU_SELECTABLE	1
#define		DEMO_MENU_SELECTED		2
#define		DEMO_MENU_EXIT			4
#define		DEMO_MENU_EXTRA			8

#define		DEMO_MENU_MSG_INIT		0
#define		DEMO_MENU_MSG_DRAW		1
#define		DEMO_MENU_MSG_KEY		2
#define		DEMO_MENU_MSG_WIDTH		3
#define		DEMO_MENU_MSG_HEIGHT	4
#define		DEMO_MENU_MSG_TICK		5

extern BITMAP *demo_menu_canvas;

typedef struct DEMO_MENU DEMO_MENU;

struct DEMO_MENU {
   int (*proc) (DEMO_MENU *, int, int);
   char *name;
   int flags;
   int extra;
   void **data;
   void (*on_activate) (DEMO_MENU *);
};

void init_demo_menu(DEMO_MENU * menu, int PlayMusic);
int update_demo_menu(DEMO_MENU * menu);
void draw_demo_menu(BITMAP * canvas, DEMO_MENU * menu);

int demo_text_proc(DEMO_MENU * item, int msg, int key);
int demo_edit_proc(DEMO_MENU * item, int msg, int key);
int demo_button_proc(DEMO_MENU * item, int msg, int key);
int demo_choice_proc(DEMO_MENU * item, int msg, int key);
int demo_key_proc(DEMO_MENU * item, int msg, int key);
int demo_color_proc(DEMO_MENU * item, int msg, int extra);
int demo_separator_proc(DEMO_MENU * item, int msg, int extra);

#endif				/* __DEMO_MENU_H__ */
