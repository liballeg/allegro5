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
 *      Inline functions (Watcom style 386 asm).
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#if (!defined ALLEGRO_WATCOM) || (!defined ALLEGRO_I386)
   #error bad include
#endif



/* _default_ds:
 *  Return a copy of the current %ds selector.
 */
int _default_ds(void);

#pragma aux _default_ds =     \
   " mov eax, 0 "             \
   " mov ax, ds "             \
			      \
   value [eax];



/* _my_cs:
 *  Return a copy of the current %cs selector.
 */
int _my_cs(void);

#pragma aux _my_cs =          \
   " mov eax, 0 "             \
   " mov ax, cs "             \
			      \
   value [eax];



/* bmp_write_line/bmp_read_line:
 *  Bank switch functions.
 */
unsigned long _bmp_bank_switcher(BITMAP *bmp, int line, void *bank_switch);

#pragma aux _bmp_bank_switcher =    \
   " call ecx "                     \
				    \
   parm [edx] [eax] [ecx]           \
   value [eax];

#define bmp_write_line(bmp, line)   _bmp_bank_switcher(bmp, line, (void *)bmp->write_bank)
#define bmp_read_line(bmp, line)    _bmp_bank_switcher(bmp, line, (void *)bmp->read_bank)



/* bmp_unwrite_line:
 *  Terminate bank switch function.
 */
void _bmp_unbank_switcher(BITMAP *bmp, void *bank_unswitcher);

#pragma aux _bmp_unbank_switcher =  \
   " call ecx "                     \
				    \
   parm [edx] [ecx];

#define bmp_unwrite_line(bmp)       _bmp_unbank_switcher(bmp, (void *)bmp->vtable->unwrite_bank)



/* _set_errno_erange:
 *  Watcom's asm syntax doesn't provide any nice way to do this inline...
 */
AL_INLINE(void, _set_errno_erange, (void),
{
   *allegro_errno = ERANGE;
})



/* fadd:
 *  Fixed point (16.16) addition.
 */
fixed fadd(fixed x, fixed y);

#pragma aux fadd =               \
   "  add eax, edx "             \
   "  jno Out1 "                 \
   "  call _set_errno_erange "   \
   "  mov eax, 0x7FFFFFFF "      \
   "  cmp edx, 0 "               \
   "  jg Out1 "                  \
   "  neg eax "                  \
   " Out1: "                     \
				 \
   parm [eax] [edx]              \
   value [eax];



/* fsub:
 *  Fixed point (16.16) subtraction.
 */
fixed fsub(fixed x, fixed y);

#pragma aux fsub =               \
   "  sub eax, edx "             \
   "  jno Out1 "                 \
   "  call _set_errno_erange "   \
   "  mov eax, 0x7FFFFFFF "      \
   "  cmp edx, 0 "               \
   "  jl Out1 "                  \
   "  neg eax "                  \
   " Out1: "                     \
				 \
   parm [eax] [edx]              \
   value [eax];



/* fmul:
 *  Fixed point (16.16) multiplication.
 */
fixed fmul(fixed x, fixed y);

#pragma aux fmul =               \
   "  mov eax, ebx "             \
   "  imul ecx "                 \
   "  shrd eax, edx, 16 "        \
   "  sar edx, 15 "              \
   "  jz Out2 "                  \
   "  cmp edx, -1 "              \
   "  jz Out2 "                  \
   "  call _set_errno_erange "   \
   "  mov eax, 0x7FFFFFFF "      \
   "  cmp ebx, 0 "               \
   "  jge Out1 "                 \
   "  neg eax "                  \
   " Out1: "                     \
   "  cmp ecx, 0 "               \
   "  jge Out2 "                 \
   "  neg eax "                  \
   " Out2: "                     \
				 \
   parm [ebx] [ecx]              \
   modify [edx]                  \
   value [eax];



/* fdiv:
 *  Fixed point (16.16) division.
 */
fixed fdiv(fixed x, fixed y);

#pragma aux fdiv =               \
   "  xor ebx, ebx "             \
   "  or eax, eax "              \
   "  jns Out1 "                 \
   "  neg eax "                  \
   "  inc ebx "                  \
   " Out1: "                     \
   "  or ecx, ecx "              \
   "  jns Out2 "                 \
   "  neg ecx "                  \
   "  inc ebx "                  \
   " Out2: "                     \
   "  mov edx, eax "             \
   "  shr edx, 0x10 "            \
   "  shl eax, 0x10 "            \
   "  cmp edx, ecx "             \
   "  jae Out3 "                 \
   "  div ecx "                  \
   "  or eax, eax "              \
   "  jns Out4 "                 \
   " Out3: "                     \
   "  call _set_errno_erange "   \
   "  mov eax, 0x7FFFFFFF "      \
   " Out4: "                     \
   "  test ebx, 1 "              \
   "  je Out5 "                  \
   "  neg eax "                  \
   " Out5: "                     \
				 \
   parm [eax] [ecx]              \
   modify [ebx edx]              \
   value [eax];

