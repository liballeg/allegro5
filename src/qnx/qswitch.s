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
 *      See readme.txt for copyright information.
 */


#include "../i386/asmdefs.inc"

.text


#ifndef ALLEGRO_NO_ASM
	/* Use only with ASM calling convention.  */

FUNC (ph_write_line_asm)

	pushl %ecx
	pushl %eax
	pushl %edx
	call GLOBL(ph_write_line)
	popl %edx
	popl %ecx
	popl %ecx
	ret


#endif

