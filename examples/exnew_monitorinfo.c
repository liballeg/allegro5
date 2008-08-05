#include "allegro5/allegro5.h"
#include <stdio.h>

int main(void)
{
	int num_adapters;
	int i;

	al_init();

	num_adapters = al_get_num_video_adapters();

	printf("%d adapters found...\n", num_adapters);

	for (i = 0; i < num_adapters; i++) {
		ALLEGRO_MONITOR_INFO info;
		al_get_monitor_info(i, &info);
		if (i != 0) {
			int j;
			for (j = 0; j < 79; j++) {
				printf("-");
			}
			printf("\n");
		}
		printf("Adapter %d\n", i);
		printf("%d,%d\n", info.x1, info.y1);
		printf("\n\n\n\n\t\t%d,%d\n", info.x2, info.y2);
	}

	return 0;
}
END_OF_MAIN()

