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
 *      BeOS gfx line switchers.
 *
 *      By Jason Wilkins
 *
 *	    Windowed mode support added by Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include "src/i386/asmdefs.inc"



.text


/* _be_gfx_fullscreen_read_write_bank_asm:
 *   eax = line number
 *   edx = bitmap
 */
FUNC(_be_gfx_fullscreen_read_write_bank_asm)
   testl $BMP_ID_LOCKED, BMP_ID(%edx)
   jnz be_gfx_fullscreen_already_acquired

   pushal
   call *GLOBL(_be_sync_func)
   popal

   orl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
   orl $BMP_ID_LOCKED, BMP_ID(%edx)

be_gfx_fullscreen_already_acquired:
   movl BMP_LINE(%edx, %eax, 4), %eax
   ret



/* _be_gfx_fullscreen_unwrite_bank_asm:
 *   edx = bitmap
 */
FUNC(_be_gfx_fullscreen_unwrite_bank_asm)
   testl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
   jz be_gfx_fullscreen_no_release

   andl $~BMP_ID_AUTOLOCK, BMP_ID(%edx)
   andl $~BMP_ID_LOCKED, BMP_ID(%edx)

be_gfx_fullscreen_no_release:
   ret



/* _be_gfx_windowed_read_write_bank_asm:
 *   eax = line number
 *   edx = bitmap
 */
FUNC(_be_gfx_windowed_read_write_bank_asm)
   testl $BMP_ID_LOCKED, BMP_ID(%edx)
   jnz be_gfx_windowed_already_acquired
   
   orl $BMP_ID_LOCKED, BMP_ID(%edx)
   orl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
   
be_gfx_windowed_already_acquired:
   pushl %ecx
   pushl %eax
   movl GLOBL(_be_dirty_lines), %ecx
   addl BMP_YOFFSET(%edx), %eax
   movl $1, (%ecx, %eax, 4)
   popl %eax
   popl %ecx
   movl BMP_LINE(%edx, %eax, 4), %eax
   ret



/* _be_gfx_windowed_unwrite_bank_asm:
 *   edx = bitmap
 */
FUNC(_be_gfx_windowed_unwrite_bank_asm)
   testl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
   jz be_gfx_windowed_no_release
   
   pushl %eax
   pushl %ecx
   pushl %edx
   pushl GLOBL(_be_window_lock)
   call GLOBL(release_sem)
   addl $4, %esp
   popl %edx
   popl %ecx
   popl %eax

be_gfx_windowed_no_release:
   ret 

