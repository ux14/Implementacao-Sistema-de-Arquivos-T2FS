#include "../include/apidisk.h"
#include "../include/bitmap2.h"
#include <stdio.h>

void dump(unsigned int sector)
{
	char buf[SECTOR_SIZE];

	printf("codigo de retorno: %d\n", read_sector(sector,buf) );

	int i, j, offset = 16;
	for(i=0; i<256; i += offset)
	{
		printf("%08x - ", sector*SECTOR_SIZE + i);
		for(j=0; j<offset; j++)
			printf("%02x ", buf[i + j] & 0xFF);
		printf("- ");
		for(j=0; j<offset; j++)
			if( buf[i + j] >= 0x20 && buf[i + j] != 0xEF)
				printf("%c", buf[i + j]);
			else
				printf(".");

		printf("\n");
	}
}

int main()
{
	dump(0);

	return 0;
}