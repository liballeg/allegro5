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
 *      MacOS X application bundle fixer utility. Creates an application
 *      bundle out of an executable and some optional icon image files.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#define ALLEGRO_USE_CONSOLE

#include <stdio.h>
#include <string.h>
#include <allegro5.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#undef TRUE
#undef FALSE

#ifndef SCAN_DEPEND
   #include <Carbon/Carbon.h>
#endif

#undef TRUE
#undef FALSE
#define TRUE -1
#define FALSE 0

/* 16x16 */
#define ICON_SMALL		0
/* 32x32 */
#define ICON_LARGE		1
/* 48x48 */
#define ICON_HUGE		2
/* 128x128 */
#define ICON_THUMBNAIL		3


#define F_SMALL_DEFINED		0x1
#define F_LARGE_DEFINED		0x2
#define F_HUGE_DEFINED		0x4
#define F_THUMBNAIL_DEFINED	0x8
#define F_ICONS_DEFINED		0xf
#define F_MOVE			0x10
#define F_GOT_VERSION		0x20
#define F_GOT_LONG_VERSION	0x40
#define F_EMBED_FRAMEWORK	0x80

#define MAX_STRING_SIZE		1024
#define ONE_SIXTH		(1.0 / 6.0)


typedef struct ICON_DATA
{
   BITMAP *original, *workspace, *scaled;
   int size;
   OSType data, mask8, mask1;
   int defined;
} ICON_DATA;


static ICON_DATA icon_data[4] = {
 { NULL, NULL, NULL, 16, kSmall32BitData, kSmall8BitMask, kSmall1BitMask, F_SMALL_DEFINED },
 { NULL, NULL, NULL, 32, kLarge32BitData, kLarge8BitMask, kLarge1BitMask, F_LARGE_DEFINED },
 { NULL, NULL, NULL, 48, kHuge32BitData, kHuge8BitMask, kHuge1BitMask, F_HUGE_DEFINED },
 { NULL, NULL, NULL, 128, kThumbnail32BitData, kThumbnail8BitMask, 0, F_THUMBNAIL_DEFINED }
};
static int flags = 0;



static float cubic_bspline(float x)
{
   float a, b, c, d;

   if (x <= -2.0)
      a = 0.0;
   else
      a = (x + 2.0) * (x + 2.0) * (x + 2.0);

   if (x <= -1.0)
      b = 0.0;
   else
      b = ((x + 1.0) * (x + 1.0) * (x + 1.0)) * -4.0;

   if (x <= 0)
      c = 0.0;
   else
      c = (x * x * x) * 6.0;

   if (x <= 1.0)
      d = 0.0;
   else
      d = ((x - 1.0) * (x - 1.0) * (x - 1.0)) * -4.0;
  
   return (a + b + c + d) * ONE_SIXTH;
}



static int scale_icon(ICON_DATA *icon)
{
   BITMAP *shape;
   int size, x_ofs = 2, y_ofs = 2;
   int x, y, m, n;
   int i_x, i_y;
   float k, f_x, f_y, a, b, r1, r2;
   float red, green, blue, alpha;
   unsigned char *p;
   unsigned int color;
   
   if (icon->original->w > icon->original->h) {
      size = 4 + icon->original->w;
      y_ofs = 2 + ((icon->original->w - icon->original->h) / 2);
   }
   else {
      size = 4 + icon->original->h;
      x_ofs = 2 + ((icon->original->h - icon->original->w) / 2);
   }
   k = (float)(size - 4) / (float)icon->size;
   icon->workspace = create_bitmap_ex(32, size, size);
   if (!icon->workspace)
      return -1;
   icon->scaled = create_bitmap_ex(32, icon->size, icon->size);
   if (!icon->scaled)
      return -1;
   shape = create_bitmap_ex(32, icon->original->w, icon->original->h);
   if (!shape)
      return -1;
   blit(icon->original, shape, 0, 0, 0, 0, icon->original->w, icon->original->h);
   clear_to_color(icon->workspace, makeacol32(0, 0, 0, 255));
   shape->vtable->mask_color = makeacol32(255, 0, 255, 0);
   masked_blit(shape, icon->workspace, 0, 0, x_ofs, y_ofs, shape->w, shape->h);
   destroy_bitmap(shape);
   
   for (y = 0; y < icon->size; y++) {
      f_y = (float)y * k;
      i_y = (int)floor(f_y);
      a = f_y - floor(f_y);
      for (x = 0; x < icon->size; x++) {
         f_x = (float)x * k;
	 i_x = (int)floor(f_x);
	 b = f_x - floor(f_x);
         red = green = blue = alpha = 0.0;
         for (m = -1; m < 3; m++) {
	    r1 = cubic_bspline((float)m - a);
            for (n = -1; n < 3; n++) {
               r2 = cubic_bspline(b - (float)n);
	       color = ((unsigned int *)(icon->workspace->line[i_y + m + 2]))[i_x + n + 2];
	       red += ((float)getr32(color) * r1 * r2);
	       green += ((float)getg32(color) * r1 * r2);
	       blue += ((float)getb32(color) * r1 * r2);
	       alpha += ((float)geta32(color) * r1 * r2);
            }
         }
	 color = makeacol32((int)floor(red), (int)floor(green), (int)floor(blue), 255 - (int)floor(alpha));
         ((unsigned int *)(icon->scaled->line[y]))[x] = color;
      }
   }
   
   return 0;
}



static int load_resource(char *datafile, char *name, ICON_DATA *icon)
{
   DATAFILE *data;
   BITMAP *temp, *bitmap = NULL;
   RLE_SPRITE *rle_sprite;
   PALETTE palette;
   int size, type, i;
   int result = 0;
   
   if (datafile[0] != '\0') {
      data = load_datafile_object(datafile, name);
      if (!data) {
         fprintf(stderr, "Error loading object '%s' from %s\n", name, datafile);
	 return -1;
      }
      switch (data->type) {
      
         case DAT_BITMAP:
	    temp = (BITMAP *)data->dat;
	    bitmap = create_bitmap_ex(temp->vtable->color_depth, temp->w, temp->h);
	    blit(temp, bitmap, 0, 0, 0, 0, temp->w, temp->h);
	    break;
	    
	 case DAT_RLE_SPRITE:
	    rle_sprite = (RLE_SPRITE *)data->dat;
	    bitmap = create_bitmap_ex(rle_sprite->color_depth, rle_sprite->w, rle_sprite->h);
	    clear_to_color(bitmap, bitmap->vtable->mask_color);
	    draw_rle_sprite(bitmap, rle_sprite, 0, 0);
	    break;
	    
	 case DAT_PALETTE:
	    select_palette((RGB *)data->dat);
	    unload_datafile_object(data);
	    return 0;
	 
	 default:
	    fprintf(stderr, "'%s' is not a BITMAP, RLE_SPRITE or PALETTE object in datafile '%s'\n", name, datafile);
	    unload_datafile_object(data);
	    return -1;
      }
      unload_datafile_object(data);
   }
   else {
      bitmap = load_bitmap(name, palette);
      select_palette(palette);
      if (!bitmap) {
         fprintf(stderr, "Unable to load '%s'\n", name);
	 return -1;
      }
   }
   
   if (!icon) {
      size = MAX(bitmap->w, bitmap->h);
      if (size <= 16)
         type = ICON_SMALL;
      else if (size <= 32)
         type = ICON_LARGE;
      else if (size <= 48)
         type = ICON_HUGE;
      else
         type = ICON_THUMBNAIL;
      icon = &icon_data[type];
      if (flags & icon->defined) {
         for (i = 0; i < 3; i++) {
	    type = (type + 1) % 4;
	    icon = &icon_data[type];
	    if (!(flags & icon->defined))
	       break;
	 }
	 if (flags & icon->defined) {
	    fprintf(stderr, "Too many icon resources!");
	    result = -1;
	    goto exit_error;
	 }
      }
   }
   else {
      if (icon->scaled) {
         fprintf(stderr, "Multiple icon resources of the same size");
	 result = -1;
	 goto exit_error;
      }
   }
   icon->original = create_bitmap_ex(bitmap->vtable->color_depth, bitmap->w, bitmap->h);
   blit(bitmap, icon->original, 0, 0, 0, 0, bitmap->w, bitmap->h);
   result = scale_icon(icon);
   flags |= icon->defined;
   
exit_error:
   destroy_bitmap(bitmap);
   
   return result;
}



static void usage(void)
{
   fprintf(stderr, "\nMacOS X application bundle fixer utility for Allegro " ALLEGRO_VERSION_STR "\n"
      "By Angelo Mottola, " ALLEGRO_DATE_STR "\n\n"
      "Usage: fixbundle exename [-m] [-o bundlename] [-v version] [-V long_version]\n"
      "\t\t[-e] [[-d datafile] [[palette] [-{16,32,48,128}] icon] ...]\n"
      "\twhere icon is either a datafile bitmap or a RLE sprite object, either\n"
      "\tan image file.\n"
      "Options:\n"
      "\t-m\t\tMoves executable inside bundle instead of copying it\n"
      "\t-o bundlename\tSpecifies a bundle name (default: exename.app)\n"
      "\t-v version\tSets application version string (default: 1.0)\n"
      "\t-V long_version\tSets long application version string\n"
      "\t-e\t\tEmbeds the Allegro framework into the application bundle\n"
      "\t-d datafile\tUses datafile as source for objects and palettes\n"
      "\t-{16,32,48,128}\tForces next icon image into the 16x16, 32x32, 48x48 or\n"
      "\t\t\t128x128 icon resource slot\n"
      "\n");
   exit(EXIT_FAILURE);
}



static int copy_file(const char *filename, const char *dest_path)
{
   char *buffer = NULL;
   char dest_file[1024];
   PACKFILE *f;
   size_t size;
   
   if (!exists(filename))
      return -1;
   buffer = malloc(size = file_size_ex(filename));
   if (!buffer)
      return -1;
   append_filename(dest_file, dest_path, get_filename(filename), 1024);
   f = pack_fopen(filename, F_READ);
   if (!f) {
      free(buffer);
      return -1;
   }
   pack_fread(buffer, size, f);
   pack_fclose(f);
   f = pack_fopen(dest_file, F_WRITE);
   if (!f) {
      free(buffer);
      return -1;
   }
   pack_fwrite(buffer, size, f);
   pack_fclose(f);
   free(buffer);
   
   return 0;
}



/* main:
 *  Guess what this function does.
 */
int main(int argc, char *argv[])
{
   PACKFILE *f;
   CFURLRef cf_url_ref;
   FSRef fs_ref;
   FSSpec fs_spec;
   IconFamilyHandle icon_family;
   Handle raw_data;
   char datafile[MAX_STRING_SIZE];
   char bundle[MAX_STRING_SIZE];
   char bundle_dir[MAX_STRING_SIZE];
   char bundle_contents_dir[MAX_STRING_SIZE];
   char bundle_contents_resources_dir[MAX_STRING_SIZE];
   char bundle_contents_macos_dir[MAX_STRING_SIZE];
   char bundle_contents_frameworks_dir[MAX_STRING_SIZE];
   char *bundle_exe = NULL;
   char bundle_plist[MAX_STRING_SIZE];
   char bundle_pkginfo[MAX_STRING_SIZE];
   char bundle_icns[MAX_STRING_SIZE];
   char bundle_version[MAX_STRING_SIZE];
   char bundle_long_version[MAX_STRING_SIZE];
   char *buffer = NULL;
   int arg, type = 0, result = 0;
   int i, size, x, y, mask_bit, mask_byte;
   unsigned char *data;
   
   install_allegro(SYSTEM_NONE, &errno, &atexit);
   set_color_depth(32);
   set_color_conversion(COLORCONV_TOTAL | COLORCONV_KEEP_TRANS);
   
   if (argc < 2)
      usage();
   
   datafile[0] = '\0';
   bundle[0] = '\0';
   select_palette(black_palette);
   
   /* Parse command line and load any given resource */
   for (arg = 2; arg < argc; arg++) {
      if (!strcmp(argv[arg], "-m"))
         flags |= F_MOVE;
      else if (!strcmp(argv[arg], "-e"))
         flags |= F_EMBED_FRAMEWORK;
      else if (!strcmp(argv[arg], "-o")) {
         if ((argc < arg + 2) || (bundle[0] != '\0'))
	    usage();
	 strcpy(bundle, argv[++arg]);
      }
      else if (!strcmp(argv[arg], "-v")) {
         if (argc < arg + 2)
	    usage();
	 flags |= F_GOT_VERSION;
	 strcpy(bundle_version, argv[++arg]);
      }
      else if (!strcmp(argv[arg], "-V")) {
         if (argc < arg + 2)
	    usage();
	 flags |= F_GOT_LONG_VERSION;
	 strcpy(bundle_long_version, argv[++arg]);
      }
      else if (!strcmp(argv[arg], "-d")) {
         if (argc < arg + 2)
	    usage();
	 strcpy(datafile, argv[++arg]);
      }
      else if ((!strcmp(argv[arg], "-16")) || (!strcmp(argv[arg], "-32")) ||
               (!strcmp(argv[arg], "-48")) || (!strcmp(argv[arg], "-128"))) {
         if (argc < arg + 2)
	    usage();
	 switch (atoi(&argv[arg][1])) {
	    case 16: type = 0; break;
	    case 32: type = 1; break;
	    case 48: type = 2; break;
	    case 128: type = 3; break;
	 }
	 if (load_resource(datafile, argv[++arg], &icon_data[type])) {
	    result = -1;
	    goto exit_error;
	 }
      }
      else {
         if (load_resource(datafile, argv[arg], NULL)) {
	    result = -1;
	    goto exit_error;
	 }
      }
   }
   
   buffer = malloc(4096);
   if (!buffer) {
      result = -1;
      goto exit_error_bundle;
   }
   
   bundle_exe = argv[1];
   if (!exists(bundle_exe)) {
      fprintf(stderr, "Cannot locate executable file '%s'\n", bundle_exe);
      result = -1;
      goto exit_error;
   }
   if (bundle[0] == '\0')
      strcpy(bundle, bundle_exe);
   replace_extension(bundle_dir, bundle, "app", MAX_STRING_SIZE);
   strcpy(bundle_contents_dir, bundle_dir);
   strcat(bundle_contents_dir, "/Contents");
   strcpy(bundle_contents_resources_dir, bundle_contents_dir);
   strcat(bundle_contents_resources_dir, "/Resources");
   strcpy(bundle_contents_macos_dir, bundle_contents_dir);
   strcat(bundle_contents_macos_dir, "/MacOS");
   strcpy(bundle_contents_frameworks_dir, bundle_contents_dir);
   strcat(bundle_contents_frameworks_dir, "/Frameworks");
   bundle_icns[0] = '\0';
   bundle_plist[0] = '\0';
   bundle_pkginfo[0] = '\0';
   
   /* Create bundle structure */
   if ((mkdir(bundle_dir, 0777) && (errno != EEXIST)) ||
       (mkdir(bundle_contents_dir, 0777) && (errno != EEXIST)) ||
       (mkdir(bundle_contents_resources_dir, 0777) && (errno != EEXIST)) ||
       (mkdir(bundle_contents_macos_dir, 0777) && (errno != EEXIST))) {
      fprintf(stderr, "Cannot create %s\n", bundle_dir);
      result = -1;
      goto exit_error_bundle;
   }
   
   /* Copy/move executable into the bundle */
   if (copy_file(bundle_exe, bundle_contents_macos_dir)) {
      fprintf(stderr, "Cannot create %s\n", bundle_contents_macos_dir);
      result = -1;
      goto exit_error_bundle;
   }
   strcat(bundle_contents_macos_dir, "/");
   strcat(bundle_contents_macos_dir, get_filename(bundle_exe));
   chmod(bundle_contents_macos_dir, 0755);
   if (flags & F_MOVE)
      unlink(bundle_exe);
   
   /* Embed Allegro framework if requested */
   if (flags & F_EMBED_FRAMEWORK) {
      if (!file_exists("/Library/Frameworks/Allegro.framework", FA_RDONLY | FA_DIREC, NULL)) {
         fprintf(stderr, "Cannot find Allegro framework\n");
	 result = -1;
	 goto exit_error_bundle;
      }
      if (!exists("/Library/Frameworks/Allegro.framework/Resources/Embeddable")) {
         fprintf(stderr, "Cannot embed system wide Allegro framework; install embeddable version first!\n");
	 result = -1;
	 goto exit_error_bundle;
      }
      sprintf(buffer, "/Developer/Tools/pbxcp -exclude .DS_Store -exclude CVS -resolve-src-symlinks /Library/Frameworks/Allegro.framework %s", bundle_contents_frameworks_dir);
      if ((mkdir(bundle_contents_frameworks_dir, 0777) && (errno != EEXIST)) ||
	  (system(buffer))) {
         fprintf(stderr, "Cannot create %s\n", bundle_contents_frameworks_dir);
	 result = -1;
	 goto exit_error_bundle;
      }
   }
   
   /* Setup the .icns resource */
   if (flags & F_ICONS_DEFINED) {
      strcat(bundle_contents_resources_dir, "/");
      strcat(bundle_contents_resources_dir, get_filename(bundle));
      replace_extension(bundle_icns, bundle_contents_resources_dir, "icns", MAX_STRING_SIZE);
      
      icon_family = (IconFamilyHandle)NewHandle(0);
      
      for (i = 0; i < 4; i++) {
         if (flags & icon_data[i].defined) {
	    /* Set 32bit RGBA data */
	        raw_data = NewHandle(icon_data[i].size * icon_data[i].size * 4);
	    data = *(unsigned char **)raw_data;
	    for (y = 0; y < icon_data[i].size; y++) {
	       for (x = 0; x < icon_data[i].size; x++) {
	          *data++ = geta32(((unsigned int *)(icon_data[i].scaled->line[y]))[x]);
	          *data++ = getr32(((unsigned int *)(icon_data[i].scaled->line[y]))[x]);
	          *data++ = getg32(((unsigned int *)(icon_data[i].scaled->line[y]))[x]);
	          *data++ = getb32(((unsigned int *)(icon_data[i].scaled->line[y]))[x]);
	       }
	    }
	    if (SetIconFamilyData(icon_family, icon_data[i].data, raw_data) != noErr) {
               DisposeHandle(raw_data);
	       fprintf(stderr, "Error setting %dx%d icon resource RGBA data\n", icon_data[i].size, icon_data[i].size);
	       result = -1;
	       goto exit_error_bundle;
	    }
	    DisposeHandle(raw_data);
	    /* Set 8bit mask */
            raw_data = NewHandle(icon_data[i].size * icon_data[i].size);
	    data = *(unsigned char **)raw_data;
	    for (y = 0; y < icon_data[i].size; y++) {
	       for (x = 0; x < icon_data[i].size; x++) {
	          *data++ = geta32(((unsigned int *)(icon_data[i].scaled->line[y]))[x]);
	       }
	    }
	    if (SetIconFamilyData(icon_family, icon_data[i].mask8, raw_data) != noErr) {
               DisposeHandle(raw_data);
	       fprintf(stderr, "Error setting %dx%d icon resource 8bit mask\n", icon_data[i].size, icon_data[i].size);
	       result = -1;
	       goto exit_error_bundle;
	    }
	    DisposeHandle(raw_data);
	    /* Set 1bit mask */
	    if (icon_data[i].mask1) {
	       size = ((icon_data[i].size * icon_data[i].size) + 7) / 8;
	       raw_data = NewHandle(size * 2);
	       data = *(unsigned char **)raw_data;
	       mask_byte = 0;
	       mask_bit = 7;
	       for (y = 0; y < icon_data[i].size; y++) {
	          for (x = 0; x < icon_data[i].size; x++) {
		     if (geta32(((unsigned int *)(icon_data[i].scaled->line[y]))[x]) >= 0xfd)
		        mask_byte |= (1 << mask_bit);
		     mask_bit--;
		     if (mask_bit < 0) {
		        *data++ = mask_byte;
			mask_byte = 0;
			mask_bit = 7;
		     }
		  }
	       }
	       memcpy(*raw_data + size, *raw_data, size);
               if (SetIconFamilyData(icon_family, icon_data[i].mask1, raw_data) != noErr) {
                  DisposeHandle(raw_data);
	          fprintf(stderr, "Error setting %dx%d icon resource 1bit mask\n", icon_data[i].size, icon_data[i].size);
	          result = -1;
	          goto exit_error_bundle;
	       }
	       DisposeHandle(raw_data);
	    }
	 }
      }

      f = pack_fopen(bundle_icns, F_WRITE);
      if (!f) {
         fprintf(stderr, "Cannot create %s\n", bundle_icns);
	 result = -1;
	 goto exit_error_bundle;
      }
      pack_fclose(f);
      
      cf_url_ref = CFURLCreateWithBytes(kCFAllocatorDefault, (unsigned char *)bundle_icns, strlen(bundle_icns), 0, NULL);
      if (!cf_url_ref) {
         fprintf(stderr, "Cannot create %s\n", bundle_icns);
	 result = -1;
	 goto exit_error_bundle;
      }
      CFURLGetFSRef(cf_url_ref, &fs_ref);
      CFRelease(cf_url_ref);
      if ((FSGetCatalogInfo(&fs_ref, kFSCatInfoNone, NULL, NULL, &fs_spec, NULL)) || 
          (WriteIconFile(icon_family, &fs_spec) != noErr)) {
         fprintf(stderr, "Cannot create %s\n", bundle_icns);
	 result = -1;
	 goto exit_error_bundle;
      }
      DisposeHandle((Handle)icon_family);
   }
   
   /* Setup Info.plist */
   sprintf(bundle_plist, "%s/Info.plist", bundle_contents_dir);
   f = pack_fopen(bundle_plist, F_WRITE);
   if (!f) {
      fprintf(stderr, "Cannot create %s\n", bundle_plist);
      result = -1;
      goto exit_error_bundle;
   }
   sprintf(buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
      "<plist version=\"1.0\">\n"
      "<dict>\n"
      "\t<key>CFBundleExecutable</key>\n"
      "\t<string>%s</string>\n"
      "\t<key>CFBundleInfoDictionaryVersion</key>\n"
      "\t<string>6.0</string>\n"
      "\t<key>CFBundlePackageType</key>\n"
      "\t<string>APPL</string>\n"
      "\t<key>CFBundleSignature</key>\n"
      "\t<string>%s</string>\n"
      "\t<key>CFBundleVersion</key>\n"
      "\t<string>%s</string>\n"
      "\t<key>CFBundleDocumentTypes</key>\n"
      "\t<array>\n"
      "\t\t<dict>\n"
      "\t\t\t<key>CFBundleTypeExtensions</key>\n"
      "\t\t\t<array>\n"
      "\t\t\t\t<string>*</string>\n"
      "\t\t\t</array>\n"
      "\t\t\t<key>CFBundleTypeName</key>\n"
      "\t\t\t<string>NSStringPboardType</string>\n"
      "\t\t\t<key>CFBundleTypeOSTypes</key>\n"
      "\t\t\t<array>\n"
      "\t\t\t\t<string>****</string>\n"
      "\t\t\t</array>\n"
      "\t\t\t<key>CFBundleTypeRole</key>\n"
      "\t\t\t<string>Viewer</string>\n"
      "\t\t</dict>\n"
      "\t</array>\n",
      get_filename(bundle_exe), "????", (flags & F_GOT_VERSION) ? bundle_version : "1.0");
   pack_fputs(buffer, f);
   if (flags & F_GOT_LONG_VERSION) {
      sprintf(buffer, "\t<key>CFBundleGetInfoString</key>\n"
         "\t<string>%s</string>\n", bundle_long_version);
      pack_fputs(buffer, f);
   }
   if (flags & F_ICONS_DEFINED) {
      sprintf(buffer, "\t<key>CFBundleIconFile</key>\n"
         "\t<string>%s</string>\n", get_filename(bundle_icns));
      pack_fputs(buffer, f);
   }
   pack_fputs("</dict>\n</plist>\n", f);
   pack_fclose(f);
   
   /* Setup PkgInfo */
   sprintf(bundle_pkginfo, "%s/PkgInfo", bundle_contents_dir);
   f = pack_fopen(bundle_pkginfo, F_WRITE);
   if (!f) {
      fprintf(stderr, "Cannot create %s\n", bundle_pkginfo);
      result = -1;
      goto exit_error_bundle;
   }
   pack_fputs("APPL????", f);
   pack_fclose(f);
   
exit_error:
   if (buffer)
      free(buffer);
   for (i = 0; i < 4; i++) {
      if (icon_data[i].original)
         destroy_bitmap(icon_data[i].original);
      if (icon_data[i].workspace)
         destroy_bitmap(icon_data[i].workspace);
      if (icon_data[i].scaled)
         destroy_bitmap(icon_data[i].scaled);
   }
   return result;

exit_error_bundle:
   sprintf(buffer, "%s/%s", bundle_contents_macos_dir, get_filename(bundle_exe));
   unlink(buffer);
   unlink(bundle_plist);
   unlink(bundle_pkginfo);
   unlink(bundle_icns);
   rmdir(bundle_dir);
   rmdir(bundle_contents_dir);
   rmdir(bundle_contents_resources_dir);
   rmdir(bundle_contents_macos_dir);
   goto exit_error;
}
