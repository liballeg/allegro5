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
 *      QNX asm line switcher routines.
 *
 *      By Angelo Mottola.
 *
 *      Window updating system derived from Win32 version by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "../i386/asmdefs.inc"

.text


#ifndef ALLEGRO_NO_ASM

/* Use only with ASM calling convention.  */

/* ph_write_line_asm:
 *  edx = bitmap
 *  eax = line
 */
FUNC (ph_write_line_asm)
   testl $BMP_ID_LOCKED, BMP_ID(%edx)
   jnz ph_already_locked

   /* set lock and autolock flags */
   orl $(BMP_ID_LOCKED | BMP_ID_AUTOLOCK), BMP_ID(%edx)
   
   pushal
   call GLOBL(PgWaitHWIdle)
   popal

ph_already_locked:
   /* get pointer to the video memory */
   movl BMP_LINE(%edx,%eax,4), %eax
   
   ret



/* ph_unwrite_line_asm:
 *  edx = bmp
 */
FUNC (ph_unwrite_line_asm)
   /* only unlock bitmaps that were autolocked */
   testl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
   jz dont_release_ph
      
   /* clear lock and autolock flags */
   andl $~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK), BMP_ID(%edx)
   
dont_release_ph:

   ret
   
   
   
/* ph_write_line_win_asm:
 *  edx = bitmap
 *  eax = line
 */
FUNC (ph_write_line_win_asm)
   pushal
   
   movl GLOBL(ph_dirty_lines), %ebx
   addl BMP_YOFFSET(%edx), %ebx
   movb $1, (%ebx,%eax)   /* ph_dirty_lines[line] = 1; (line has changed) */
   
   /* check whether is locked already */
   testl $BMP_ID_LOCKED, BMP_ID(%edx)
   jnz ph_already_locked_win

   /* lock the surface */
   pushl %edx
   call *GLOBL(ptr_ph_acquire_win) 
   popl %edx

   /* set the autolock flag */
   orl $BMP_ID_AUTOLOCK, BMP_ID(%edx)

ph_already_locked_win:
   popal
   
   /* get pointer to the video memory */
   movl BMP_LINE(%edx,%eax,4), %eax
      
   ret



/* ph_unwrite_line_win_asm:
 *  edx = bmp
 */
FUNC (ph_unwrite_line_win_asm)
   /* only unlock bitmaps that were autolocked */
   testl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
   jz dont_release_ph_win

   /* unlock surface */
   pushal
   pushl %edx
   call *GLOBL(ptr_ph_release_win)
   popl %edx
   popal
      
   /* clear the autolock flag */
   andl $~BMP_ID_AUTOLOCK, BMP_ID(%edx)
   
dont_release_ph_win:

   ret


#endif
