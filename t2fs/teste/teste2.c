#include "../include/apidisk.h"
#include "../include/bitmap2.h"
#include "../include/auxiliar.h"
#include "../include/help.h"
#include "../include/t2fs.h"
#include <stdio.h>

int main()
{
	PARTINFO *pp = &partition_atual;

	unmount();

	format2(3,1);

	printf("codigo mount(3): %d\n", mount(3));
	
	// Handle de arquivos
	FILE2 teste1, teste2, teste3, teste4, teste5, teste6, teste7, teste8, teste9, teste10, teste11;
	
	
	// Teste de criação de arquivos
	printf("Comeco de Testes de criacao:\n");
	
	// Teste 1
	printf("Teste 1: criacao de teste1.txt...\n");
	teste1 = create2("teste1.txt");
	if(teste1 != -1)
		printf("Arquivo criado com sucesso!\n");
	
	// Teste 2
	printf("Teste 2: criacao de teste2.txt...\n");
	teste2 = create2("teste2.txt");
	if(teste2 != -1)
		printf("Arquivo criado com sucesso!\n");
	
	// Teste 3
	printf("Teste 3: criacao de teste3.txt...\n");
	teste3 = create2("teste3.txt");
	if(teste3 != -1)
		printf("Arquivo criado com sucesso!\n");
	
	// Teste 4
	printf("Teste 4: criacao de teste4.txt...\n");
	teste4 = create2("teste4.txt");
	if(teste4 != -1)
		printf("Arquivo criado com sucesso!\n");
	
	// Teste 5
	printf("Teste 5: criacao de teste5.txt...\n");
	teste5 = create2("teste5.txt");
	if(teste5 != -1)
		printf("Arquivo criado com sucesso!\n");
	
	// Teste 6
	printf("Teste 6: criacao de teste6.txt...\n");
	teste6 = create2("teste6.txt");
	if(teste6 != -1)
		printf("Arquivo criado com sucesso!\n");
	
	// Teste 7
	printf("Teste 7: criacao de teste7.txt...\n");
	teste7 = create2("teste7.txt");
	if(teste7 != -1)
		printf("Arquivo criado com sucesso!\n");
	
	// Teste 8
	printf("Teste 8: criacao de teste8.txt...\n");
	teste8 = create2("teste8.txt");
	if(teste8 != -1)
		printf("Arquivo criado com sucesso!\n");
	
	// Teste 9
	printf("Teste 9: criacao de teste9.txt...\n");
	teste9 = create2("teste9.txt");
	if(teste9 != -1)
		printf("Arquivo criado com sucesso!\n");
	
	// Teste 10
	printf("Teste 10: criacao de teste10.txt...\n");
	teste10 = create2("teste10.txt");
	if(teste10 != -1)
		printf("Arquivo criado com sucesso!\n");
	
	// Teste 11 (especial: deve ser criado mas não pode ser aberto simultaneamente com os outros arquivos)
	/* É necessario fechar outro arquivo pois ficamos com duvida devido a documentação:
		Já que quando um arquivo é criado, ele também é aberto, poderiamos ficar na dúvida se o erro retornado pela
		função create2 seria devido a criação ou devido a abertura do arquivo*/
	if(close2(teste10) != -1)
		printf("Arquivo teste10 fechado para a criacao do teste11 (max de 10 arquivos abertos)\n");
	
	printf("Teste 11: criacao de teste11.txt...\n");
	teste11 = create2("teste11.txt");
	if(teste11 == -1)
		printf("Arquivo foi criado com sucesso\n");
	
	if(close2(teste11) != -1)
		printf("Arquivo teste11 fechado\n");
	
	teste10 = open2("teste10.txt");
	if(teste10 != -1)
		printf("Arquivo teste10 aberto novamente\n");
	
	teste11 = open2("teste11.txt");
	if(teste10 == -1)
		printf("Nao foi possivel abrir o arquivo teste11 pois foi atingido o maximo de arquivos abertos, OK!\n"); // Esse print deve passar caso alguém esteja se perguntando...
	

	// Teste de escrita nos arquivos
	printf("\n\nComeco dos testes de escrita dos arquivos:\n");
	printf("Lembrando que todos os arquivos de teste do 1 ao 10 ja estao abertos");
	
	// Teste 1
	printf("Escrevendo no arquivo teste1: Este eh o primeiro teste\n");
	if(write2(teste1, "Este eh o primeiro teste", sizeof("Este eh o primeiro teste")) != -1)
		printf("Escrita realizada com sucesso\n");
	
	if(close2(teste1) != -1)
		printf("Arquivo teste1 fechado\n");
	
	// Teste 2
	printf("Escrevendo no arquivo teste2 algo que cheguen o ponteiro de indirecao dupla\n");
	char bytes[256] = "asfsafasfasfasa";
	int i, j = 0;
	for(i=0; i<2080; i++ )
	{
		if(write2(teste2, bytes, sizeof(bytes)) != -1)
			j++;
	}
	if(j == 2079)
		printf("Escrita realizada com sucesso\n");
	else
		printf("Houve um erro na escrita\n");
	
	if(close2(teste2) != -1)
		printf("Arquivo teste2 fechado\n");
	
	// Teste de leitura dos arquivos
	printf("\n\nComeco dos testes de leitura dos arquivos:\n");
	
	char buffer_leitura[25];
	
	// Teste 1
	printf("Lendo do arquivo teste1:\n");
	open2("teste1.txt");
	if(read2(teste1, buffer_leitura, sizeof(buffer_leitura)) != -1)
	{
		printf("Leitura realizada com sucesso\n");
		printf("Resultado: %s", buffer_leitura);
	}
	if(close2(teste1) != -1)
		printf("Arquivo teste1 fechado\n");
	
	// Teste 2
	printf("\nLendo do arquivo teste2:\n");
	open2("teste2.txt");
	char bytes2[256];
	for(i=0; i<2079; i++ )
	{
		if(read2(teste1, bytes2, sizeof(bytes2)) != -1)
	}
	// teste feio emcima do ultimo bloco
	if(read2(teste1, bytes2, sizeof(bytes2)) != -1)
	{
		printf("Leitura realizada com sucesso\n");
		printf("Resultado: %s", bytes2);
	}
	if(close2(teste2) != -1)
		printf("Arquivo teste2 fechado\n");
	
	// Criação de Links
	printf("\nTestes para links\n");
	// Softlinks:
	printf("\nCriando link para teste1\n");
	if(sln2("link_teste1", "teste1.txt") != -1)
		printf("Link para teste1 criado com sucesso");
	
	teste1 = open2("link_teste1");
	if(teste1 != -1)
		printf("Arquivo teste1 aberto pelo link\n");
	if(read2(teste1, buffer_leitura, sizeof(buffer_leitura)) != -1)
	{
		printf("Leitura realizada com sucesso\n");
		printf("Resultado: %s", buffer_leitura);
	}
	
	
	return 0;
}
