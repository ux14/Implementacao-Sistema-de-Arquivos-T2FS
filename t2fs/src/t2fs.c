
/**
*/
#include "../include/apidisk.h"
#include "../include/bitmap2.h"
#include "../include/t2disk.h"
#include "../include/t2fs.h"
#include "../include/auxiliar.h"
#include <string.h>

/*-----------------------------------------------------------------------------
Função:	Informa a identificação dos desenvolvedores do T2FS.
-----------------------------------------------------------------------------*/
int identify2 (char *name, int size) {
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Formata logicamente uma partição do disco virtual t2fs_disk.dat para o sistema de
		arquivos T2FS definido usando blocos de dados de tamanho 
		corresponde a um múltiplo de setores dados por sectors_per_block.
-----------------------------------------------------------------------------*/
int format2(int partition, int sectors_per_block) {

	if(partition < 0 || partition > 3)
		return -1;

	PARTINFO pinfo;

	int ret;

	ret = read_sector2(0, (BYTE *)&pinfo, 2*sizeof(DWORD), 8 + partition*32);
	if( ret != 2*sizeof(DWORD) )
		return -1;

	ret = fill_superbloco(pinfo.sector_start, pinfo.sector_end, sectors_per_block, &(pinfo.sb) );
	if( ret != 0 )
		return -1;

	ret = write_superbloco(pinfo.sector_start, &(pinfo.sb) );
	if( ret != 0 )
		return -1;

	ret = reset_bitmaps(&pinfo);
	if( ret != 0 )
		return -1;

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Monta a partição indicada por "partition" no diretório raiz
-----------------------------------------------------------------------------*/
int mount(int partition) {
	
	if(partition < 0 || partition > 3)
		return -1;

	if ( read_sector2(0, (BYTE *)&partition_atual, 2*sizeof(DWORD), 8 + partition*32) != 2*sizeof(DWORD))
		return -1;

	ret = read_superbloco( partition_atual.sector_start, &(partition_atual.sb) );
	if( ret != 0 )
		return -1;

	partition_atual.mounted = 1;

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Desmonta a partição atualmente montada, liberando o ponto de montagem.
-----------------------------------------------------------------------------*/
int unmount(void) {

	partition_atual.mounted = 0;

	// fechar todos os arquivos
	int i;
	for(i=0; i<MAX_OPEN_FILES; i++)
		open_files[i].valid = 0;

	root_dir.valid = 0;

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um novo arquivo no disco e abrí-lo,
		sendo, nesse último aspecto, equivalente a função open2.
		No entanto, diferentemente da open2, se filename referenciar um 
		arquivo já existente, o mesmo terá seu conteúdo removido e 
		assumirá um tamanho de zero bytes.
-----------------------------------------------------------------------------*/
FILE2 create2 (char *filename) {

	if (!partition_atual.mounted)
		return -1;

	int ret, handle;

	ret = delete2(filename);
	if( ret != -1 )
		return -1;

	int ret;
	struct t2fs_inode root_inode;
	struct t2fs_inode file_inode;
	file_inode.blocksFileSize = 0;
	file_inode.bytesFileSize = 0;
	file_inode.RefCounter = 0;

	if( read_inode(&partition_atual, 0, &root_inode ) != 0 )
		return -1;

	int num_entry;
	struct t2fs_record entry;

	for(num_entry=0; num_entry*sizeof(struct t2fs_record) < root_inode.bytesFileSize; num_entry++)
	{
		ret = read_entry(&partition_atual, num_entry, &entry);
		if( ret != 0 )
			return -1;

		if( entry.TypeVal == TYPEVAL_INVALIDO )
		{
			memset(entry.name,'\0', sizeof(entry.name));
			strncpy(entry.name, filename, sizeof(entry.name) - 1);
			entry.TypeVal = TYPEVAL_REGULAR;

			entry.inodeNumber = alloc_inode(&partition_atual);
			if(entry.inodeNumber == -1)
				return -1;

			ret = write_entry(&partition_atual, num_entry, &entry);
			if( ret != 0 )
			{
				free_inode(&partition_atual, entry.inodeNumber);
				return -1;
			}

			ret = write_inode(&partition_atual, entry.inodeNumber, &file_inode);
			if( ret != 0 )
			{
				free_inode(&partition_atual, entry.inodeNumber);
				return -1;
			}

			break;
		}
	}

	if(num_entry*sizeof(struct t2fs_record) < root_inode.bytesFileSize)
		return open2(filename);

	// socorro
	memset(entry.name,'\0', sizeof(entry.name));
	strncpy(entry.name, filename, sizeof(entry.name) - 1);
	entry.TypeVal = TYPEVAL_REGULAR;
	entry.inodeNumber = alloc_inode(&partition_atual);
	
	if(entry.inodeNumber == -1)
		return -1;

	if( (root_inode.bytesFileSize + 1)%(partition_atual.sb.blockSize*SECTOR_SIZE) == 0)
		if( alloc_block_to_file(&partition_atual, &root_inode) < 0 )
			return -1;
	
	root_inode.bytesFileSize += sizeof(struct t2fs_record);
	write_inode(&partition_atual, 0, &root_inode);
	write_entry(&partition_atual, num_entry, &entry);

	return open2(filename);

}

/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um arquivo do disco. 
-----------------------------------------------------------------------------*/
int delete2 (char *filename) {

	if (!partition_atual.mounted)
		return -1;

	int ret;
	struct t2fs_inode root_inode;
	struct t2fs_inode file_inode;

	if( read_inode(&partition_atual, 0, &root_inode ) != 0 )
		return -1;

	int num_entry;
	struct t2fs_record entry;

	for(num_entry=0; num_entry*sizeof(struct t2fs_record) < root_inode.bytesFileSize; num_entry++)
	{
		ret = read_entry(&partition_atual, num_entry, &entry);
		if( ret != 0 )
			return -1;

		if( entry.TypeVal == TYPEVAL_REGULAR )
		{
			if( strcmp(filename, entry.name) == 0 )
			{
				ret = free_inode(entry.inodeNumber);
				if( ret != 0)
					return -1;

				entry.TypeVal = TYPEVAL_INVALIDO;

				ret = write_entry(&partition_atual, num_entry, &entry);
				if( ret != 0 )
					return -1;

				break;
			}
		}
	}

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um arquivo existente no disco.
-----------------------------------------------------------------------------*/
FILE2 open2 (char *filename) {
	
	if (!partition_atual.mounted)
		return -1;

	int handle;
	for(handle=0; handle<MAX_OPEN_FILES; handle++)
		if( !open_files[handle].valid )
			break;

	if( handle == MAX_OPEN_FILES )
		return -1;

	int ret;
	struct t2fs_inode root_inode;

	if( read_inode(&partition_atual, 0, &root_inode ) != 0 )
		return -1;

	int num_entry;
	struct t2fs_record entry;

	for(num_entry=0; num_entry*sizeof(struct t2fs_record) < root_inode.bytesFileSize; num_entry++)
	{
		ret = read_entry(&partition_atual, num_entry, &entry);
		if( ret != 0 )
			return -1;

		if( entry.TypeVal == TYPEVAL_REGULAR )
		{
			if( strcmp(filename, entry.name) == 0 )
			{
				ret = read_inode(&partition_atual, entry.inodeNumber, &open_files[handle].inode);
				if( ret != 0 )
					return -1;
			
				open_files[handle].valid = 1;
				open_files[handle].current_pointer = 0;

				return handle;
			}
		}
	}

	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um arquivo.
-----------------------------------------------------------------------------*/
int close2 (FILE2 handle) {

	if (!partition_atual.mounted)
		return -1;

	if( handle < 0 || handle >= MAX_OPEN_FILES)
		return -1;

	open_files[handle].valid = 0;

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a leitura de uma certa quantidade
		de bytes (size) de um arquivo.
-----------------------------------------------------------------------------*/
int read2 (FILE2 handle, char *buffer, int size) {
	
	if (!partition_atual.mounted)
		return -1;

	if( handle < 0 || handle >= MAX_OPEN_FILES)
		return -1;

	if( !open_files[handle].valid )
		return -1;

	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a escrita de uma certa quantidade
		de bytes (size) de  um arquivo.
-----------------------------------------------------------------------------*/
int write2 (FILE2 handle, char *buffer, int size) {
	
	if (!partition_atual.mounted)
		return -1;

	if( handle < 0 || handle >= MAX_OPEN_FILES)
		return -1;

	if( !open_files[handle].valid )
		return -1;

	return -1;

}

/*-----------------------------------------------------------------------------
Função:	Função que abre um diretório existente no disco.
-----------------------------------------------------------------------------*/
int opendir2 (void) {

	if (!partition_atual.mounted)
		return -1;

	if( read_inode(&partition_atual, 0, &(root_dir.inode) ) != 0 )
		return -1;

	root_dir.current_pointer = 0;
	root_dir.valid = 1;

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para ler as entradas de um diretório.
-----------------------------------------------------------------------------*/
int readdir2 (DIRENT2 *dentry) {
		
	if (!partition_atual.mounted)
		return -1;

	if( !root_dir[handle].valid )
		return -1;

	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um diretório.
-----------------------------------------------------------------------------*/
int closedir2 (void) {
	
	if (!partition_atual.mounted)
		return -1;

	root_dir.valid = 0;

	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (softlink)
-----------------------------------------------------------------------------*/
int sln2 (char *linkname, char *filename) {
	
	if (!partition_atual.mounted)
		return -1;

	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (hardlink)
-----------------------------------------------------------------------------*/
int hln2(char *linkname, char *filename) {
	
	if (!partition_atual.mounted)
		return -1;

	return -1;
}



