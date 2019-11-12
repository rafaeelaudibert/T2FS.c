
/**
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "t2fs.h"
#include "t2disk.h"
#include "apidisk.h"

#define MBR_SECTOR 0
#define BOOL unsigned short int
#define TRUE 1
#define FALSE 0

typedef struct t2fs_superbloco SUPERBLOCK;
typedef struct t2fs_record RECORD;
typedef struct t2fs_inode I_NODE;

MBR mbr;
BOOL diskMounted = FALSE;
BOOL debug = TRUE;

// "Private" functions
int buildMBR();
int formatPartition(int partition, int sectors_per_block);
DWORD computeChecksum(SUPERBLOCK* superBlock);

/*-----------------------------------------------------------------------------
Função:	Informa a identificação dos desenvolvedores do T2FS.
-----------------------------------------------------------------------------*/
int identify2(char *name, int size)
{
	strncpy(name, "Ana Carolina Pagnoncelli - 00287714\nAugusto Zanella Bardini  - 00278083\nRafael Baldasso Audibert - 00287695", size);
	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Formata logicamente uma partição do disco virtual t2fs_disk.dat para o sistema de
		arquivos T2FS definido usando blocos de dados de tamanho
		corresponde a um múltiplo de setores dados por sectors_per_block.
-----------------------------------------------------------------------------*/
int format2(int partition, int sectors_per_block)
{
	if(buildMBR() != 0){
		printf("\tERROR: Failed reading MBR.\n");
		return -1;
	}

	if (partition >= (int)mbr.partitionQuantity) {
		printf("\tERROR: There is no partition %d in disk.\n", partition);
		return -1;
	}

	if (formatPartition(partition, sectors_per_block) != 0) {
		printf("ERROR: Failed formating partition %d\t", partition);
		return -1;
	}

	return 0;
}

int buildMBR(){

	int i = 0;

	if(read_sector(MBR_SECTOR, (BYTE*)&mbr) != 0){
		printf("\tERROR: Failed reading sector 0 (MBR).\n");
		return -1;
	}

	if(debug){
		printf("MBR Version: %d\n", (int)mbr.version);
		printf("MBR sectorSize: %d\n", (int)mbr.sectorSize);
		printf("MBR PartitionTableByteInit: %d\n", (int)mbr.partitionsTableByteInit);
		printf("MBR partitionQuantity: %d\n", (int)mbr.partitionQuantity);

		for(i = 0; i < MAX_PARTITION_NUMBER; i++){
			printf("PARTITION %d ---- %s\n", i, mbr.partitions[i].name);
			printf("First Sector %d\n", mbr.partitions[i].firstSector);
			printf("Last Sector %d\n", mbr.partitions[i].lastSector);
		}

		printf("		-----\n");
	}

	return 0;
}

int formatPartition(int partition_number, int sectors_per_block) {
	PARTITION partition = mbr.partitions[partition_number];
	SUPERBLOCK superBlock;

	int sectorQuantity = partition.lastSector - partition.firstSector;

	partitionSizeInBytes = sectorQuantity * SECTOR_SIZE;
	blockSizeInBytes = sectors_per_block * SECTOR_SIZE;
	blockQuantity = (partitionSizeInBytes) / blockSizeInBytes;

	printf("asdfasdf");
	printf("sectors per block %d\n", sectors_per_block);
	printf("broco %d\n", (sectors_per_block * SECTOR_SIZE));

	// Preenche super block
	strncpy(superBlock.id, "T2FS", 4);
	superBlock.version = (WORD)0x7E32;
	superBlock.superblockSize = (WORD)1;
	superBlock.freeBlocksBitmapSize = (WORD)blockQuantity / (blockSizeInBytes * 8);
	superBlock.freeInodeBitmapSize = (WORD)superBlock.freeBlocksBitmapSize;
	superBlock.inodeAreaSize = (WORD)ceil(blockQuantity * 0.1);
	superBlock.blockSize = (WORD)sectors_per_block;
	superBlock.diskSize = (DWORD)sectorQuantity / sectors_per_block;
	superBlock.Checksum = computeChecksum(&superBlock);

	BYTE *buffer = (BYTE *)calloc(0, sizeof(BYTE) * superBlock.blockSize * SECTOR_SIZE);
	memcpy(buffer, (BYTE*)&superBlock, sizeof(superBlock));

	printf("size -> %d\n", sizeof(BYTE) * superBlock.blockSize);
	printf("%.*s", sizeof(BYTE) * superBlock.blockSize, buffer);

	// Só irá funcionar quando blockSize estiver calculado
	if(write_sector(partition.firstSector, (BYTE*)buffer) != 0){
		printf("\tERROR: Failed writing superBlock for partition %d.\n", partition_number);
		return -1;
	}

	return 0;
}

DWORD computeChecksum(SUPERBLOCK *superBlock)
{
	DWORD value = 0, i = 0;

	for (i = 0; i < 20; i += 4)
	{
		value += *((DWORD *)(superBlock + i));
	}

	return !value;
}

/*-----------------------------------------------------------------------------
Função:	Monta a partição indicada por "partition" no diretório raiz
-----------------------------------------------------------------------------*/
int mount(int partition)
{
	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Desmonta a partição atualmente montada, liberando o ponto de montagem.
-----------------------------------------------------------------------------*/
int unmount(void)
{
	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um novo arquivo no disco e abrí-lo,
		sendo, nesse último aspecto, equivalente a função open2.
		No entanto, diferentemente da open2, se filename referenciar um
		arquivo já existente, o mesmo terá seu conteúdo removido e
		assumirá um tamanho de zero bytes.
-----------------------------------------------------------------------------*/
FILE2 create2(char *filename)
{
	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um arquivo do disco.
-----------------------------------------------------------------------------*/
int delete2(char *filename)
{
	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um arquivo existente no disco.
-----------------------------------------------------------------------------*/
FILE2 open2(char *filename)
{
	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um arquivo.
-----------------------------------------------------------------------------*/
int close2(FILE2 handle)
{
	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a leitura de uma certa quantidade
		de bytes (size) de um arquivo.
-----------------------------------------------------------------------------*/
int read2(FILE2 handle, char *buffer, int size)
{
	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a escrita de uma certa quantidade
		de bytes (size) de  um arquivo.
-----------------------------------------------------------------------------*/
int write2(FILE2 handle, char *buffer, int size)
{
	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um diretório existente no disco.
-----------------------------------------------------------------------------*/
int opendir2(void)
{
	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para ler as entradas de um diretório.
-----------------------------------------------------------------------------*/
int readdir2(DIRENT2 *dentry)
{
	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um diretório.
-----------------------------------------------------------------------------*/
int closedir2(void)
{
	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (softlink)
-----------------------------------------------------------------------------*/
int sln2(char *linkname, char *filename)
{
	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (hardlink)
-----------------------------------------------------------------------------*/
int hln2(char *linkname, char *filename)
{
	return -9;
}
