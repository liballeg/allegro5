/*
 *	Benchmark test program for JPGalleg
 *
 *  Version 2.6, by Angelo Mottola, 2000-2006
 *
 *	This program tests the speed of the JPGalleg loading functions.
 */

#include <string.h>
#include <allegro.h>
#include <jpgalleg.h>


static volatile int timer = 0;


static void
about(void)
{
	printf(JPGALLEG_VERSION_STRING "\n"
		"Benchmark utility\n\n"
		"usage:\n"
		"\ttest [-f file] [times] [-nommx]\n\n"
		"-f file\t\tThe JPG image file to test with. If this is not specified, the\n"
		"\t\tprogram will try to use \"jpgalleg.jpg\" from the current dir.\n"
		"times\t\tNumber of times the test is repeated. Defaults to 30.\n"
		"-nommx\t\tDisables MMX optimizations before doing the test.\n\n"
	);
	exit(EXIT_FAILURE);
}


static void
timer_handler(void)
{
	timer++;
}
END_OF_FUNCTION(timer_handler);


int
main(int argc, char **argv)
{
	BITMAP *bmp;
	PACKFILE *f;
	char *file = NULL, *times = NULL, *memory = NULL;
	int arg, i, n, start, end, size;

	allegro_init();
	install_keyboard();
	install_timer();
	
	jpgalleg_init();
	
	set_color_conversion(COLORCONV_NONE);
	
	for (arg = 1; arg < argc; arg++) {
		if (!strcmp(argv[arg], "-nommx"))
			cpu_capabilities &= ~CPU_MMX;
		else if (!strcmp(argv[arg], "-f"))
			file = argv[++arg];
		else if (!times)
			times = argv[arg];
		else
			about();
	}
	if (times)
		n = atoi(times);
	else
		n = 30;
	if (!file)
		file = "jpgalleg.jpg";
	bmp = load_jpg(file, NULL);
	if (!bmp) {
		printf("Cannot find %s!\n", file);
		return -1;
	}
	size = file_size(file);
	memory = (char *)malloc(size);
	if (!memory) {
		printf("Not enough memory!\n");
		return -1;
	}
	f = pack_fopen(file, F_READ);
	pack_fread(memory, size, f);
	pack_fclose(f);
	
	LOCK_FUNCTION(timer_handler);
	LOCK_VARIABLE(timer);
	
	install_int(timer_handler, 10);
	
	printf("Average timing for %d function calls:\n", n);
	start = timer;
	for (i = 0; i < n; i++)
		bmp = load_jpg(file, NULL);
	end = timer;
	printf("load_jpg: %f seconds\n", ((float)end - (float)start) / 1000.0 / (float)n);

	start = timer;
	for (i = 0; i < n; i++)
		bmp = load_memory_jpg(memory, size, NULL);
	end = timer;
	printf("load_memory_jpg: %f seconds\n", ((float)end - (float)start) / 1000.0 / (float)n);
	
	free(memory);
	return 0;
}
END_OF_MAIN()
