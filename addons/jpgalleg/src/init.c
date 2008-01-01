/*
 *         __   _____    ______   ______   ___    ___
 *        /\ \ /\  _ `\ /\  ___\ /\  _  \ /\_ \  /\_ \
 *        \ \ \\ \ \L\ \\ \ \__/ \ \ \L\ \\//\ \ \//\ \      __     __
 *      __ \ \ \\ \  __| \ \ \  __\ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\
 *     /\ \_\/ / \ \ \/   \ \ \L\ \\ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \
 *     \ \____//  \ \_\    \ \____/ \ \_\ \_\/\____\/\____\ \____\ \____ \
 *      \/____/    \/_/     \/___/   \/_/\/_/\/____/\/____/\/____/\/___L\ \
 *                                                                  /\____/
 *                                                                  \_/__/
 *
 *      Version 2.6, by Angelo Mottola, 2000-2006
 *
 *      Init function and datafile type registration.
 *
 *      See the readme.txt file for instructions on using this package in your
 *      own programs.
 *
 */


#include <internal.h>


/* load_datafile_jpg:
 *  Hook function for loading a JPEG object from a datafile. Returns the
 *  decoded JPG into a BITMAP or NULL on error.
 */
static void *
load_datafile_jpg(PACKFILE *f, long size)
{
	BITMAP *bmp;
	char *buffer;
	
	buffer = (char *)malloc(size);
	if (!buffer)
		return NULL;
	pack_fread(buffer, size, f);
	bmp = load_memory_jpg(buffer, size, NULL);
	free(buffer);
	
	return (void *)bmp;
}


/* destroy_datafile_jpg:
 *  Hook function for freeing memory of JPEG objects in a loaded datafile.
 */
static void
destroy_datafile_jpg(void *data)
{
	if (data)
		destroy_bitmap((BITMAP *)data);
}


/* jpgalleg_init:
 *  Initializes JPGalleg by registering the file format with the Allegro image
 *  handling and datafile subsystems.
 */
int
jpgalleg_init(void)
{
	register_datafile_object(DAT_JPEG, load_datafile_jpg, destroy_datafile_jpg);
	register_bitmap_file_type("jpg", load_jpg, save_jpg);
	jpgalleg_error = JPG_ERROR_NONE;
	
	return 0;
}
