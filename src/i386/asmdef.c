/* 
 *    This a little on the complex side, but I couldn't think of any 
 *    other way to do it. I was getting fed up with having to rewrite 
 *    my asm code every time I altered the layout of a C struct, but I 
 *    couldn't figure out any way to get the asm stuff to read and 
 *    understand the C headers. So I made this program. It includes 
 *    allegro.h so it knows about everything the C code uses, and when 
 *    run it spews out a bunch of #defines containing information about 
 *    structure sizes which the asm code can refer to.
 */


#define USE_CONSOLE

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "allegro.h"
#include "allegro/aintern.h"

#ifdef ALLEGRO_DOS
   #include "allegro/aintdos.h"
#endif

#ifdef ALLEGRO_MMX_HEADER
   #include ALLEGRO_MMX_HEADER
#endif



int main(int argc, char *argv[])
{
   FILE *f;
   int x, y;

   if (argc < 2) {
      fprintf(stderr, "Usage: %s <output file>\n", argv[0]);
      return 1;
   }

   printf("writing structure offsets into %s...\n", argv[1]);

   f = fopen(argv[1], "w");
   if (f == 0) {
      fprintf(stderr, "%s: can not open file %s\n", argv[0], argv[1]);
      return 1;
   }

   fprintf(f, "/* Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR " */\n");
   fprintf(f, "/* automatically generated structure offsets for use by asm code */\n\n");

   #ifdef ALLEGRO_DJGPP
      fprintf(f, "#ifndef ALLEGRO_DJGPP\n");
      fprintf(f, "#define ALLEGRO_DJGPP\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   #ifdef ALLEGRO_WATCOM
      fprintf(f, "#ifndef ALLEGRO_WATCOM\n");
      fprintf(f, "#define ALLEGRO_WATCOM\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   #ifdef ALLEGRO_DOS
      fprintf(f, "#ifndef ALLEGRO_DOS\n");
      fprintf(f, "#define ALLEGRO_DOS\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   #ifdef ALLEGRO_WINDOWS
      fprintf(f, "#ifndef ALLEGRO_WINDOWS\n");
      fprintf(f, "#define ALLEGRO_WINDOWS\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   #ifdef ALLEGRO_LINUX
      fprintf(f, "#ifndef ALLEGRO_LINUX\n");
      fprintf(f, "#define ALLEGRO_LINUX\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   #ifdef ALLEGRO_LINUX_VBEAF
      fprintf(f, "#ifndef ALLEGRO_LINUX_VBEAF\n");
      fprintf(f, "#define ALLEGRO_LINUX_VBEAF\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   #ifdef ALLEGRO_COLOR8
      fprintf(f, "#ifndef ALLEGRO_COLOR8\n");
      fprintf(f, "#define ALLEGRO_COLOR8\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   #ifdef ALLEGRO_COLOR16
      fprintf(f, "#ifndef ALLEGRO_COLOR16\n");
      fprintf(f, "#define ALLEGRO_COLOR16\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   #ifdef ALLEGRO_COLOR24
      fprintf(f, "#ifndef ALLEGRO_COLOR24\n");
      fprintf(f, "#define ALLEGRO_COLOR24\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   #ifdef ALLEGRO_COLOR32
      fprintf(f, "#ifndef ALLEGRO_COLOR32\n");
      fprintf(f, "#define ALLEGRO_COLOR32\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   #ifdef ALLEGRO_MMX
      fprintf(f, "#ifndef ALLEGRO_MMX\n");
      fprintf(f, "#define ALLEGRO_MMX\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   #ifdef ALLEGRO_NO_ASM
      fprintf(f, "#ifndef ALLEGRO_NO_ASM\n");
      fprintf(f, "#define ALLEGRO_NO_ASM\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   fprintf(f, "#define BMP_W                 %d\n",   (int)offsetof(BITMAP, w));
   fprintf(f, "#define BMP_H                 %d\n",   (int)offsetof(BITMAP, h));
   fprintf(f, "#define BMP_CLIP              %d\n",   (int)offsetof(BITMAP, clip));
   fprintf(f, "#define BMP_CL                %d\n",   (int)offsetof(BITMAP, cl));
   fprintf(f, "#define BMP_CR                %d\n",   (int)offsetof(BITMAP, cr));
   fprintf(f, "#define BMP_CT                %d\n",   (int)offsetof(BITMAP, ct));
   fprintf(f, "#define BMP_CB                %d\n",   (int)offsetof(BITMAP, cb));
   fprintf(f, "#define BMP_VTABLE            %d\n",   (int)offsetof(BITMAP, vtable));
   fprintf(f, "#define BMP_WBANK             %d\n",   (int)offsetof(BITMAP, write_bank));
   fprintf(f, "#define BMP_RBANK             %d\n",   (int)offsetof(BITMAP, read_bank));
   fprintf(f, "#define BMP_DAT               %d\n",   (int)offsetof(BITMAP, dat));
   fprintf(f, "#define BMP_ID                %d\n",   (int)offsetof(BITMAP, id));
   fprintf(f, "#define BMP_EXTRA             %d\n",   (int)offsetof(BITMAP, extra));
   fprintf(f, "#define BMP_XOFFSET           %d\n",   (int)offsetof(BITMAP, x_ofs));
   fprintf(f, "#define BMP_YOFFSET           %d\n",   (int)offsetof(BITMAP, y_ofs));
   fprintf(f, "#define BMP_SEG               %d\n",   (int)offsetof(BITMAP, seg));
   fprintf(f, "#define BMP_LINE              %d\n",   (int)offsetof(BITMAP, line));
   fprintf(f, "\n");

   fprintf(f, "#ifndef BMP_ID_VIDEO\n");
   fprintf(f, "#define BMP_ID_VIDEO          0x%08X\n",  BMP_ID_VIDEO);
   fprintf(f, "#define BMP_ID_SYSTEM         0x%08X\n",  BMP_ID_SYSTEM);
   fprintf(f, "#define BMP_ID_SUB            0x%08X\n",  BMP_ID_SUB);
   fprintf(f, "#define BMP_ID_PLANAR         0x%08X\n",  BMP_ID_PLANAR);
   fprintf(f, "#define BMP_ID_NOBLIT         0x%08X\n",  BMP_ID_NOBLIT);
   fprintf(f, "#define BMP_ID_LOCKED         0x%08X\n",  BMP_ID_LOCKED);
   fprintf(f, "#define BMP_ID_AUTOLOCK       0x%08X\n",  BMP_ID_AUTOLOCK);
   fprintf(f, "#define BMP_ID_MASK           0x%08X\n",  BMP_ID_MASK);
   fprintf(f, "#endif\n");
   fprintf(f, "\n");

   fprintf(f, "#define VTABLE_COLOR_DEPTH    %d\n",   (int)offsetof(GFX_VTABLE, color_depth));
   fprintf(f, "#define VTABLE_MASK_COLOR     %d\n",   (int)offsetof(GFX_VTABLE, mask_color));
   fprintf(f, "#define VTABLE_UNBANK         %d\n",   (int)offsetof(GFX_VTABLE, unwrite_bank));
   fprintf(f, "\n");

   fprintf(f, "#define RLE_W                 %d\n",   (int)offsetof(RLE_SPRITE, w));
   fprintf(f, "#define RLE_H                 %d\n",   (int)offsetof(RLE_SPRITE, h));
   fprintf(f, "#define RLE_DAT               %d\n",   (int)offsetof(RLE_SPRITE, dat));
   fprintf(f, "\n");

   #ifndef ALLEGRO_USE_C
      fprintf(f, "#define CMP_PLANAR            %d\n",   (int)offsetof(COMPILED_SPRITE, planar));
      fprintf(f, "#define CMP_COLOR_DEPTH       %d\n",   (int)offsetof(COMPILED_SPRITE, color_depth));
      fprintf(f, "#define CMP_DRAW              %d\n",   (int)offsetof(COMPILED_SPRITE, proc));
      fprintf(f, "\n");
   #endif

   fprintf(f, "#define DRAW_SOLID            %d\n",   DRAW_MODE_SOLID);
   fprintf(f, "#define DRAW_XOR              %d\n",   DRAW_MODE_XOR);
   fprintf(f, "#define DRAW_COPY_PATTERN     %d\n",   DRAW_MODE_COPY_PATTERN);
   fprintf(f, "#define DRAW_SOLID_PATTERN    %d\n",   DRAW_MODE_SOLID_PATTERN);
   fprintf(f, "#define DRAW_MASKED_PATTERN   %d\n",   DRAW_MODE_MASKED_PATTERN);
   fprintf(f, "#define DRAW_TRANS            %d\n",   DRAW_MODE_TRANS);
   fprintf(f, "\n");

   fprintf(f, "#ifndef MASK_COLOR_8\n");
   fprintf(f, "#define MASK_COLOR_8          %d\n",   MASK_COLOR_8);
   fprintf(f, "#define MASK_COLOR_15         %d\n",   MASK_COLOR_15);
   fprintf(f, "#define MASK_COLOR_16         %d\n",   MASK_COLOR_16);
   fprintf(f, "#define MASK_COLOR_24         %d\n",   MASK_COLOR_24);
   fprintf(f, "#define MASK_COLOR_32         %d\n",   MASK_COLOR_32);
   fprintf(f, "#endif\n");
   fprintf(f, "\n");

   fprintf(f, "#define POLYSEG_U             %d\n",   (int)offsetof(POLYGON_SEGMENT, u));
   fprintf(f, "#define POLYSEG_V             %d\n",   (int)offsetof(POLYGON_SEGMENT, v));
   fprintf(f, "#define POLYSEG_DU            %d\n",   (int)offsetof(POLYGON_SEGMENT, du));
   fprintf(f, "#define POLYSEG_DV            %d\n",   (int)offsetof(POLYGON_SEGMENT, dv));
   fprintf(f, "#define POLYSEG_C             %d\n",   (int)offsetof(POLYGON_SEGMENT, c));
   fprintf(f, "#define POLYSEG_DC            %d\n",   (int)offsetof(POLYGON_SEGMENT, dc));
   fprintf(f, "#define POLYSEG_R             %d\n",   (int)offsetof(POLYGON_SEGMENT, r));
   fprintf(f, "#define POLYSEG_G             %d\n",   (int)offsetof(POLYGON_SEGMENT, g));
   fprintf(f, "#define POLYSEG_B             %d\n",   (int)offsetof(POLYGON_SEGMENT, b));
   fprintf(f, "#define POLYSEG_DR            %d\n",   (int)offsetof(POLYGON_SEGMENT, dr));
   fprintf(f, "#define POLYSEG_DG            %d\n",   (int)offsetof(POLYGON_SEGMENT, dg));
   fprintf(f, "#define POLYSEG_DB            %d\n",   (int)offsetof(POLYGON_SEGMENT, db));
   fprintf(f, "#define POLYSEG_Z             %d\n",   (int)offsetof(POLYGON_SEGMENT, z));
   fprintf(f, "#define POLYSEG_DZ            %d\n",   (int)offsetof(POLYGON_SEGMENT, dz));
   fprintf(f, "#define POLYSEG_FU            %d\n",   (int)offsetof(POLYGON_SEGMENT, fu));
   fprintf(f, "#define POLYSEG_FV            %d\n",   (int)offsetof(POLYGON_SEGMENT, fv));
   fprintf(f, "#define POLYSEG_DFU           %d\n",   (int)offsetof(POLYGON_SEGMENT, dfu));
   fprintf(f, "#define POLYSEG_DFV           %d\n",   (int)offsetof(POLYGON_SEGMENT, dfv));
   fprintf(f, "#define POLYSEG_TEXTURE       %d\n",   (int)offsetof(POLYGON_SEGMENT, texture));
   fprintf(f, "#define POLYSEG_UMASK         %d\n",   (int)offsetof(POLYGON_SEGMENT, umask));
   fprintf(f, "#define POLYSEG_VMASK         %d\n",   (int)offsetof(POLYGON_SEGMENT, vmask));
   fprintf(f, "#define POLYSEG_VSHIFT        %d\n",   (int)offsetof(POLYGON_SEGMENT, vshift));
   fprintf(f, "#define POLYSEG_SEG           %d\n",   (int)offsetof(POLYGON_SEGMENT, seg));
   fprintf(f, "\n");

   fprintf(f, "#define ERANGE                %d\n",   ERANGE);
   fprintf(f, "\n");

   for (x=0; x<3; x++)
      for (y=0; y<3; y++)
	 fprintf(f, "#define M_V%d%d                 %d\n", x, y, (int)offsetof(MATRIX_f, v[x][y]));

   for (x=0; x<3; x++)
      fprintf(f, "#define M_T%d                  %d\n", x, (int)offsetof(MATRIX_f, t[x]));

   fprintf(f, "\n");

   #ifdef ALLEGRO_DOS
      fprintf(f, "#define IRQ_SIZE              %d\n",   (int)sizeof(_IRQ_HANDLER));
      fprintf(f, "#define IRQ_HANDLER           %d\n",   (int)offsetof(_IRQ_HANDLER, handler));
      fprintf(f, "#define IRQ_NUMBER            %d\n",   (int)offsetof(_IRQ_HANDLER, number));
      fprintf(f, "#define IRQ_OLDVEC            %d\n",   (int)offsetof(_IRQ_HANDLER, old_vector));
      fprintf(f, "\n");

      fprintf(f, "#define DPMI_AX               %d\n",   (int)offsetof(__dpmi_regs, x.ax));
      fprintf(f, "#define DPMI_BX               %d\n",   (int)offsetof(__dpmi_regs, x.bx));
      fprintf(f, "#define DPMI_CX               %d\n",   (int)offsetof(__dpmi_regs, x.cx));
      fprintf(f, "#define DPMI_DX               %d\n",   (int)offsetof(__dpmi_regs, x.dx));
      fprintf(f, "#define DPMI_SP               %d\n",   (int)offsetof(__dpmi_regs, x.sp));
      fprintf(f, "#define DPMI_SS               %d\n",   (int)offsetof(__dpmi_regs, x.ss));
      fprintf(f, "#define DPMI_FLAGS            %d\n",   (int)offsetof(__dpmi_regs, x.flags));
      fprintf(f, "\n");
   #endif

   #ifdef ALLEGRO_ASM_USE_FS
      fprintf(f, "#define USE_FS\n");
      fprintf(f, "#define FSEG                  %%fs:\n");
   #else
      fprintf(f, "#define FSEG\n");
   #endif
      fprintf(f, "\n");

   #ifdef ALLEGRO_ASM_PREFIX
      #define PREFIX    ALLEGRO_ASM_PREFIX "##"
   #else
      #define PREFIX    ""
   #endif

   #ifdef ALLEGRO_WATCOM
      fprintf(f, "#define FUNC(name)            .globl " PREFIX "name ; nop ; _align_ ; " PREFIX "name:\n");
   #else
      fprintf(f, "#define FUNC(name)            .globl " PREFIX "name ; _align_ ; " PREFIX "name:\n");
   #endif

   fprintf(f, "#define GLOBL(name)           " PREFIX "name\n");
   fprintf(f, "\n");

   if (ferror(f)) {
      fprintf(stderr, "%s: cannot write file %s\n", argv[0], argv[1]);
      return 1;
   }

   if (fclose(f)) {
      fprintf(stderr, "%s: cannot close file %s\n", argv[0], argv[1]);
      return 1;
   }

   return 0;
}

END_OF_MAIN();
