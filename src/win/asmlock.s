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
 *      Asm functions for Windows bitmap locking.
 *
 *      By Isaac Cruz.
 *
 *      Improved dirty rectangles mechanism by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "src/i386/asmdefs.inc"


.text


/* gfx_directx_write_bank:
 *  edx = bitmap
 *  eax = line
 */
FUNC (gfx_directx_write_bank)

      /* check whether bitmap is already locked */
      testl $BMP_ID_LOCKED, BMP_ID(%edx)
      jnz Locked

      /* lock the surface */
      pushl %ecx
      pushl %eax
      pushl %edx
      call *GLOBL(ptr_gfx_directx_autolock)
      popl %edx
      popl %eax
      popl %ecx

   Locked:
      /* get pointer to the video memory */
      movl BMP_LINE(%edx,%eax,4), %eax

      ret




/* gfx_directx_unwrite_bank:
 *  edx = bmp
 */
FUNC (gfx_directx_unwrite_bank)

      /* only unlock bitmaps that were autolocked */
      testl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
      jz No_unlock

      /* unlock surface */
      pushl %ecx
      pushl %eax
      pushl %edx
      call *GLOBL(ptr_gfx_directx_unlock)
      popl %edx
      popl %eax
      popl %ecx

      /* clear the autolock flag */
      andl $~BMP_ID_AUTOLOCK, BMP_ID(%edx)

   No_unlock:
      ret




/* gfx_directx_write_bank_win:
 *  edx = bitmap
 *  eax = line
 */
FUNC (gfx_directx_write_bank_win)
      pushl %ecx

      /* clobber the line */
      movl GLOBL(wd_dirty_lines), %ecx
      addl BMP_YOFFSET(%edx), %ecx
      movb $1, (%ecx,%eax)   /* wd_dirty_lines[line] = 1; (line has changed) */

      /* check whether bitmap is already locked */
      testl $BMP_ID_LOCKED, BMP_ID(%edx)
      jnz Locked_win

      /* lock the surface */
      pushl %eax
      pushl %edx
      call *GLOBL(ptr_gfx_directx_autolock)
      popl %edx
      popl %eax

   Locked_win:
      popl %ecx

      /* get pointer to the video memory */
      movl BMP_LINE(%edx,%eax,4), %eax

      ret



/* gfx_directx_unwrite_bank_win:
 *  edx = bmp
 */
FUNC (gfx_directx_unwrite_bank_win)

      /* only unlock bitmaps that were autolocked */
      testl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
      jz No_unlock_win

      pushl %ecx
      pushl %eax

      /* unlock surface */
      pushl %edx
      call *GLOBL(ptr_gfx_directx_unlock)
      popl %edx

      /* clear the autolock flag */
      andl $~BMP_ID_AUTOLOCK, BMP_ID(%edx)

      /* update dirty lines: this is safe because autolocking
       * is guaranteed to be the only level of locking.
       */
      movl GLOBL(pseudo_screen), %edx
      call update_dirty_lines

      popl %eax
      popl %ecx

   No_unlock_win:
      ret


/* gfx_directx_unlock_win:
 *  arg1 = bmp
 */
FUNC(gfx_directx_unlock_win)

      /* unlock surface */
      movl 4(%esp), %edx
      pushl %edx 
      call *GLOBL(ptr_gfx_directx_unlock)
      popl %edx

      /* pseudo_screen may still be locked in case of nested locking */
      movl GLOBL(pseudo_screen), %edx
      testl $BMP_ID_LOCKED, BMP_ID(%edx)
      jnz No_update_win

      /* ok, we can safely update */
      call update_dirty_lines

   No_update_win:
      ret



/* update_dirty_lines
 *  edx = pseudo_screen
 *  clobbers: %eax, %ecx
 */

#define RECT_LEFT   (%esp)
#define RECT_TOP    4(%esp)
#define RECT_RIGHT  8(%esp)
#define RECT_BOTTOM 12(%esp)

update_dirty_lines:
      pushl %ebx
      pushl %esi
      pushl %edi
      subl $16, %esp  /* allocate a RECT structure */

      movl $0, RECT_LEFT
      movl BMP_W(%edx), %ecx           /* ecx = pseudo_screen->w */
      movl %ecx, RECT_RIGHT
      movl GLOBL(wd_dirty_lines), %ebx /* ebx = wd_dirty_lines   */
      movl BMP_H(%edx), %esi           /* esi = pseudo_screen->h */
      movl $0, %edi  

   _align_
   next_line:
      movb (%ebx,%edi), %al  /* al = wd_dirty_lines[edi] */
      testb %al, %al         /* is dirty? */
      jz test_end            /* no ! */

      movl %edi, RECT_TOP
   _align_
   loop_dirty_lines:
      movb $0, (%ebx,%edi)   /* wd_dirty_lines[edi] = 0  */
      incl %edi
      movb (%ebx,%edi), %al  /* al = wd_dirty_lines[edi] */
      testb %al, %al         /* is still dirty? */
      jnz loop_dirty_lines   /* yes ! */

      movl %edi, RECT_BOTTOM
      leal RECT_LEFT, %eax
      pushl %eax
      call *GLOBL(update_window) 
      popl %eax

   _align_
   test_end:   
      incl %edi
      cmpl %edi, %esi  /* last line? */
      jge next_line    /* no ! */

      addl $16, %esp
      popl %edi
      popl %esi
      popl %ebx
      ret



/* gfx_gdi_write_bank:
 *  edx = bitmap
 *  eax = line
 */
FUNC (gfx_gdi_write_bank)
      pushl %ecx

      /* clobber the line */
      movl GLOBL(gdi_dirty_lines), %ecx
      addl BMP_YOFFSET(%edx), %ecx
      movb $1, (%ecx,%eax)   /* gdi_dirty_lines[line] = 1; (line has changed) */

      /* check whether bitmap is already locked */
      testl $BMP_ID_LOCKED, BMP_ID(%edx)
      jnz Locked_gdi

      /* lock the surface */
      pushl %eax
      pushl %edx
      call *GLOBL(ptr_gfx_gdi_autolock) 
      popl %edx
      popl %eax

   Locked_gdi:
      popl %ecx

      /* get pointer to the video memory */
      movl BMP_LINE(%edx,%eax,4), %eax

      ret



/* gfx_gdi_unwrite_bank:
 *  edx = bmp
 */
FUNC (gfx_gdi_unwrite_bank)

      /* only unlock bitmaps that were autolocked */
      testl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
      jz No_unlock_gdi

      /* unlock surface */
      pushl %ecx
      pushl %eax
      pushl %edx
      call *GLOBL(ptr_gfx_gdi_unlock) 
      popl %edx
      popl %eax
      popl %ecx

      /* clear the autolock flag */
      andl $~BMP_ID_AUTOLOCK, BMP_ID(%edx)

   No_unlock_gdi:
      ret
