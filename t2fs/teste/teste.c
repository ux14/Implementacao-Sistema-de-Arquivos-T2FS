#include "../include/apidisk.h"
#include "../include/bitmap2.h"
#include "../include/auxiliar.h"
#include "../include/help.h"
#include "../include/t2fs.h"
#include <stdio.h>

int main()
{
	format2(0,1);

	dump_superbloco(1);
	dump_sector(1);

	return 0;
}
