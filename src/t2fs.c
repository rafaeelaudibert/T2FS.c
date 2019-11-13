
/**
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

// Global variables
MBR mbr;
int mounted_partition = 0;

// Debug variables
BOOL debug = TRUE;

// "Private" functions
int buildMBR();
int formatPartition(int partition, int sectors_per_block);
DWORD computeChecksum(SUPERBLOCK *superBlock);

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
	if (buildMBR() != 0)
	{
		printf("ERROR: Failed reading MBR.\n");
		return -1;
	}

	if (partition >= (int)mbr.partitionQuantity)
	{
		printf("ERROR: There is no partition %d in disk.\n", partition);
		return -1;
	}

	if (formatPartition(partition, sectors_per_block) != 0)
	{
		printf("ERROR: Failed formating partition %d\n", partition);
		return -1;
	}

	return 0;
}

int buildMBR()
{

	int i = 0;

	if (read_sector(MBR_SECTOR, (BYTE *)&mbr) != 0)
	{
		printf("ERROR: Failed reading sector 0 (MBR).\n");
		return -1;
	}

	if (debug)
	{
		printf("MBR Version: %d\n", (int)mbr.version);
		printf("MBR sectorSize: %d\n", (int)mbr.sectorSize);
		printf("MBR PartitionTableByteInit: %d\n", (int)mbr.partitionsTableByteInit);
		printf("MBR partitionQuantity: %d\n", (int)mbr.partitionQuantity);

		for (i = 0; i < MAX_PARTITION_NUMBER; i++)
		{
			printf("PARTITION %d ---- %s\n", i, mbr.partitions[i].name);
			printf("First Sector %d\n", mbr.partitions[i].firstSector);
			printf("Last Sector %d\n", mbr.partitions[i].lastSector);
		}

		printf("		-----\n");
	}

	return 0;
}

int formatPartition(int partition_number, int sectors_per_block)
{
	PARTITION partition = mbr.partitions[partition_number];
	SUPERBLOCK sb;
	BYTE *buffer = (BYTE *)malloc(sizeof(BYTE) * SECTOR_SIZE);
	BYTE *zeroed_buffer = (BYTE *)calloc(0, sizeof(BYTE) * SECTOR_SIZE);

	// Calcula variáveis auxiliares
	DWORD sectorQuantity = partition.lastSector - partition.firstSector + 1;
	DWORD partitionSizeInBytes = sectorQuantity * SECTOR_SIZE;
	DWORD blockSizeInBytes = sectors_per_block * SECTOR_SIZE;
	DWORD blockQuantity = partitionSizeInBytes / blockSizeInBytes;
	DWORD inodeOccupiedBlocks = ceil(blockQuantity * 0.1);
	DWORD inodeOccupiedBytes = (inodeOccupiedBlocks * sectors_per_block * SECTOR_SIZE);
	DWORD inodeQuantity = ceil(inodeOccupiedBytes / 32.0);
	DWORD inodeBitmapSizeInBlocks = ceil(inodeQuantity / (8.0 * blockSizeInBytes));

	printf("sectorQuantity: %u\n", sectorQuantity);
	printf("partitionSizeInBytes: %u\n", partitionSizeInBytes);
	printf("blockSizeInBytes: %u\n", blockSizeInBytes);
	printf("blockQuantity: %u\n", blockQuantity);
	printf("inodeOccupiedBlocks: %u\n", inodeOccupiedBlocks);
	printf("inodeOccupiedBytes: %u\n", inodeOccupiedBytes);
	printf("inodeQuantity: %u\n", inodeQuantity);
	printf("inodeBitmapSizeInBlocks: %u\n", inodeBitmapSizeInBlocks);

	// Preenche super block
	strncpy(sb.id, "T2FS", 4);
	sb.version = (WORD)0x7E32;
	sb.superblockSize = (WORD)1;
	sb.freeBlocksBitmapSize = (WORD)inodeBitmapSizeInBlocks;
	sb.freeInodeBitmapSize = (WORD)inodeBitmapSizeInBlocks;
	sb.inodeAreaSize = (WORD)inodeOccupiedBlocks;
	sb.blockSize = (WORD)sectors_per_block;
	sb.diskSize = (DWORD)sectorQuantity / sectors_per_block;
	sb.Checksum = computeChecksum(&sb);

	printf("freeBlocksBitmapSize: %hd\n", sb.freeBlocksBitmapSize);
	printf("freeInodeBitmapSize: %hd\n", sb.freeInodeBitmapSize);
	printf("inodeAreaSize: %hd\n", sb.inodeAreaSize);
	printf("blockSize: %hd\n", sb.blockSize);
	printf("diskSize: %u\n", sb.diskSize);
	printf("Checksum: %u\n", sb.Checksum);

	// Fill buffer with superBlock
	memset(buffer, 0, sizeof(BYTE) * SECTOR_SIZE);
	memcpy(buffer, (BYTE *)(&sb), sizeof(sb));

	// Escreve superBlock no disco (os dados de verdade ocupam apenas o primeiro setor, os outros são zerados)
	if (write_sector(partition.firstSector, (BYTE *)buffer) != 0)
	{
		printf("ERROR: Failed writing main superBlock sector for partition %d.\n", partition_number);
		return -1;
	}
	for (DWORD sectorIdx = partition.firstSector + 1; sectorIdx < partition.firstSector + sb.blockSize; ++sectorIdx)
	{
		if (write_sector(sectorIdx, (BYTE *)zeroed_buffer) != 0)
		{
			printf("ERROR: Failed writing zeroed superBlock sector %d for partition %d.\n", sectorIdx, partition_number);
			return -1;
		}
		printf("Formatted zeroed superBlock sector %d for partition %d.\n", sectorIdx, partition_number);
	}

	// Criar/limpar bitmap dos blocos com o zeroed_buffer
	DWORD first_bbitmap = partition.firstSector + sb.superblockSize * sb.blockSize;
	DWORD last_bbitmap = first_bbitmap + sb.freeBlocksBitmapSize * sb.blockSize;
	for (DWORD sectorIdx = first_bbitmap; sectorIdx < last_bbitmap; ++sectorIdx)
	{
		if (write_sector(sectorIdx, (BYTE *)zeroed_buffer) != 0)
		{
			printf("ERROR: Failed writing block bitmap sector %d on partition %d while formatting it.\n", sectorIdx, partition_number);
			return -1;
		}
		printf("Formatted free block bitmap sector %d\n", sectorIdx);
	}

	// Criar/limpar bitmap dos inodes
	DWORD first_ibitmap = last_bbitmap;
	DWORD last_ibitmap = first_ibitmap + sb.freeInodeBitmapSize * sb.blockSize;
	for (DWORD sectorIdx = first_ibitmap; sectorIdx < last_ibitmap; ++sectorIdx)
	{
		if (write_sector(sectorIdx, (BYTE *)zeroed_buffer) != 0)
		{
			printf("ERROR: Failed writing inode bitmap sector %d on partition %d while formatting it.\n", sectorIdx, partition_number);
			return -1;
		}
		printf("Formatted free inode bitmap sector %d\n", sectorIdx);
	}

	// Lembrar de liberar memória utilizada pelos buffers
	free(buffer);
	free(zeroed_buffer);

	return 0;
}

DWORD computeChecksum(SUPERBLOCK *superBlock)
{
	DWORD value = 0, i = 0;

	for (i = 0; i < 20; i += 4)
	{
		value += *((DWORD *)(superBlock + i));
	}

	return ~value;
}

/*-----------------------------------------------------------------------------
Função:	Monta a partição indicada por "partition" no diretório raiz
-----------------------------------------------------------------------------*/
int mount(int partition)
{
	if (mounted_partition != -1)
	{
		printf("ERROR: There is already a mounted partition. Please unmount it first.\n");
		return -1;
	}

	mounted_partition = partition;
	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Desmonta a partição atualmente montada, liberando o ponto de montagem.
-----------------------------------------------------------------------------*/
int unmount(void)
{
	mounted_partition = -1;
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
