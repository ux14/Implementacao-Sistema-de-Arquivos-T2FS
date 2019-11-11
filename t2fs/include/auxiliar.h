#ifndef AUXILIAR_H
#define AUXILIAR_H

#include "apidisk.h"
#include "bitmap2.h"
#include "t2disk.h"
#include "t2fs.h"

#define MAX_OPEN_FILES 256
typedef struct t2fs_file
{
	int valid;
	unsigned int current_pointer;
	struct t2fs_inode inode;
} OPENFILE;

typedef struct t2fs_partition
{
	DWORD sector_start;
	DWORD sector_end;
	struct t2fs_superbloco sb;
	int mounted;
} PARTINFO;

extern OPENFILE root_dir = {0};
extern OPENFILE open_files[MAX_OPEN_FILES] = {0};
extern PARTINFO partition_atual = {0};

// leitura e escrita de setores de acordo com um tamanho do buffer e offset dentro do setor. 
// Retorna o número de bytes escritos se conseguiu, e negativo (-1) caso contrário
int read_sector2 (unsigned int sector, unsigned char *buffer, int size, int offset);
int write_sector2 (unsigned int sector, unsigned char *buffer, int size, int offset);

// le/escreve na/da estrutura de superbloco
int read_superbloco (int superbloco_sector, struct t2fs_superbloco *sb);
int write_superbloco (int superbloco_sector, struct t2fs_superbloco *sb);

// análogo às funções acima para blocos, assumindo que são da partição que começa no setor 'superbloco_sector'.
int read_block (PARTINFO *partition, unsigned int block, unsigned char *buffer, int size, int offset);
int write_block (PARTINFO *partition, unsigned int block, unsigned char *buffer, int size, int offset);

// análogas
int read_inode(PARTINFO *partition, int num_inode, struct t2fs_inode *inode);
int write_inode(PARTINFO *partition, int num_inode, struct t2fs_inode *inode);

// le/escreve entradas do diretório root
int read_entry(PARTINFO *partition, int num_entry, struct t2fs_record *ent);
int write_entry(PARTINFO *partition, int num_entry, struct t2fs_record *ent);

// Preenche a estrutura de superbloco da partição indicada.
int fill_superbloco (int sector_start, int sector_end, int sectors_per_block, struct t2fs_superbloco *sb);

// reseta os bitmaps para o formato padrão, com o de inodes vazio, e o de blocos
// vazio exceto os 1 + tamanhoDosBitmaps + 1 blocos iniciais(superbloco, bitmaps e 0-ésimo inode).
int reset_bitmaps(PARTINFO *partition);

// retorna o numero do novo bloco se deu certo, 0 se há falta de espaço e negativo caso contrário (erro)
int alloc_block(PARTINFO *partition);
int free_block(PARTINFO *partition, int block);

// retorna o numero do novo inode se deu certo, e negativo caso contrário (erro ou falta de espaço)
int alloc_inode(PARTINFO *partition);
int free_inode(PARTINFO *partition, int inode);

// devolve o bloco físico correspondente ao bloco lógico dado, de acordo com o inode.
// Exemplo: se block for menor que 2, devolve um dos ponteiros diretos do inode, senão
// procura nos ponteiros indiretos pelo bloco certo. Se der erro, retorna negativo.
int address_conversion(PARTINFO *partition, unsigned int block, struct t2fs_inode *inode);

// Aloca bloco e coloca no lugar certo no inode (ultimo possível), cuidando dos ponteiros indiretos.
// retorna bloco se sucesso, senão valor negativo.
int alloc_block_to_file(PARTINFO *partition, struct t2fs_inode *inode);

#endif