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
 *      BeOS Stuff
 *
 *      By Jason Wilkins
 *
 *      See readme.txt for copyright information.
 */


#include "src/i386/asmdefs.inc"



.text

/* _be_gfx_fullscreen_read_write_bank:
 *   eax = line number
 *   edx = bitmap
 */
FUNC(_be_gfx_fullscreen_read_write_bank)
   testl $BMP_ID_LOCKED, BMP_ID(%edx)
   jnz be_gfx_fullscreen_already_acquired

   pushl %eax
   pushl %ecx
   pushl %edx
   pushl GLOBL(fullscreen_lock)
   call GLOBL(acquire_sem)
   addl $4, %esp
   popl %edx
   popl %ecx
   popl %eax 

   orl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
   orl $BMP_ID_LOCKED, BMP_ID(%edx)

be_gfx_fullscreen_already_acquired:
   movl BMP_LINE(%edx, %eax, 4), %eax
   ret



/* _be_gfx_fullscreen_unwrite_bank:
 *   edx = bitmap
 */
FUNC(_be_gfx_fullscreen_unwrite_bank)
   testl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
   jz be_gfx_fullscreen_no_release

   andl $~BMP_ID_AUTOLOCK, BMP_ID(%edx)
   andl $~BMP_ID_LOCKED, BMP_ID(%edx)

   pushl %eax
   pushl %ecx
   pushl %edx
   pushl GLOBL(fullscreen_lock)
   call GLOBL(release_sem)
   addl $4, %esp
   popl %edx
   popl %ecx
   popl %eax

be_gfx_fullscreen_no_release:
   ret
