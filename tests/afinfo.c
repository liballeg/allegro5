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
 *      Allegro library utility for checking which VBE/AF modes are available.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */

#define ALLEGRO_USE_CONSOLE

#include <stdio.h>
#include <string.h>

#include "allegro.h"


/* this program is not portable! */
#if (defined ALLEGRO_DOS) || (defined ALLEGRO_LINUX_VBEAF)

#include "allegro/internal/aintern.h"

#ifdef ALLEGRO_INTERNAL_HEADER
   #include ALLEGRO_INTERNAL_HEADER
#endif

#if (defined ALLEGRO_DJGPP) && (!defined SCAN_DEPEND)
   #include <sys/nearptr.h>
#endif



#ifdef ALLEGRO_LINUX

   #include <sys/mman.h>

   /* quick and dirty Linux emulation of the DOS memory mapping routines */
   #define _create_linear_mapping(_addr, _base, _len)       \
   ({                                                       \
      (_addr)->base = _base;                                \
      (_addr)->size = _len;                                 \
      (_addr)->perms = PROT_READ | PROT_WRITE;              \
      __al_linux_map_memory(_addr);                         \
   })

   #define _remove_linear_mapping(_addr)                    \
   {                                                        \
      if ((_addr)->data)                                    \
	 __al_linux_unmap_memory(_addr);                    \
   }

   #define MMAP      struct MAPPED_MEMORY
   #define NOMM      { 0, 0, 0, 0 }
   #define MVAL(a)   a.data

#else

   /* DOS version */
   #define MMAP      unsigned long
   #define NOMM      0
   #define MVAL(a)   (void *)((a)-__djgpp_base_address)

#endif



int verbose = FALSE;



#ifdef ALLEGRO_GCC
   #define __PACKED__   __attribute__ ((packed))
#else
   #define __PACKED__
#endif

#ifdef ALLEGRO_WATCOM
   #pragma pack (1)
#endif



typedef struct AF_MODE_INFO
{
   unsigned short Attributes              __PACKED__;
   unsigned short XResolution             __PACKED__;
   unsigned short YResolution             __PACKED__;
   unsigned short BytesPerScanLine        __PACKED__;
   unsigned short BitsPerPixel            __PACKED__;
   unsigned short MaxBuffers              __PACKED__;
   unsigned char  RedMaskSize;
   unsigned char  RedFieldPosition;
   unsigned char  GreenMaskSize;
   unsigned char  GreenFieldPosition;
   unsigned char  BlueMaskSize;
   unsigned char  BlueFieldPosition;
   unsigned char  RsvdMaskSize;
   unsigned char  RsvdFieldPosition;
   unsigned short MaxBytesPerScanLine     __PACKED__;
   unsigned short MaxScanLineWidth        __PACKED__;
   unsigned short LinBytesPerScanLine     __PACKED__;
   unsigned char  BnkMaxBuffers;
   unsigned char  LinMaxBuffers;
   unsigned char  LinRedMaskSize;
   unsigned char  LinRedFieldPosition;
   unsigned char  LinGreenMaskSize;
   unsigned char  LinGreenFieldPosition;
   unsigned char  LinBlueMaskSize;
   unsigned char  LinBlueFieldPosition;
   unsigned char  LinRsvdMaskSize;
   unsigned char  LinRsvdFieldPosition;
   unsigned long  MaxPixelClock           __PACKED__;
   unsigned long  VideoCapabilities       __PACKED__;
   unsigned short VideoMinXScale          __PACKED__;
   unsigned short VideoMinYScale          __PACKED__;
   unsigned short VideoMaxXScale          __PACKED__;
   unsigned short VideoMaxYScale          __PACKED__;
   unsigned char  reserved[76];
} AF_MODE_INFO;



#define DC  struct AF_DRIVER *dc



typedef struct AF_DRIVER
{
   char           Signature[12];
   unsigned long  Version                 __PACKED__;
   unsigned long  DriverRev               __PACKED__;
   char           OemVendorName[80];
   char           OemCopyright[80];
   short          *AvailableModes         __PACKED__;
   unsigned long  TotalMemory             __PACKED__;
   unsigned long  Attributes              __PACKED__;
   unsigned long  BankSize                __PACKED__;
   unsigned long  BankedBasePtr           __PACKED__;
   unsigned long  LinearSize              __PACKED__;
   unsigned long  LinearBasePtr           __PACKED__;
   unsigned long  LinearGranularity       __PACKED__;
   unsigned short *IOPortsTable           __PACKED__;
   unsigned long  IOMemoryBase[4]         __PACKED__;
   unsigned long  IOMemoryLen[4]          __PACKED__;
   unsigned long  LinearStridePad         __PACKED__;
   unsigned short PCIVendorID             __PACKED__;
   unsigned short PCIDeviceID             __PACKED__;
   unsigned short PCISubSysVendorID       __PACKED__;
   unsigned short PCISubSysID             __PACKED__;
   unsigned long  Checksum                __PACKED__;
   unsigned long  res2[6]                 __PACKED__;
   void           *IOMemMaps[4]           __PACKED__;
   void           *BankedMem              __PACKED__;
   void           *LinearMem              __PACKED__;
   unsigned long  res3[5]                 __PACKED__;
   unsigned long  BufferEndX              __PACKED__;
   unsigned long  BufferEndY              __PACKED__;
   unsigned long  OriginOffset            __PACKED__;
   unsigned long  OffscreenOffset         __PACKED__;
   unsigned long  OffscreenStartY         __PACKED__;
   unsigned long  OffscreenEndY           __PACKED__;
   unsigned long  res4[10]                __PACKED__;
   unsigned long  SetBank32Len            __PACKED__;
   void           *SetBank32;
   void           *Int86;
   void           *CallRealMode;
   void           *InitDriver;
   void           *af10Funcs[40];
   void           *PlugAndPlayInit;
   void           *(*OemExt)(DC, unsigned long id);
   void           *SupplementalExt;
   long           (*GetVideoModeInfo)(DC, short mode, AF_MODE_INFO *modeInfo);
} AF_DRIVER;



#undef DC



#define FAF_ID(a,b,c,d)    ((a<<24) | (b<<16) | (c<<8) | d)

#define FAFEXT_INIT     FAF_ID('I','N','I','T')
#define FAFEXT_MAGIC    FAF_ID('E','X', 0,  0)

#define FAFEXT_LIBC     FAF_ID('L','I','B','C')
#define FAFEXT_PMODE    FAF_ID('P','M','O','D')

#define FAFEXT_HWPTR    FAF_ID('H','P','T','R')

#define FAFEXT_CONFIG   FAF_ID('C','O','N','F')


typedef struct FAF_CONFIG_DATA
{
   unsigned long id;
   unsigned long value;
} FAF_CONFIG_DATA;



AF_DRIVER *af = NULL;

MMAP af_memmap[4] = { NOMM, NOMM, NOMM, NOMM };
MMAP af_banked_mem = NOMM;
MMAP af_linear_mem = NOMM;



#ifdef ALLEGRO_DOS

/* hooks to let the driver call BIOS routines */
extern void _af_int86(void), _af_call_rm(void);

#else

/* no BIOS on this platform, my friend! */
void nobios(void) { }

#endif



void af_exit(void)
{
   int c;

   for (c=0; c<4; c++)
      _remove_linear_mapping(&af_memmap[c]);

   _remove_linear_mapping(&af_banked_mem);
   _remove_linear_mapping(&af_linear_mem);

   if (af) {
      free(af);
      af = NULL;
   }
}



static int call_vbeaf_asm(void *proc)
{
   int ret;

   proc = (void *)((long)af + (long)proc);

   #ifdef ALLEGRO_GCC

      /* use gcc-style inline asm */
      asm (
	 " call *%%edx "

      : "=&a" (ret)                       /* return value in eax */

      : "b" (af),                         /* VBE/AF driver in ds:ebx */
	"d" (proc)                        /* function ptr in edx */

      : "memory"                          /* assume everything is clobbered */
      );

   #elif defined ALLEGRO_WATCOM

      /* use Watcom-style inline asm */
      {
	 int _af(void *func, AF_DRIVER *driver);

	 #pragma aux _af =                \
	    " call esi "                  \
					  \
	 parm [esi] [ebx]                 \
	 modify [ecx edx edi]             \
	 value [eax];

	 ret = _af(proc, af);
      }

   #else

      /* don't know what to do on this compiler */
      ret = -1;

   #endif

   return ret;
}



int load_vbeaf_driver(char *filename)
{
   long size;
   PACKFILE *f;

   size = file_size_ex(filename);
   if (size <= 0)
      return -1;

   f = pack_fopen(filename, F_READ);
   if (!f)
      return -1;

   af = malloc(size);

   if (pack_fread(af, size, f) != size) {
      free(af);
      af = NULL;
      return -1;
   }

   pack_fclose(f);

   return 0;
}



void initialise_freebeaf_extensions(void)
{
   typedef unsigned long (*EXT_INIT_FUNC)(AF_DRIVER *af, unsigned long id);
   EXT_INIT_FUNC ext_init;
   FAF_CONFIG_DATA *cfg;
   unsigned long magic;
   int v1, v2;

   if (!af->OemExt) {
      printf("no OemExt\n");
      return;
   }

   ext_init = (EXT_INIT_FUNC)((long)af + (long)af->OemExt);

   magic = ext_init(af, FAFEXT_INIT);

   v1 = (magic>>8)&0xFF;
   v2 = magic&0xFF;

   if (((magic&0xFFFF0000) != FAFEXT_MAGIC) || (!uisdigit(v1)) || (!uisdigit(v2))) {
      printf("bad magic number!\n");
      return;
   }

   printf("EX%c%c", v1, v2);

   if (af->OemExt(af, FAFEXT_LIBC))
      printf(", FAFEXT_LIBC");

   if (af->OemExt(af, FAFEXT_PMODE))
      printf(", FAFEXT_PMODE");

   if (af->OemExt(af, FAFEXT_HWPTR))
      printf(", FAFEXT_HWPTR");

   cfg = af->OemExt(af, FAFEXT_CONFIG);
   if (cfg)
      printf(", FAFEXT_CONFIG");

   printf("\n");

   if (cfg) {
      while (cfg->id) {
	 printf("Config variable:\t%c%c%c%c = 0x%08lX\n", 
		(char)(cfg->id>>24), (char)(cfg->id>>16), 
		(char)(cfg->id>>8), (char)(cfg->id), cfg->value);

	 cfg++;
      }
   }
}



static int initialise_vbeaf(void)
{
   int c;

   #ifdef ALLEGRO_DOS
      if (__djgpp_nearptr_enable() == 0)
	 return -1;
   #else
      if (!__al_linux_have_ioperms)
	 return -1;
   #endif

   for (c=0; c<4; c++) {
      if (af->IOMemoryBase[c]) {
	 if (_create_linear_mapping(&af_memmap[c], af->IOMemoryBase[c], 
				    af->IOMemoryLen[c]) != 0)
	    return -1;

	 af->IOMemMaps[c] = MVAL(af_memmap[c]);
      }
   }

   if (af->BankedBasePtr) {
      if (_create_linear_mapping(&af_banked_mem, af->BankedBasePtr, 0x10000) != 0)
	 return -1;

      af->BankedMem = MVAL(af_banked_mem);
   }

   if (af->LinearBasePtr) {
      if (_create_linear_mapping(&af_linear_mem, af->LinearBasePtr, af->LinearSize*1024) != 0)
	 return -1;

      af->LinearMem  = MVAL(af_linear_mem);
   }

   #ifdef ALLEGRO_DOS
      af->Int86 = _af_int86;
      af->CallRealMode = _af_call_rm;
   #else
      af->Int86 = nobios;
      af->CallRealMode = nobios;
   #endif

   return 0;
}



void print_af_attributes(unsigned long attrib)
{
   static char *flags[] =
   {
      "multibuf",
      "scroll",
      "banked",
      "linear",
      "accel",
      "dualbuf",
      "hwcursor",
      "8bitdac",
      "nonvga",
      "2scan",
      "interlace",
      "3buffer",
      "stereo",
      "rop2",
      "hwstsync",
      "evcstsync"
   };

   int first = TRUE;
   int c;

   printf("Attributes:\t\t");

   for (c=0; c<(int)(sizeof(flags)/sizeof(flags[0])); c++) {
      if (attrib & (1<<c)) {
	 if (first)
	    first = FALSE;
	 else
	    printf(", ");

	 printf(flags[c]);
      }
   }

   printf("\n");
}



int get_af_info(void)
{
   static char *possible_filenames[] =
   {
   #ifdef ALLEGRO_DOS
      "c:\\vbeaf.drv",
   #else
      "/usr/local/lib/vbeaf.drv",
      "/usr/lib/vbeaf.drv",
      "/lib/vbeaf.drv",
      "/vbeaf.drv",
   #endif
      NULL
   };

   char filename[256];
   AL_CONST char *p;
   int c, attrib;

   p = get_config_string(NULL, "vbeaf_driver", NULL);
   if ((p) && (*p)) {
      strcpy(filename, p);
      if (*get_filename(filename) == 0) {
	 strcat(filename, "vbeaf.drv");
      }
      else {
	 if (file_exists(filename, FA_DIREC, &attrib)) {
	    if (attrib & FA_DIREC) {
	       put_backslash(filename);
	       strcat(filename, "vbeaf.drv");
	    }
	 }
      }
      if (load_vbeaf_driver(filename) == 0)
	 goto found_it;
   }

   for (c=0; possible_filenames[c]; c++) {
      strcpy(filename, possible_filenames[c]);
      if (load_vbeaf_driver(filename) == 0)
	 goto found_it;
   }

   p = getenv("VBEAF_PATH");
   if (p) {
      strcpy(filename, p);
      put_backslash(filename);
      strcat(filename, "vbeaf.drv");
      if (load_vbeaf_driver(filename) == 0)
	 goto found_it;
   }

   #ifdef ALLEGRO_DOS
      printf("Error: no VBE/AF driver found!\nYou should have a vbeaf.drv file in the root of your C: drive\n");
   #else
      printf("Error: no VBE/AF driver found!\nYou should have a vbeaf.drv file in the root of your filesystem\n");
   #endif

   return -1;

   found_it:

   printf("Driver file:\t\t%s\n", filename);

   if (strcmp(af->Signature, "VBEAF.DRV") != 0) {
      af_exit();
      printf("\nError: bad header ID in this driver file!\n");
      return -1;
   }

   if (af->Version < 0x200) {
      af_exit();
      printf("Error: obsolete driver version! (0x%lX)\n", af->Version);
      return -1;
   }

   printf("FreeBE/AF ext:\t\t");

   if (strstr(af->OemVendorName, "FreeBE"))
      initialise_freebeaf_extensions();
   else
      printf("not a FreeBE/AF driver\n");

   if (call_vbeaf_asm(af->PlugAndPlayInit) != 0) {
      af_exit();
      printf("\nError: VBE/AF Plug and Play initialisation failed!\n");
      return -1;
   }

   if (initialise_vbeaf() != 0) {
      af_exit();
      printf("\nError: cannot map VBE/AF device memory!\n");
      return -1;
   }

   if (call_vbeaf_asm(af->InitDriver) != 0) {
      af_exit();
      printf("\nError: VBE/AF device not present!\n");
      return -1;
   }

   printf("VBE/AF version:\t\t0x%lX\n", af->Version);
   printf("VendorName:\t\t%s\n", af->OemVendorName);
   printf("VendorCopyright:\t%s\n", af->OemCopyright);
   printf("TotalMemory:\t\t%ld\n", af->TotalMemory);

   print_af_attributes(af->Attributes);

   if (verbose) {
      printf("BankedAddr:\t\t0x%08lX\n", af->BankedBasePtr);
      printf("BankedSize:\t\t0x%08lX\n", af->BankSize*1024);
      printf("LinearAddr:\t\t0x%08lX\n", af->LinearBasePtr);
      printf("LinearSize:\t\t0x%08lX\n", af->LinearSize*1024);

      for (c=0; c<4; c++) {
	 printf("IOMemoryBase[%d]:\t0x%08lX\n", c, af->IOMemoryBase[c]);
	 printf("IOMemoryLen[%d]:\t\t0x%08lX\n", c, af->IOMemoryLen[c]);
      }

      printf("\n\n\n");
   }
   else
      printf("\n");

   return 0;
}



typedef struct COLORINFO
{
   int size;
   int pos;
   char c;
} COLORINFO;

COLORINFO colorinfo[4];

int colorcount = 0;



int color_cmp(const void *e1, const void *e2)
{
   COLORINFO *c1 = (COLORINFO *)e1;
   COLORINFO *c2 = (COLORINFO *)e2;

   return c2->pos - c1->pos;
}



void add_color(int size, int pos, char c)
{
   if ((size > 0) || (pos > 0)) {
      colorinfo[colorcount].size = size;
      colorinfo[colorcount].pos = pos;
      colorinfo[colorcount].c = c;
      colorcount++;
   }
}



void get_color_desc(char *s)
{
   int c;

   if (colorcount > 0) {
      qsort(colorinfo, colorcount, sizeof(COLORINFO), color_cmp);

      for (c=0; c<colorcount; c++)
	 *(s++) = colorinfo[c].c;

      *(s++) = ' ';

      for (c=0; c<colorcount; c++)
	 *(s++) = '0' + colorinfo[c].size;

      colorcount = 0;
   }

   *s = 0;
}



int get_mode_info(int mode)
{
   AF_MODE_INFO mode_info;
   char color_desc[80];
   char buf[80];

   if (!verbose) {
      sprintf(buf, "%X:", mode);
      printf("Mode 0x%-5s", buf);
   }
   else
      printf("Mode 0x%X:\n\n", mode);

   if (af->GetVideoModeInfo(af, mode, &mode_info) != 0) {
      printf("GetVideoModeInfo failed!\n");
      if (verbose)
	 printf("\n\n\n");
      return -1;
   }

   add_color(mode_info.RedMaskSize, mode_info.RedFieldPosition, 'R');
   add_color(mode_info.GreenMaskSize, mode_info.GreenFieldPosition, 'G');
   add_color(mode_info.BlueMaskSize, mode_info.BlueFieldPosition, 'B');
   add_color(mode_info.RsvdMaskSize, mode_info.RsvdFieldPosition, 'x');
   get_color_desc(color_desc);

   if (verbose) {
      print_af_attributes(mode_info.Attributes);

      if (color_desc[0])
	 printf("Format:\t\t\t%s\n", color_desc);

      printf("XResolution:\t\t%d\n", mode_info.XResolution);
      printf("YResolution:\t\t%d\n", mode_info.YResolution);
      printf("BitsPerPixel:\t\t%d\n", mode_info.BitsPerPixel);
      printf("BytesPerScanLine:\t%d\n", mode_info.BytesPerScanLine);
      printf("LinBytesPerScanLine:\t%d\n", mode_info.LinBytesPerScanLine);
      printf("MaxBytesPerScanLine:\t%d\n", mode_info.MaxBytesPerScanLine);
      printf("MaxScanLineWidth:\t%d\n", mode_info.MaxScanLineWidth);
      printf("MaxBuffers:\t\t%d\n", mode_info.MaxBuffers);
      printf("BnkMaxBuffers:\t\t%d\n", mode_info.BnkMaxBuffers);
      printf("LinMaxBuffers:\t\t%d\n", mode_info.LinMaxBuffers);
      printf("RedMaskSize:\t\t%d\n", mode_info.RedMaskSize);
      printf("RedFieldPos:\t\t%d\n", mode_info.RedFieldPosition);
      printf("GreenMaskSize:\t\t%d\n", mode_info.GreenMaskSize);
      printf("GreenFieldPos:\t\t%d\n", mode_info.GreenFieldPosition);
      printf("BlueMaskSize:\t\t%d\n", mode_info.BlueMaskSize);
      printf("BlueFieldPos:\t\t%d\n", mode_info.BlueFieldPosition);
      printf("RsvdMaskSize:\t\t%d\n", mode_info.RsvdMaskSize);
      printf("RsvdFieldPos:\t\t%d\n", mode_info.RsvdFieldPosition);
      printf("LinRedMaskSize:\t\t%d\n", mode_info.LinRedMaskSize);
      printf("LinRedFieldPos:\t\t%d\n", mode_info.LinRedFieldPosition);
      printf("LinGreenMaskSize:\t%d\n", mode_info.LinGreenMaskSize);
      printf("LinGreenFieldPos:\t%d\n", mode_info.LinGreenFieldPosition);
      printf("LinBlueMaskSize:\t%d\n", mode_info.LinBlueMaskSize);
      printf("LinBlueFieldPos:\t%d\n", mode_info.LinBlueFieldPosition);
      printf("LinRsvdMaskSize:\t%d\n", mode_info.LinRsvdMaskSize);
      printf("LinRsvdFieldPos:\t%d\n", mode_info.LinRsvdFieldPosition);
      printf("MaxPixelClock:\t\t%ld\n", mode_info.MaxPixelClock);

      printf("\n\n\n");
   }
   else {
      printf("%5dx%-6d", mode_info.XResolution, mode_info.YResolution);
      printf("%d bpp", mode_info.BitsPerPixel);
      if (color_desc[0])
	 printf(" (%s)", color_desc);
      printf("\n");
   }

   return 0;
}



int main(int argc, char *argv[])
{
   int c;

   if (allegro_init() != 0)
      return 1;

   #ifdef SYSTEM_XWINDOWS
      if (system_driver->id == SYSTEM_XWINDOWS) {
	 allegro_message("Sorry, VBE/AF is not available under X, run AFINFO from Linux console\n");
	 return 1;
      }
   #endif

   #ifdef ALLEGRO_LINUX
      if (!__al_linux_have_ioperms) {
	 printf("Sorry, you need to be root before you can do stuff with VBE/AF\n");
	 return 1;
      } 
   #endif

   for (c=1; c<argc; c++)
      if (((argv[c][0] == '-') || (argv[c][0] == '/')) &&
	  ((argv[c][1] == 'v') || (argv[c][1] == 'V')))
	 verbose = TRUE;

   printf("\nAllegro VBE/AF info utility " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR);
   printf("\nBy Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n");

   if (get_af_info() != 0)
      return -1;

   for (c=0; af->AvailableModes[c] != -1; c++)
      get_mode_info(af->AvailableModes[c]);

   if (!verbose)
      printf("\n'afinfo -v' for a verbose listing\n\n");

   af_exit();

   return 0;
}



#else       /* ifdef VBE/AF cool on this platform */


int main(void)
{
   allegro_init();
   allegro_message("Sorry, the AFINFO program only works on DOS or Linux\n");
   return 1;
}


#endif


END_OF_MAIN()
