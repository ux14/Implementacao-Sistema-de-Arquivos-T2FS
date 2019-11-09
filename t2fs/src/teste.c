#include "../include/apidisk.h"
#include "../include/bitmap2.h"
#include <stdio.h>

void dump(unsigned int sector)
{
	char buf[SECTOR_SIZE];

	printf("codigo de retorno: %d\n", read_sector(sector,buf) );

	int i, j, offset = 8;
	for(i=0; i<SECTOR_SIZE; i += offset)
	{
		printf("%x ", i);
		for(j=0; j<offset; j++)
			printf("%x ", buf[i + j]);
		printf("\n");
	}
}

int main()
{
	dump(0);

	return 0;
}