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
 *      Some definitions for internal use by the Linux console code.
 *
 *      By George Foot.
 * 
 *      See readme.txt for copyright information.
 */

#ifndef __al_included_allegro5_aintlnx_h
#define __al_included_allegro5_aintlnx_h

#ifdef __cplusplus
extern "C" {
#endif


/**************************************/
/************ Driver lists ************/
/**************************************/

extern _AL_DRIVER_INFO _linux_gfx_driver_list[];
extern _AL_DRIVER_INFO _al_linux_keyboard_driver_list[];
extern _AL_DRIVER_INFO _al_linux_mouse_driver_list[];


/****************************************/
/************ Memory mapping ************/ /* (src/linux/lmemory.c) */
/****************************************/

/* struct MAPPED_MEMORY:  Used to describe a block of memory mapped 
 *  into our address space (in particular, the video memory).
 */
struct MAPPED_MEMORY {
	unsigned int base, size;      /* linear address and size of block */
	int perms;                    /* PROT_READ | PROT_WRITE, etc */
	void *data;                   /* pointer to block after mapping */
};

extern int __al_linux_have_ioperms;

int __al_linux_init_memory (void);
int __al_linux_shutdown_memory (void);
int __al_linux_map_memory (struct MAPPED_MEMORY *info);
int __al_linux_unmap_memory (struct MAPPED_MEMORY *info);


/******************************************/
/************ Console routines ************/ /* (src/linux/lconsole.c) */
/******************************************/

#define N_CRTC_REGS  24
#define N_ATC_REGS   21
#define N_GC_REGS    9
#define N_SEQ_REGS   5

#define MISC_REG_R       0x03CC
#define MISC_REG_W       0x03C2
#define ATC_REG_IW       0x03C0
#define ATC_REG_R        0x03C1
#define GC_REG_I         0x03CE
#define GC_REG_RW        0x03CF
#define SEQ_REG_I        0x03C4
#define SEQ_REG_RW       0x03C5
#define PEL_REG_IW       0x03C8
#define PEL_REG_IR       0x03C7
#define PEL_REG_D        0x03C9

#define _is1             0x03DA

#define ATC_DELAY        10 /* microseconds - for usleep() */

#define VGA_MEMORY_BASE  0xA0000
#define VGA_MEMORY_SIZE  0x10000
#define VGA_FONT_SIZE    0x02000

/* This structure is also used for video state saving/restoring, therefore it
 * contains fields that are used only when saving/restoring the text mode. */
typedef struct MODE_REGISTERS
{
   unsigned char crt[N_CRTC_REGS];
   unsigned char seq[N_SEQ_REGS];
   unsigned char atc[N_ATC_REGS];
   unsigned char gc[N_GC_REGS];
   unsigned char misc;
   unsigned char *ext;
   unsigned short ext_count;
   unsigned char *text_font1;
   unsigned char *text_font2;
   unsigned long flags;
   union {
      unsigned char vga[768];
   } palette;
} MODE_REGISTERS;

extern int __al_linux_vt;
extern int __al_linux_console_fd;
extern int __al_linux_prev_vt;
extern int __al_linux_got_text_message;
extern struct termios __al_linux_startup_termio;
extern struct termios __al_linux_work_termio;

int __al_linux_use_console (void);
int __al_linux_leave_console (void);

int __al_linux_console_graphics (void);
int __al_linux_console_text (void);

int __al_linux_wait_for_display (void);


/*************************************/
/************ VGA helpers ************/ /* (src/linux/lvgahelp.c) */
/*************************************/

int __al_linux_init_vga_helpers (void);
int __al_linux_shutdown_vga_helpers (void);

void __al_linux_screen_off (void);
void __al_linux_screen_on (void);

void __al_linux_clear_vram (void);

void __al_linux_set_vga_regs (MODE_REGISTERS *regs);
void __al_linux_get_vga_regs (MODE_REGISTERS *regs);

void __al_linux_save_gfx_mode (void);
void __al_linux_restore_gfx_mode (void);
void __al_linux_save_text_mode (void);
void __al_linux_restore_text_mode (void);

#define __just_a_moment() usleep(ATC_DELAY)


/**************************************/
/************ VT switching ************/ /* (src/linux/vtswitch.c) */
/**************************************/

/* signals for VT switching */
#define SIGRELVT        SIGUSR1
#define SIGACQVT        SIGUSR2

int __al_linux_init_vtswitch (void);
int __al_linux_done_vtswitch (void);

int __al_linux_set_display_switch_mode (int mode);
void __al_linux_display_switch_lock (int lock, int foreground);

extern volatile int __al_linux_switching_blocked;



#ifdef __cplusplus
}
#endif



/* Functions for querying the framebuffer, for the fbcon driver */
#if (defined ALLEGRO_LINUX_FBCON) && (!defined ALLEGRO_WITH_MODULES)
   extern int __al_linux_get_fb_color_depth(void);
   extern int __al_linux_get_fb_resolution(int *width, int *height);
#endif

#endif

