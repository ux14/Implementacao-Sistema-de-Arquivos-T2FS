#include "../include/apidisk.h"
#include "../include/bitmap2.h"
#include "../include/t2disk.h"
#include "../include/t2fs.h"
#include "../include/auxiliar.h"

OPENFILE root_dir;
OPENFILE open_files[MAX_OPEN_FILES];
PARTINFO partition_atual;

int read_sector2 (unsigned int sector, unsigned char *buffer, int size, int offset)
{
	char secbuf[SECTOR_SIZE];
	if( read_sector(sector, secbuf) != 0)
		return -1;

	if( offset < 0)
		offset = 0;

	int i;
	for(i=0; i < size && i + offset < SECTOR_SIZE; i++)
		buffer[i] = secbuf[i + offset];

	return i;
}

int write_sector2 (unsigned int sector, unsigned char *buffer, int size, int offset)
{
	char secbuf[SECTOR_SIZE];
	if( read_sector(sector, secbuf) != 0)
		return -1;

	if( offset < 0)
		offset = 0;

	int i;
	for(i=0; i < size && i + offset < SECTOR_SIZE; i++)
		secbuf[i + offset] = buffer[i];

	if( write_sector(sector,secbuf) != 0 )
		return -1;

	return i;
}


int read_superbloco (int superbloco_sector, struct t2fs_superbloco *sb)
{
	if( read_sector2( (DWORD)superbloco_sector, (BYTE *)sb, sizeof(struct t2fs_superbloco), 0) == sizeof(struct t2fs_superbloco) )
		return 0;
	else
		return -1;
}

int write_superbloco (int superbloco_sector, struct t2fs_superbloco *sb)
{
	if( write_sector2( (DWORD)superbloco_sector, (BYTE *)sb, sizeof(struct t2fs_superbloco), 0) == sizeof(struct t2fs_superbloco) )
		return 0;
	else
		return -1;
}

int read_block (PARTINFO *partition, unsigned int block, unsigned char *buffer, int size, int offset)
{
	if( partition == NULL )
		return -1;

	int sectors_per_block = partition->sb.blockSize;

	int sector_in_block, read_bytes, aux;
	int block_ini = partition->sector_start + block*sectors_per_block;

	for(sector_in_block = 0, read_bytes = 0; sector_in_block < sectors_per_block && read_bytes < size; sector_in_block++)
	{
		aux = read_sector2(block_ini + sector_in_block, buffer + read_bytes, size - read_bytes, offset - SECTOR_SIZE*i);
		if( aux < 0 )
			return -1;
		else
			read_bytes += aux;
	}

	return read_bytes;
}

int write_block (PARTINFO *partition, unsigned int block, unsigned char *buffer, int size, int offset);
{
	if( partition == NULL )
		return -1;

	int sectors_per_block = partition->sb.blockSize;

	int sector_in_block, writen_bytes, aux;
	int block_ini = partition->sector_start + block*sectors_per_block;

	for(sector_in_block = 0, writen_bytes = 0; sector_in_block < sectors_per_block && writen_bytes < size; sector_in_block++)
	{
		aux = write_sector2(block_ini + sector_in_block, buffer + writen_bytes, size - writen_bytes, offset - SECTOR_SIZE*i);
		if( aux < 0 )
			return -1;
		else
			writen_bytes += aux;
	}

	return writen_bytes;
}

int read_inode(PARTINFO *partition, int num_inode, struct t2fs_inode *inode)
{
	if( partition == NULL )
		return -1;

	int blockSz_in_bytes = SECTOR_SIZE * partition->sb.blockSize;
	
	int inode_ini = 1 + partition->sb.freeBlocksBitmapSize + partition->sb.freeInodeBitmapSize;
	
	int offset_in_bytes = num_inode*sizeof(struct t2fs_inode);

	int inode_block = inode_ini + offset_in_bytes/blockSz_in_bytes;

	int offset = offset_in_bytes % (blockSz_in_bytes);

	if ( read_block(partition, inode_block, (BYTE *) inode, sizeof(struct t2fs_inode), offset) == sizeof(struct t2fs_inode) )
		return 0;
	else
		return -1; 
}

int write_inode(PARTINFO *partition, int num_inode, struct t2fs_inode *inode);
{
	if( partition == NULL )
		return -1;

	int blockSz_in_bytes = SECTOR_SIZE * partition->sb.blockSize;
	
	int inode_ini = 1 + partition->sb.freeBlocksBitmapSize + partition->sb.freeInodeBitmapSize;
	
	int offset_in_bytes = num_inode*sizeof(struct t2fs_inode);

	int inode_block = inode_ini + offset_in_bytes/blockSz_in_bytes;

	int offset = offset_in_bytes % (blockSz_in_bytes);

	if ( write_block(partition, inode_block, (BYTE *) inode, sizeof(struct t2fs_inode), offset) == sizeof(struct t2fs_inode) )
		return 0;
	else
		return -1; 
}

int read_entry(PARTINFO *partition, int num_entry, struct t2fs_record *ent);

int write_entry(PARTINFO *partition, int num_entry, struct t2fs_record *ent);

int fill_superbloco (int sector_start, int sector_end, int sectors_per_block, struct t2fs_superbloco *sb);

int reset_bitmaps(PARTINFO *partition)
{
	if( partition == NULL )
		return -1;

	openBitmap2( partition->sector_start );

	int blockSz_in_bytes = SECTOR_SIZE * partition->sb.blockSize;

	int bitmapBlockSz = partition->sb.freeBlocksBitmapSize;

	int bitmapInodeSz = partition->sb.freeInodeBitmapSize;

	int i, res = 0;
	for(i=1; i<bitmapBlockSz * blockSz_in_bytes * 8; i++)
		if(i <= 1 + bitmapBlockSz + bitmapInodeSz)
			res |= setBitmap2(BITMAP_DADOS,i,1);
		else
			res |= setBitmap2(BITMAP_DADOS,i,0);

	for(i=1; i<bitmapInodeSz * blockSz_in_bytes * 8; i++)
		res |= setBitmap2(BITMAP_INODE,i,0);

	if(res != 0)
		return -1;

	closeBitmap2();

	return 0;
}

int alloc_block(PARTINFO *partition)
{
	if( partition == NULL )
		return -1;

	openBitmap2( partition->sector_start );

	int block = searchBitmap2(BITMAP_DADOS, 0);

	if( block <= 0 )
		return block;

	if( setBitmap2(BITMAP_DADOS,block,1) != 0 )
		return -1;

	closeBitmap2();

	return block;
}

int free_block(PARTINFO *partition, int block);
{
	if( partition == NULL )
		return -1;

	openBitmap2( partition->sector_start );

	if( setBitmap2(BITMAP_DADOS,block,0) != 0 )
		return -1;

	closeBitmap2();

	return 0;
}

int alloc_inode(PARTINFO *partition)
{
	if( partition == NULL )
		return -1;

	openBitmap2( partition->sector_start );

	int inode = searchBitmap2(BITMAP_INODE, 0);

	if( inode <= 0 )
		return inode;

	if( setBitmap2(BITMAP_INODE,inode,1) != 0 )
		return -1;

	closeBitmap2();

	return inode;
}

int free_inode(PARTINFO *partition, int inode);
{
	if( partition == NULL )
		return -1;

	openBitmap2( partition->sector_start );

	if( setBitmap2(BITMAP_INODE,inode,0) != 0 )
		return -1;

	closeBitmap2();

	return 0;
}

int address_conversion(unsigned int block, struct t2fs_inode *inode);