#include "../include/apidisk.h"
#include "../include/bitmap2.h"
#include "../include/t2disk.h"
#include "../include/t2fs.h"
#include "../include/auxiliar.h"
#include "../include/help.h"
#include <stdlib.h>

void dump_sector(unsigned int sector)
{
	char buf[SECTOR_SIZE];

	printf("codigo de retorno para setor %d: %d\n", sector, read_sector(sector,buf) );

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

void dump_block(PARTINFO *partition, int block)
{
	if( partition != NULL )
	{
		int sectors_per_block = partition->sb.blockSize;

		int sector_in_block;
		int block_ini = partition->sector_start + block*sectors_per_block;

		printf("Bloco %d:\n", block);
		for(sector_in_block = 0; sector_in_block < sectors_per_block; sector_in_block++)
		{
			dump_sector(block_ini + sector_in_block);
		}
	}
}

void dump_file(PARTINFO *partition, int inode_num)
{
	int logical_block;
	struct t2fs_inode inode;
	if( read_inode(partition, inode_num, &inode) != 0 )
	{
		printf("Erro na leitura do inode\n");
		return;
	}

	for( logical_block = 0; logical_block < inode.blocksFileSize; logical_block++)
	{
		dump_block(partition, address_conversion(partition, logical_block, &inode) );
	}
}


void dump_inode(PARTINFO *partition, int inode_num)
{
	int logical_block;
	struct t2fs_inode inode;
	if( read_inode(partition, inode_num, &inode) != 0 )
	{
		printf("Erro na leitura do inode\n");
		return;
	}

	printf("%d blocos:\n", inode.blockFileSize);
	for( logical_block = 0; logical_block < inode.blocksFileSize; logical_block++)
	{
		printf("%d ", address_conversion(partition, logical_block, &inode) );
		if((logical_block + 1)%10 == 0)
			printf("\n");
	}
}

void dump_superbloco(int sector_start)
{
	struct t2fs_superbloco sb;

	printf("codigo de retorno:%d\n", read_superbloco(sector_start, &sb));
	printf("id = %c%c%c%c\n", sb.id[0], sb.id[1], sb.id[2], sb.id[3]);
	printf("versao = %x", sb.version & 0xFFFF);
	printf("Blocos ocupados pelo superbloco = %d\n", sb.superblockSize);
	printf("Blocos do Bitmap de blocos = %d\n", sb.freeBlocksBitmapSize);
	printf("Blocos do Bitmap de inodes = %d\n", sb.freeInodeBitmapSize);
	printf("Blocos de inodes = %d\n", sb.inodeAreaSize);
	printf("Setores por bloco = %d\n", sb.blockSize);
	printf("Nro de blocos total = %d\n", sb.diskSize);
	printf("Checksum = %d\n", sb.Checksum);
}

void dump_partition(PARTINFO *partition)
{
	printf("setor de inicio = %d\nsetor de fim = %d\n", partition->sector_start, partition->sector_end);
	printf("superbloco:\n")
	dump_superbloco(partition->sector_start);
}