
/*
*/
#include "../include/apidisk.h"
#include "../include/bitmap2.h"
#include "../include/t2disk.h"
#include "../include/t2fs.h"
#include "../include/auxiliar.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*-----------------------------------------------------------------------------
Função:	Informa a identificação dos desenvolvedores do T2FS.
-----------------------------------------------------------------------------*/
int identify2 (char *name, int size) {
	strncpy (name, "Augusto Dalcin Peiter - 287685\nErik Bardini da Rosa - 303693\nLeonardo Holtz de Oliveira - 287702", size);
	return 0;
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

	if( read_sector2(0, (BYTE *)&pinfo, 2*sizeof(DWORD), 8 + partition*32) != 2*sizeof(DWORD) )
		return -1;

	if( fill_superbloco(pinfo.sector_start, pinfo.sector_end, sectors_per_block, &(pinfo.sb) ) != 0 )
		return -1;

	if( write_superbloco(pinfo.sector_start, &(pinfo.sb) ) != 0 )
		return -1;

	if( reset_bitmaps(&pinfo) != 0 )
		return -1;

	struct t2fs_inode root_inode;
	root_inode.bytesFileSize = 0;
	root_inode.blocksFileSize = 0;
	root_inode.RefCounter = 1;

	if( write_inode(&pinfo, 0, &root_inode) != 0 )
		return -1;

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Monta a partição indicada por "partition" no diretório raiz
-----------------------------------------------------------------------------*/
int mount(int partition) {
	
	if(partition_atual.mounted)
		return -1;

	if(partition < 0 || partition > 3)
		return -1;

	if ( read_sector2(0, (BYTE *)&partition_atual, 2*sizeof(DWORD), 8 + partition*32) != 2*sizeof(DWORD))
		return -1;

	if( read_superbloco( partition_atual.sector_start, &(partition_atual.sb) ) != 0 )
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

	if( delete2(filename) != 0 )
		return -1;

	struct t2fs_inode root_inode;
	struct t2fs_inode file_inode;
	struct t2fs_record file_entry;
	int file_inode_num;

	if( (file_inode_num = alloc_inode(&partition_atual)) == -1)
		return -1;

	file_inode.blocksFileSize = 0;
	file_inode.bytesFileSize = 0;
	file_inode.RefCounter = 1;

	memset(file_entry.name,'\0', sizeof(file_entry.name));
	strncpy(file_entry.name, filename, sizeof(file_entry.name) - 1);
	file_entry.TypeVal = TYPEVAL_REGULAR;
	file_entry.inodeNumber = file_inode_num;

	int num_entry;
	struct t2fs_record entry;
	if( read_inode(&partition_atual, 0, &root_inode ) != 0 )
		return -1;

	for(num_entry=0; num_entry*sizeof(struct t2fs_record) < root_inode.bytesFileSize; num_entry++)
	{
		if( read_entry(&partition_atual, num_entry, &entry) != 0 )
			return -1;

		if( entry.TypeVal == TYPEVAL_INVALIDO )
		{
			if( write_entry(&partition_atual, num_entry, &file_entry) != 0 )
				return -1;

			if( write_inode(&partition_atual, file_inode_num, &file_inode) != 0 )
				return -1;

			break;
		}
	}

	if(num_entry*sizeof(struct t2fs_record) >= root_inode.bytesFileSize)
	{
		if( root_inode.bytesFileSize%(partition_atual.sb.blockSize*SECTOR_SIZE) == 0)
			if( alloc_block_to_file(&partition_atual, 0) < 0 )
				return -1;

		if( read_inode(&partition_atual, 0, &root_inode) != 0)
			return -1;

		root_inode.bytesFileSize += sizeof(struct t2fs_record);
		
		if( write_inode(&partition_atual, 0, &root_inode) != 0)
			return -1;

		if( write_entry(&partition_atual, num_entry, &file_entry) != 0)
			return -1;
	}

	return open2(filename);

}

/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um arquivo do disco. 
-----------------------------------------------------------------------------*/
int delete2 (char *filename) {

	if (!partition_atual.mounted)
		return -1;

	struct t2fs_inode root_inode;
	struct t2fs_inode file_inode;

	if( read_inode(&partition_atual, 0, &root_inode ) != 0 )
		return -1;

	int num_entry, i;
	struct t2fs_record entry;

	printf("root_inode.bytesFileSize: %d\n", root_inode.bytesFileSize);
	for(num_entry=0; num_entry*sizeof(struct t2fs_record) < root_inode.bytesFileSize; num_entry++)
	{
		if( read_entry(&partition_atual, num_entry, &entry) != 0 )
			return -1;

		if( entry.TypeVal == TYPEVAL_REGULAR )
		{
			if( strcmp(filename, entry.name) == 0 )
			{
				if( read_inode(&partition_atual, entry.inodeNumber, &file_inode) != 0)
					return -1;

				printf("refCounter: %d\n", file_inode.RefCounter);

				// previnir erro pelo contador de referência ser inteiro sem sinal
				if( (int) --file_inode.RefCounter > 0 )
				{
					if( write_inode(&partition_atual, entry.inodeNumber, &file_inode) != 0)
						return -1;
				}
				else
				{
					if( free_inode(&partition_atual, entry.inodeNumber) != 0)
						return -1;

					entry.TypeVal = TYPEVAL_INVALIDO;
					
					if( write_entry(&partition_atual, num_entry, &entry) != 0 )
						return -1;

					for(i=0;i<MAX_OPEN_FILES;i++)
						if( open_files[i].inode_num == entry.inodeNumber )
							open_files[i].valid = 0;
				}

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

	struct t2fs_inode root_inode;
	struct t2fs_inode link_inode;

	if( read_inode(&partition_atual, 0, &root_inode ) != 0 )
		return -1;

	int num_entry;
	struct t2fs_record entry;

	for(num_entry=0; num_entry*sizeof(struct t2fs_record) < root_inode.bytesFileSize; num_entry++)
	{
		if( read_entry(&partition_atual, num_entry, &entry) != 0 )
			return -1;

		if( entry.TypeVal == TYPEVAL_REGULAR )
		{
			if( strcmp(filename, entry.name) == 0 )
			{
			
				open_files[handle].valid = 1;
				open_files[handle].current_pointer = 0;
				open_files[handle].inode_num = entry.inodeNumber;

				return handle;
			}
		}
		else
		if( entry.TypeVal == TYPEVAL_LINK )
		{
			if( strcmp(filename, entry.name) == 0 )
			{
				if( read_inode(&partition_atual, entry.inodeNumber, &link_inode) != 0 )
					return -1;

				if( read_block(&partition_atual, link_inode.dataPtr[0], entry.name, sizeof(entry.name), 0) != sizeof(entry.name) )
					return -1;

				return open2(entry.name);
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
	
	struct t2fs_inode file_inode;
   	int pointer, bytes_lidos;

	if( read_inode(&partition_atual, open_files[handle].inode_num, &file_inode) != 0)
		return -1;
	if(pointer = address_conversion(&partition_atual, file_inode.blocksFileSize, &file_inode) == -1)
		return -1;
	bytes_lidos = read_sector2(partition_atual.sector_start, &buffer, size, pointer);
	open_files[handle].current_pointer = pointer + bytes_lidos;
	if(bytes_lidos <= size && bytes_lidos >= 0)
		return bytes_lidos;
	else	
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

	if( !root_dir.valid )
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

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (softlink)
-----------------------------------------------------------------------------*/
int sln2 (char *linkname, char *filename) {
	
	if (!partition_atual.mounted)
		return -1;
	
	struct t2fs_inode root_inode;
	struct t2fs_inode file_inode;
	struct t2fs_record file_entry;
	int file_inode_num;
	int new_block;

	if( (file_inode_num = alloc_inode(&partition_atual)) == -1)
		return -1;
	
	// Específico para softlinks
	file_inode.blocksFileSize = 1;
	file_inode.bytesFileSize = sizeof(filename);
	
	file_inode.RefCounter = 1;
	
	memset(file_entry.name,'\0', sizeof(file_entry.name));
	strncpy(file_entry.name, linkname, sizeof(file_entry.name) - 1);
	file_entry.TypeVal = TYPEVAL_LINK; // Específico para links
	file_entry.inodeNumber = file_inode_num;
	
	int num_entry;
	struct t2fs_record entry;
	
	// Inode do diretório recebido a partir da partição atual
	if( read_inode(&partition_atual, 0, &root_inode ) != 0 )
		return -1;

	// Para cada entrada no diretório raíz, verificar se uma delas é inválida para escrever em cima e interromper a busca
	for(num_entry=0; num_entry*sizeof(struct t2fs_record) < root_inode.bytesFileSize; num_entry++)
	{
		if( read_entry(&partition_atual, num_entry, &entry) != 0 )
			return -1;

		if( entry.TypeVal == TYPEVAL_INVALIDO )
		{
			if( write_entry(&partition_atual, num_entry, &file_entry) != 0 )
				return -1;

			if( write_inode(&partition_atual, file_inode_num, &file_inode) != 0 )
				return -1;

			break;
		}
	}
	
	if(num_entry*sizeof(struct t2fs_record) >= root_inode.bytesFileSize)
	{
		if( root_inode.bytesFileSize%(partition_atual.sb.blockSize*SECTOR_SIZE) == 0)
			if( alloc_block_to_file(&partition_atual, 0) < 0 )
				return -1;

		if( read_inode(&partition_atual, 0, &root_inode) != 0)
			return -1;

		root_inode.bytesFileSize += sizeof(struct t2fs_record);
		
		if( write_inode(&partition_atual, 0, &root_inode) != 0)
			return -1;

		if( write_entry(&partition_atual, num_entry, &file_entry) != 0)
			return -1;
	}
	
	if((new_block = alloc_block(&partition_atual)) == -1)
		return -1;
	
	int writen_bytes;
	
	writen_bytes = write_block(&partition_atual, new_block, filename, sizeof(filename), 0);
	
	// Se os bytes escritos nao forem do mesmo tamanho do arquivo a ser referenciado, então o link nao funcionará
	if(writen_bytes == -1 || writen_bytes != sizeof(filename))
		return -1
	
	// Terminei?
	
	return 0; 
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (hardlink)
-----------------------------------------------------------------------------*/
int hln2(char *linkname, char *filename) {
	
	if (!partition_atual.mounted)
		return -1;
	FILE2 old_handle, new_handle;
	char *buffer;
	if (old_handle = open2(&filename) == -1)
		return -1;
	
	struct t2fs_inode file_inode;
	
	if(read_inode(&partition_atual, open_files[old_handle].inode_num, &file_inode) != 0)
		return -1;
	if(read2(old_handle, &buffer, file_inode.bytesFileSize) < 0)
		return -1;
	if(new_handle = create2(&linkname) == -1)
		return -1;
	if(write2(new_handle, &buffer, file_inode.bytesFileSize) != file_inode.bytesFileSize)
		return -1;
	file_inode.RefCounter ++;
	
	return 0;
}



