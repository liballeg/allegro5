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
 *      Asm functions for DirectDraw bitmap locking.
 *
 *      By Isaac Cruz.
 *
 *      See readme.txt for copyright information.
 */

#include "..\i386\asmdefs.inc"


.text


/* gfx_directx_write_bank:
 *  edx = bitmap
 *  eax = line
 */
FUNC (gfx_directx_write_bank)

      /* check whether is is locked already */
      testl $BMP_ID_LOCKED, BMP_ID(%edx)
      jnz Locked

      /* lock the surface */
      pushal
      pushl %edx
      movl GLOBL(ptr_gfx_directx_autolock), %ecx
      call %ecx
      popl %edx
      popal

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
      jz NoUnlock

      /* unlock surface */
      pushal
      pushl %edx
      movl GLOBL(ptr_gfx_directx_unlock), %ecx
      call %ecx
      popl %edx
      popal

      /* clear the autolock flag */
      andl $~BMP_ID_AUTOLOCK, BMP_ID(%edx)

   NoUnlock:
      ret
      



/* gfx_directx_write_bank_win:
 *  edx = bitmap
 *  eax = line
 */
FUNC (gfx_directx_write_bank_win)
      
      pushal
      movl GLOBL(dirty_lines), %ebx
      movl $1, (%ebx,%eax,4)   /* dirty_lines[line] = 1; (line has changed) */
      
      /* check whether is is locked already */
      testl $BMP_ID_LOCKED, BMP_ID(%edx)
      jnz Locked_win

      /* lock the surface */
      pushl %edx
      movl GLOBL(ptr_gfx_directx_autolock), %ecx
      call %ecx
      popl %edx

   Locked_win:
      popal
      /* get pointer to the video memory */
      movl BMP_LINE(%edx,%eax,4), %eax
      
      ret


#define RECT_LEFT   (%esp)
#define RECT_TOP    4(%esp)
#define RECT_RIGHT  8(%esp)
#define RECT_BOTTOM 12(%esp)



/* gfx_directx_unwrite_bank_win:
 *  edx = bmp
 */
FUNC (gfx_directx_unwrite_bank_win)

      /* only unlock bitmaps that were autolocked */
      testl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
      jz NoUnlock_win

      /* unlock surface */
      pushal
      pushl %edx
      movl GLOBL(ptr_gfx_directx_unlock), %ecx
      call %ecx
      popl %edx
      
      /* clear the autolock flag */
      andl $~BMP_ID_AUTOLOCK, BMP_ID(%edx)

      subl $16, %esp  /* allocate a RECT structure */
      movl $0, RECT_LEFT
      movl GLOBL(pseudo_screen), %eax
      movl (%eax), %ecx       /* ecx = pseudo_screen->w */
      movl %ecx, RECT_RIGHT
      movl GLOBL(dirty_lines), %ebx    /* ebx = dirty_lines */
      movl BMP_H(%eax), %edi           /* edi = pseudo_screen->h */
      movl GLOBL(update_window), %esi  /* esi = update_window */
      
      _align_
   next_line:     /* update dirty lines */
      decl %edi
      movl (%ebx,%edi,4), %eax  /* eax = dirty_lines[edi] */
      testl %eax, %eax   /* is dirty? */
      jz test_end
      movl %edi, RECT_TOP
      leal 1(%edi), %eax
      movl %eax, RECT_BOTTOM
      leal RECT_LEFT, %eax
      pushl %eax
      call %esi                /* update_window(&rect) */
      addl $4, %esp
      movl $0, (%ebx,%edi,4)   /* dirty_lines[edi] = 0 */
   test_end:   
      testl %edi, %edi   /* last line? */
      jnz next_line
      
      addl $16, %esp
      popal

   NoUnlock_win:
      ret
      

