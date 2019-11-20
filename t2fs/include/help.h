#ifndef HELP_H
#define HELP_H

#include "auxiliar.h"
#include "apidisk.h"
#include "bitmap2.h"
#include "t2disk.h"
#include "t2fs.h"

void dump_sector(unsigned int sector);
void dump_block(PARTITION *partition, int block);
void dump_file(PARTITION *partition, int inode_num);
void dump_inode(PARTITION *partition, int inode_num);
void dump_superbloco(int sector_start);
void dump_partition(PARTITION *partition);

#endif