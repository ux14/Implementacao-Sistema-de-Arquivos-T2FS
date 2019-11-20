#include "../include/apidisk.h"
#include "../include/bitmap2.h"
#include "../include/t2disk.h"
#include "../include/t2fs.h"
#include "../include/auxiliar.h"
#include <assert.h>
#include <stdlib.h>

OPENFILE root_dir;
OPENFILE open_files[MAX_OPEN_FILES];
PARTINFO partition_atual;

int read_sector2 (unsigned int sector, unsigned char *buffer, int size, int offset)
{
	if( buffer == NULL )
		return -1;

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
	if( buffer == NULL )
		return -1;

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
	if( sb == NULL )
		return -1;

	if( read_sector2( (DWORD)superbloco_sector, (BYTE *)sb, sizeof(struct t2fs_superbloco), 0) == sizeof(struct t2fs_superbloco) )
		return 0;

	return -1;
}

int write_superbloco (int superbloco_sector, struct t2fs_superbloco *sb)
{
	if( sb == NULL )
		return -1;

	if( write_sector2( (DWORD)superbloco_sector, (BYTE *)sb, sizeof(struct t2fs_superbloco), 0) == sizeof(struct t2fs_superbloco) )
		return 0;
	
	return -1;
}

int read_block (PARTINFO *partition, unsigned int block, unsigned char *buffer, int size, int offset)
{
	if( partition == NULL || buffer == NULL )
		return -1;

	int sectors_per_block = partition->sb.blockSize;

	int sector_in_block, read_bytes = 0, aux;
	int block_ini = partition->sector_start + block*sectors_per_block;

	for(sector_in_block = offset/SECTOR_SIZE; sector_in_block < sectors_per_block && read_bytes < size; sector_in_block++)
	{
		aux = read_sector2(block_ini + sector_in_block, buffer + read_bytes, size - read_bytes, offset - SECTOR_SIZE*sector_in_block);
		if( aux < 0 )
			return -1;
		else
			read_bytes += aux;
	}

	return read_bytes;
}

int write_block (PARTINFO *partition, unsigned int block, unsigned char *buffer, int size, int offset)
{
	if( partition == NULL || buffer == NULL )
		return -1;

	int sectors_per_block = partition->sb.blockSize;

	int sector_in_block, writen_bytes = 0, aux;
	int block_ini = partition->sector_start + block*sectors_per_block;

	for(sector_in_block = offset/SECTOR_SIZE; sector_in_block < sectors_per_block && writen_bytes < size; sector_in_block++)
	{
		aux = write_sector2(block_ini + sector_in_block, buffer + writen_bytes, size - writen_bytes, offset - SECTOR_SIZE*sector_in_block);
		if( aux < 0 )
			return -1;
		else
			writen_bytes += aux;
	}

	return writen_bytes;
}

int read_inode(PARTINFO *partition, int num_inode, struct t2fs_inode *inode)
{
	if( partition == NULL || inode == NULL )
		return -1;

	int blockSz_in_bytes = SECTOR_SIZE * partition->sb.blockSize;
	
	int inode_ini = 1 + partition->sb.freeBlocksBitmapSize + partition->sb.freeInodeBitmapSize;
	
	int offset_in_bytes = num_inode*sizeof(struct t2fs_inode);

	int inode_block = inode_ini + offset_in_bytes/blockSz_in_bytes;

	int offset = offset_in_bytes % (blockSz_in_bytes);

	if ( read_block(partition, inode_block, (BYTE *) inode, sizeof(struct t2fs_inode), offset) != sizeof(struct t2fs_inode) )
		return -1;

	return 0;
}

int write_inode(PARTINFO *partition, int num_inode, struct t2fs_inode *inode)
{
	if( partition == NULL || inode == NULL )
		return -1;

	int blockSz_in_bytes = SECTOR_SIZE * partition->sb.blockSize;
	
	int inode_ini = 1 + partition->sb.freeBlocksBitmapSize + partition->sb.freeInodeBitmapSize;
	
	int offset_in_bytes = num_inode*sizeof(struct t2fs_inode);

	int inode_block = inode_ini + offset_in_bytes/blockSz_in_bytes;

	int offset = offset_in_bytes % (blockSz_in_bytes);

	if ( write_block(partition, inode_block, (BYTE *) inode, sizeof(struct t2fs_inode), offset) != sizeof(struct t2fs_inode) )
		return -1;

	return 0;
}

int read_entry(PARTINFO *partition, int num_entry, struct t2fs_record *ent)
{
	if( partition == NULL || ent == NULL )
		return -1;

	struct t2fs_inode root_inode;

	if( read_inode(partition, 0, &root_inode) != 0 )
		return -1;

	int blockSz_in_bytes = SECTOR_SIZE * partition->sb.blockSize;

	int logical_block = ( num_entry*sizeof(struct t2fs_record) )/blockSz_in_bytes;

	int physical_block = address_conversion(partition, logical_block, &root_inode);
	
	if(physical_block == -1)
		return -1;

	int offset_in_block = ( num_entry*sizeof(struct t2fs_record) )%blockSz_in_bytes;

	if( read_block(partition, physical_block, (BYTE *)ent, sizeof(struct t2fs_record), offset_in_block) != sizeof(struct t2fs_record) )
		return -1;

	return 0;
}

int write_entry(PARTINFO *partition, int num_entry, struct t2fs_record *ent)
{
	if( partition == NULL || ent == NULL )
		return -1;

	struct t2fs_inode root_inode;

	if( read_inode(partition, 0, &root_inode) != 0 )
		return -1;

	int blockSz_in_bytes = SECTOR_SIZE * partition->sb.blockSize;

	int logical_block = ( num_entry*sizeof(struct t2fs_record) )/blockSz_in_bytes;
	
	int physical_block = address_conversion(partition, logical_block, &root_inode);
	
	printf("physical block: %d\n", physical_block);

	if(physical_block == -1)
		return -1;

	int offset_in_block = ( num_entry*sizeof(struct t2fs_record) )%blockSz_in_bytes;

	if( write_block(partition, physical_block, (BYTE *)ent, sizeof(struct t2fs_record), offset_in_block) != sizeof(struct t2fs_record) )
		return -1;

	return 0;
}

int fill_superbloco (int sector_start, int sector_end, int sectors_per_block, struct t2fs_superbloco *sb)
{
	if( sb == NULL )
		return -1;

	WORD block_sz = SECTOR_SIZE * sectors_per_block;
	WORD partition_sz = (sector_end - sector_start + 1)/sectors_per_block;

	WORD indode_sz = (partition_sz + 10 - 1)/10;
	WORD bitmap_block_sz = (partition_sz + block_sz - 1)/block_sz;
	WORD bitmap_inode_sz = (indode_sz + block_sz - 1)/block_sz;

	if( 1 + indode_sz + bitmap_block_sz + bitmap_inode_sz >= partition_sz)
		return -1;

	sb->id[0] = 'T';
	sb->id[1] = '2';
	sb->id[2] = 'F';
	sb->id[3] = 'S';
	sb->version = 0x7E32;
	sb->superblockSize = 1;
	sb->freeBlocksBitmapSize = bitmap_block_sz;
	sb->freeInodeBitmapSize = bitmap_inode_sz;
	sb->inodeAreaSize = indode_sz;
	sb->blockSize = sectors_per_block;
	sb->diskSize = partition_sz;

	int i;
	sb->Checksum = 0;
	for(i=0; i<5; ++i)
		sb->Checksum += *((DWORD *)sb + i);
	sb->Checksum = ~sb->Checksum;

	return 0;
}

int reset_bitmaps(PARTINFO *partition)
{
	if( partition == NULL )
		return -1;

	openBitmap2( partition->sector_start );

	int blockSz_in_bytes = SECTOR_SIZE * partition->sb.blockSize;

	int bitmapBlockSz = partition->sb.freeBlocksBitmapSize;

	int bitmapInodeSz = partition->sb.freeInodeBitmapSize;

	int inodeAreaSz = partition->sb.inodeAreaSize;

	int i, res = 0;
	for(i=0; i < partition->sb.diskSize; i++)
		if(i < 1 + bitmapBlockSz + bitmapInodeSz + inodeAreaSz )
			res |= setBitmap2(BITMAP_DADOS,i,1);
		else
			res |= setBitmap2(BITMAP_DADOS,i,0);

	res |= setBitmap2(BITMAP_INODE,0,1);

	for(i=1; i < inodeAreaSz*blockSz_in_bytes/sizeof(struct t2fs_inode); i++)
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

int free_block(PARTINFO *partition, int block)
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

int free_inode(PARTINFO *partition, int inode)
{
	if( partition == NULL )
		return -1;

	openBitmap2( partition->sector_start );

	if( setBitmap2(BITMAP_INODE,inode,0) != 0 )
		return -1;

	int i,ret;
	struct t2fs_inode in;
	
	ret = read_inode(partition, inode, &in);
	if( ret != 0 )
		return -1;

	for(i=0;i<in.blocksFileSize;i++)
	{
		ret = free_block(partition, address_conversion(partition,i,&in) );
		if( ret != 0)
			return -1;
	}

	closeBitmap2();

	return 0;
}

int address_conversion(PARTINFO *partition, unsigned int block, struct t2fs_inode *inode)
{

	if( partition == NULL || inode == NULL || block < 0 )
		return -1;

	int pointerSz = sizeof(DWORD);

	int blockSz_in_bytes = SECTOR_SIZE * partition->sb.blockSize;

	int blockSz_in_pointers = blockSz_in_bytes/pointerSz;

	if( block < 2 )
		return inode->dataPtr[block];

	int pointer, offset, doubleIndBlock;
	
	if( block < 2 + blockSz_in_pointers )
	{
		offset = (block - 2)*pointerSz;
		if( read_block(partition, inode->singleIndPtr, (BYTE *)pointer, pointerSz, offset) == pointerSz )
			return pointer;
	}

	if ( block < 2 + blockSz_in_pointers + blockSz_in_pointers*blockSz_in_pointers )
	{
		offset = (block - blockSz_in_pointers - 2)/blockSz_in_pointers;
		if( read_block(partition, inode->doubleIndPtr, (BYTE *)pointer, pointerSz, offset) == pointerSz )
		{
			offset = (block - blockSz_in_pointers - 2)%blockSz_in_pointers;
			if( read_block(partition, pointer, (BYTE *)pointer, pointerSz, offset) == pointerSz )
				return pointer;
		}
	}

	return -1;
}

int alloc_block_to_file(PARTINFO *partition, int inode_num)
{
	if(partition == NULL)
		return -1;

	struct t2fs_inode file_inode;

	if( read_inode(partition, inode_num, &file_inode) != 0)
		return -1;

	assert(file_inode.bytesFileSize == file_inode.blocksFileSize*partition->sb.blockSize*SECTOR_SIZE);

	int pointerSz = sizeof(DWORD);

	int blockSz_in_bytes = SECTOR_SIZE * partition->sb.blockSize;

	int blockSz_in_pointers = blockSz_in_bytes/pointerSz;

	int pointer = alloc_block(partition);
	int block = file_inode.blocksFileSize++;
	int offset, pointer_sndInd;

	// se igual a zero faltou espaco
	if( pointer <= 0 )
		return -1;

	assert(block >= 0);

	if( block < 2 )
		file_inode.dataPtr[ block ] = pointer;
	else
	if( block < 2 + blockSz_in_pointers )
	{
		if(block == 2)
		{
			file_inode.singleIndPtr = pointer;
			pointer = alloc_block(partition);
			if( pointer <= 0 )
				return -1;
		}

		offset = (block - 2)*pointerSz;
		if( write_block(partition, file_inode.singleIndPtr, (BYTE *)pointer, pointerSz, offset) != pointerSz )
			return -1;
	}
	else
	if ( block < 2 + blockSz_in_pointers + blockSz_in_pointers*blockSz_in_pointers )
	{
		offset = (block - 2 - blockSz_in_pointers)/blockSz_in_pointers;
		if( (block - 2)%blockSz_in_pointers == 0 )
		{
			if( write_block(partition, file_inode.doubleIndPtr, (BYTE *)pointer, pointerSz, offset) != pointerSz )
				return -1;
			pointer_sndInd = pointer;
			pointer = alloc_block(partition);
			if( pointer <= 0 )
				return -1;
		}
		else
			if( read_block(partition, file_inode.doubleIndPtr, (BYTE *)pointer_sndInd, pointerSz, offset) != pointerSz )
				return -1;

		offset = (block - 2 - blockSz_in_pointers)%blockSz_in_pointers;
		if( write_block(partition, pointer_sndInd, (BYTE *)pointer, pointerSz, offset) != pointerSz )
				return -1;
	}

	if( write_inode(partition, inode_num, &file_inode) != 0)
		return -1;

	return 0;
}
