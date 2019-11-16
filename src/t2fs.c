#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "t2fs.h"
#include "t2disk.h"
#include "apidisk.h"
#include "bitmap2.h"

#define MBR_SECTOR 0
#define BOOL unsigned short int
#define TRUE 1
#define FALSE 0

typedef struct t2fs_superbloco SUPERBLOCK;
typedef struct t2fs_record RECORD;
typedef struct t2fs_inode I_NODE;

// Global variables
MBR *mbr = NULL;
SUPERBLOCK *superblock = NULL;
int mounted_partition = -1;
BOOL rootOpened = FALSE;
DWORD rootFolderFileIndex = 0;

// "Private" functions
void initialize();
int readMBR();
int formatPartition(int, int);
int createRootFolder(int);
DWORD computeChecksum(SUPERBLOCK *);
DWORD getBlockBitmapFirstSector(PARTITION *, SUPERBLOCK *);
DWORD getBlockBitmapLastSector(PARTITION *, SUPERBLOCK *);
DWORD getInodeBitmapFirstSector(PARTITION *, SUPERBLOCK *);
DWORD getInodeBitmapLastSector(PARTITION *, SUPERBLOCK *);
DWORD getInodesFirstSector(PARTITION *, SUPERBLOCK *);
DWORD getDataBlocksFirstSector(PARTITION *, SUPERBLOCK *);
BOOL notMountedPartition();
BOOL notRootOpened();

// Debug variables
BOOL debug = TRUE;

/*-----------------------------------------------------------------------------
Função:	Informa a identificação dos desenvolvedores do T2FS.
-----------------------------------------------------------------------------*/
int identify2(char *name, int size)
{
	initialize();

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
	initialize();

	// TODO: Remove this line
	unmount(); // Unmount temporarily, for testing in the shell

	if (partition >= (int)mbr->partitionQuantity)
	{
		printf("ERROR: There is no partition %d in disk.\n", partition);
		return -1;
	}

	if (formatPartition(partition, sectors_per_block) != 0)
	{
		printf("ERROR: Failed formating partition %d\n", partition);
		return -1;
	}

	if (createRootFolder(partition) != 0)
	{
		printf("ERROR: Failed while creating root folder on partition %d\n", partition);
		return -1;
	}

	// TODO: Remove this line
	mount(partition); // Mount temporarily, for testing in the shell

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Monta a partição indicada por "partition" no diretório raiz
-----------------------------------------------------------------------------*/
int mount(int partition)
{
	if (superblock != NULL)
	{
		printf("ERROR: There is already a mounted partition. Please unmount it first.\n");
		return -1;
	}

	// Read superblock
	BYTE *buffer = (BYTE *)malloc(sizeof(BYTE) * SECTOR_SIZE);
	if (read_sector(mbr->partitions[partition].firstSector, (BYTE *)buffer) != 0)
	{
		printf("ERROR: Failed reading superblock.\n");
		return -1;
	}

	superblock = (SUPERBLOCK *)malloc(sizeof(SUPERBLOCK));
	memcpy(superblock, buffer, sizeof(SUPERBLOCK));

	// Mark mounted partition
	mounted_partition = partition;

	// Remember to clean up buffer allocated memory
	free(buffer);

	printf("Mounted partition %d successfuly.\n", partition);

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Desmonta a partição atualmente montada, liberando o ponto de montagem.
-----------------------------------------------------------------------------*/
int unmount(void)
{
	initialize();

	// Free dynamically allocated superblock memory and point it to null
	if (superblock != NULL)
	{
		free(superblock);
		superblock = NULL;
	}

	// Unmark mounted partition
	mounted_partition = -1;

	printf("Unmounted successfully.\n");

	return 0;
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
	initialize();
	if (notMountedPartition())
		return -1;
	if (notRootOpened())
		return -1;

	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um arquivo do disco.
-----------------------------------------------------------------------------*/
int delete2(char *filename)
{
	initialize();
	if (notMountedPartition())
		return -1;
	if (notRootOpened())
		return -1;

	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um arquivo existente no disco.
-----------------------------------------------------------------------------*/
FILE2 open2(char *filename)
{
	initialize();
	if (notMountedPartition())
		return -1;
	if (notRootOpened())
		return -1;

	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um arquivo.
-----------------------------------------------------------------------------*/
int close2(FILE2 handle)
{
	if (notMountedPartition())
		return -9;
	if (notRootOpened())
		return -1;

	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a leitura de uma certa quantidade
		de bytes (size) de um arquivo.
-----------------------------------------------------------------------------*/
int read2(FILE2 handle, char *buffer, int size)
{
	initialize();
	if (notMountedPartition())
		return -1;
	if (notRootOpened())
		return -1;
	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a escrita de uma certa quantidade
		de bytes (size) de  um arquivo.
-----------------------------------------------------------------------------*/
int write2(FILE2 handle, char *buffer, int size)
{
	initialize();
	if (notMountedPartition())
		return -1;
	if (notRootOpened())
		return -1;

	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um diretório existente no disco.
-----------------------------------------------------------------------------*/
//Se a opera��o foi realizada com sucesso,
//a fun��o deve posicionar o ponteiro de entradas (current entry) na primeira posi��o v�lida do diret�rio.

int opendir2(void)
{
	initialize();
	if (notMountedPartition())
		return -1;
	rootOpened = TRUE;
	rootFolderFileIndex = 0;

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para ler as entradas de um diretório.
-----------------------------------------------------------------------------*/
int readdir2(DIRENT2 *dentry)
{
	initialize();
	if (notMountedPartition())
		return -1;
	if (notRootOpened())
		return -1;

	DWORD direct_quantity = 2;
	DWORD simple_indirect_quantity = superblock->blockSize * SECTOR_SIZE / sizeof(DWORD);
	DWORD double_indirect_quantity = simple_indirect_quantity * (superblock->blockSize * SECTOR_SIZE / sizeof(DWORD));
	PARTITION partition = mbr->partitions[mounted_partition];
	I_NODE rootFolderInode;
	BYTE *buffer = (BYTE *)malloc(sizeof(BYTE) * SECTOR_SIZE);
	if ((read_sector(getInodesFirstSector(&partition, superblock), buffer)) != 0)
	{
		printf("ERROR: Couldn't read root folder inode.\n");
		return -1;
	}
	memcpy(&rootFolderInode, buffer, sizeof(I_NODE)); // Read first inode as it is hardcoded as the folder one

	// Check if we already finished reading the entries
	if (rootFolderFileIndex * sizeof(DWORD) >= rootFolderInode.bytesFileSize)
	{
		return -1;
	}

	// Get in which block and sector of the block we should look for
	DWORD block = (rootFolderFileIndex * sizeof(RECORD)) / (superblock->blockSize * SECTOR_SIZE);
	DWORD block_position = (rootFolderFileIndex * sizeof(RECORD)) % (superblock->blockSize * SECTOR_SIZE);
	DWORD sector = block_position / SECTOR_SIZE;
	DWORD sector_position = block_position % SECTOR_SIZE;

	if (block < direct_quantity)
	{
		// Read without indirection
		DWORD block_to_read = rootFolderInode.dataPtr[block] * superblock->blockSize;
		DWORD sector_to_read = block_to_read + sector;
		if ((read_sector(getDataBlocksFirstSector(&partition, superblock) + sector_to_read, buffer)) != 0)
		{
			printf("ERROR: Failed to read folder data sector.\n");
			return -1;
		}
		RECORD record;
		memcpy(&record, buffer + sector_position, sizeof(RECORD));

		memcpy(dentry->name, record.name, sizeof(BYTE) * 51);
		dentry->fileType = record.TypeVal;

		// Get file inode to find out file size
		I_NODE file_inode;
		DWORD inode_sector = (record.inodeNumber * sizeof(I_NODE)) / SECTOR_SIZE;
		DWORD inode_sector_position = (record.inodeNumber * sizeof(I_NODE)) % SECTOR_SIZE;
		if ((read_sector(getInodesFirstSector(&partition, superblock) + inode_sector, buffer)) != 0)
		{
			printf("ERROR: Couldn't read file inode.\n");
			return -1;
		}
		memcpy(&file_inode, buffer + inode_sector_position, sizeof(I_NODE));
		dentry->fileSize = file_inode.bytesFileSize;
	}
	else if (block < direct_quantity + simple_indirect_quantity)
	{
		// Read indirectly simple
		block -= direct_quantity; // To find out which block inside the indirection we should read

		// We need to find out what is the sector we should read, but for that we need to go through the indirection
		DWORD indirect_sector = (block * sizeof(DWORD)) / SECTOR_SIZE;
		DWORD indirect_sector_position = (block * sizeof(DWORD)) % SECTOR_SIZE;
		if ((read_sector(getDataBlocksFirstSector(&partition, superblock) + (rootFolderInode.singleIndPtr * superblock->blockSize) + indirect_sector, buffer)) != 0)
		{
			printf("ERROR: Couldn't read simple indirection data block.\n");
			return -1;
		}

		// Want this values
		DWORD block_to_read = *((DWORD *)(buffer + indirect_sector_position)) * superblock->blockSize;
		DWORD sector_to_read = block_to_read + sector;
		if ((read_sector(getDataBlocksFirstSector(&partition, superblock) + sector_to_read, buffer)) != 0)
		{
			printf("ERROR: Failed to read folder data sector.\n");
			return -1;
		}
		RECORD record;
		memcpy(&record, buffer + sector_position, sizeof(RECORD));

		memcpy(dentry->name, record.name, sizeof(BYTE) * 51);
		dentry->fileType = record.TypeVal;

		// Get file inode to find out file size
		I_NODE file_inode;
		DWORD inode_sector = (record.inodeNumber * sizeof(I_NODE)) / SECTOR_SIZE;
		DWORD inode_sector_position = (record.inodeNumber * sizeof(I_NODE)) % SECTOR_SIZE;
		if ((read_sector(getInodesFirstSector(&partition, superblock) + inode_sector, buffer)) != 0)
		{
			printf("ERROR: Couldn't read file inode.\n");
			return -1;
		}
		memcpy(&file_inode, buffer + inode_sector_position, sizeof(I_NODE));
		dentry->fileSize = file_inode.bytesFileSize;
	}
	else
	{
		block -= direct_quantity;
		block -= simple_indirect_quantity;

		// We need to find out what is the sector we should read, but for that we need to go through the indirection
		DWORD double_indirect_block = (block * sizeof(DWORD)) / ((superblock->blockSize * SECTOR_SIZE) / sizeof(DWORD));
		DWORD double_indirect_sector = double_indirect_block / SECTOR_SIZE;
		DWORD double_indirect_sector_position = double_indirect_block % SECTOR_SIZE;
		if ((read_sector(getDataBlocksFirstSector(&partition, superblock) + (rootFolderInode.doubleIndPtr * superblock->blockSize) + double_indirect_sector, buffer)) != 0)
		{
			printf("ERROR: Couldn't read double indirection first data block.\n");
			return -1;
		}
		DWORD double_indirection_block = *((DWORD *)(buffer + double_indirect_sector_position)) * superblock->blockSize;

		// ------------------------------- //

		// We need to find out what is the sector we should read, but for that we need to go through the indirection
		DWORD simple_indirect_block_position = (block * sizeof(DWORD)) % ((superblock->blockSize * SECTOR_SIZE) / sizeof(DWORD));
		DWORD simple_indirect_sector = simple_indirect_block_position / SECTOR_SIZE;
		DWORD simple_indirect_sector_position = simple_indirect_block_position % SECTOR_SIZE;
		if ((read_sector(getDataBlocksFirstSector(&partition, superblock) + double_indirection_block + simple_indirect_sector, buffer)) != 0)
		{
			printf("ERROR: Couldn't read double indirection double data block.\n");
			return -1;
		}

		// Want this values
		DWORD block_to_read = *((DWORD *)(buffer + simple_indirect_sector_position)) * superblock->blockSize;
		DWORD sector_to_read = block_to_read + sector;
		if ((read_sector(getDataBlocksFirstSector(&partition, superblock) + sector_to_read, buffer)) != 0)
		{
			printf("ERROR: Failed to read folder data sector.\n");
			return -1;
		}
		RECORD record;
		memcpy(&record, buffer + sector_position, sizeof(RECORD));

		memcpy(dentry->name, record.name, sizeof(BYTE) * 51);
		dentry->fileType = record.TypeVal;

		// Get file inode to find out file size
		I_NODE file_inode;
		DWORD inode_sector = (record.inodeNumber * sizeof(I_NODE)) / SECTOR_SIZE;
		DWORD inode_sector_position = (record.inodeNumber * sizeof(I_NODE)) % SECTOR_SIZE;
		if ((read_sector(getInodesFirstSector(&partition, superblock) + inode_sector, buffer)) != 0)
		{
			printf("ERROR: Couldn't read file inode.\n");
			return -1;
		}
		memcpy(&file_inode, buffer + inode_sector_position, sizeof(I_NODE));
		dentry->fileSize = file_inode.bytesFileSize;
	}

	// Update rootFolderFileIndex
	rootFolderFileIndex++;

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um diretório.
-----------------------------------------------------------------------------*/
int closedir2(void)
{
	initialize();
	if (notMountedPartition())
		return -1;
	rootOpened = FALSE;

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (softlink)
-----------------------------------------------------------------------------*/
int sln2(char *linkname, char *filename)
{
	initialize();
	if (notMountedPartition())
		return -1;
	if (notRootOpened())
		return -1;

	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (hardlink)
-----------------------------------------------------------------------------*/
int hln2(char *linkname, char *filename)
{
	initialize();
	if (notMountedPartition())
		return -1;
	if (notRootOpened())
		return -1;

	return -9;
}

/*












	HELPER FUNCTIONS


*/
void initialize()
{
	if (mbr == NULL)
	{
		readMBR();
	}
}

int readMBR()
{

	// Free MBR memory, if it is not null (shouldn't have called this function then)
	if (mbr != NULL)
	{
		printf("Freeing MBR memory. You shouldn't call this function if it has already been loaded.\n");
		free(mbr);
	}

	// Dynamically allocated MBR memory
	if ((mbr = (MBR *)malloc(sizeof(MBR))) == NULL)
	{
		printf("Couldn't allocate memory for MBR.");
		return -1;
	}

	// Read it from "disk"
	if (read_sector(MBR_SECTOR, (BYTE *)mbr) != 0)
	{
		printf("ERROR: Failed reading sector 0 (MBR).\n");
		return -1;
	}

	if (debug)
	{
		printf("MBR Version: %d\n", (int)mbr->version);
		printf("MBR sectorSize: %d\n", (int)mbr->sectorSize);
		printf("MBR PartitionTableByteInit: %d\n", (int)mbr->partitionsTableByteInit);
		printf("MBR partitionQuantity: %d\n", (int)mbr->partitionQuantity);

		for (int i = 0; i < MAX_PARTITION_NUMBER && i < mbr->partitionQuantity; i++)
		{
			printf("PARTITION %d ---- %s\n", i, mbr->partitions[i].name);
			printf("First Sector %d\n", mbr->partitions[i].firstSector);
			printf("Last Sector %d\n", mbr->partitions[i].lastSector);
		}

		printf("		-----\n");
	}

	return 0;
}

int formatPartition(int partition_number, int sectors_per_block)
{
	PARTITION partition = mbr->partitions[partition_number];
	SUPERBLOCK sb;
	BYTE *buffer = (BYTE *)calloc(SECTOR_SIZE, sizeof(BYTE));
	BYTE *zeroed_buffer = (BYTE *)calloc(SECTOR_SIZE, sizeof(BYTE));

	// Calcula variáveis auxiliares
	DWORD sectorQuantity = partition.lastSector - partition.firstSector + 1;
	DWORD partitionSizeInBytes = sectorQuantity * SECTOR_SIZE;
	DWORD blockSizeInBytes = sectors_per_block * SECTOR_SIZE;
	DWORD blockQuantity = partitionSizeInBytes / blockSizeInBytes;
	DWORD inodeOccupiedBlocks = ceil(blockQuantity * 0.1);
	DWORD inodeOccupiedBytes = (inodeOccupiedBlocks * sectors_per_block * SECTOR_SIZE);
	DWORD inodeQuantity = ceil(inodeOccupiedBytes / 32.0);
	DWORD inodeBitmapSizeInBlocks = ceil(inodeQuantity / 8.0 / blockSizeInBytes);

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
	memcpy(buffer, (BYTE *)(&sb), sizeof(sb));

	// Escreve superBlock no disco (os dados de verdade ocupam apenas o primeiro setor, os outros são zerados)
	if (write_sector(partition.firstSector, buffer) != 0)
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
	DWORD first_bbitmap = getBlockBitmapFirstSector(&partition, &sb);
	DWORD last_bbitmap = getBlockBitmapLastSector(&partition, &sb);
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
	DWORD first_ibitmap = getInodeBitmapFirstSector(&partition, &sb);
	DWORD last_ibitmap = getInodeBitmapLastSector(&partition, &sb);
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

// Compute cheksum for a given superBlock summing their first 20 bytes
// and complementing it values
inline DWORD computeChecksum(SUPERBLOCK *superBlock)
{
	DWORD value = 0, i = 0;

	for (i = 0; i < 20; i += 4)
	{
		value += *((DWORD *)(superBlock + i));
	}

	return ~value;
}

// Create root folder and configure it
int createRootFolder(int partition_number)
{
	PARTITION partition = mbr->partitions[partition_number];
	SUPERBLOCK sb;

	// Open this partition bitmap
	openBitmap2(partition.firstSector);
	if (searchBitmap2(BITMAP_INODE, 1) != -1)
	{
		printf("ERROR: There already exists a set bit on Inode bitmap. Please format this partition (%d) before trying to create root folder.\n", partition_number);
		return -1;
	}

	// Read superblock of the partition to sb
	BYTE *buffer = (BYTE *)malloc(sizeof(BYTE) * SECTOR_SIZE);
	if (read_sector(partition.firstSector, (BYTE *)buffer) != 0)
	{
		printf("ERROR: Failed reading superblock of partition %d\n", partition_number);
		return -1;
	}
	memcpy(&sb, buffer, sizeof(sb));

	// Create inode and mark it on the bitmap, automatically pointing to the first entry in the data block
	BYTE *inode_buffer = (BYTE *)calloc(SECTOR_SIZE, sizeof(BYTE));
	I_NODE inode = {(DWORD)1, (DWORD)0, {(DWORD)0, (DWORD)0}, (DWORD)0, (DWORD)0, (DWORD)1, (DWORD)0};
	memcpy(inode_buffer, &inode, sizeof(inode));
	if (write_sector(getInodesFirstSector(&partition, &sb), inode_buffer) != 0)
	{
		printf("ERROR: Couldn't write root folder inode.\n");
		return -1;
	};
	printf("DEBUG: Wrote root folder inode sector on %d sector\n", getInodesFirstSector(&partition, &sb));
	memset(inode_buffer, 0, sizeof(BYTE) * SECTOR_SIZE);
	for (int i = 1; i < sb.blockSize; ++i)
	{
		if (write_sector(getInodesFirstSector(&partition, &sb) + i, inode_buffer) != 0)
		{
			printf("ERROR: Couldn't write root folder inode.\n");
			return -1;
		}
		printf("DEBUG: Wrote extra root folder inode sector %d\n", getInodesFirstSector(&partition, &sb) + i);
	}
	if (setBitmap2(BITMAP_INODE, 0, 1) != 0)
	{
		printf("ERROR: Failed setting bitmap for root folder inode.\n");
		return -1;
	};
	printf("Set inode bitmap for root folder.\n");

	// Create folder data block, emptied
	printf("Will write root folder first data block on sector %d.\n", getDataBlocksFirstSector(&partition, &sb));
	BYTE *data_buffer = (BYTE *)calloc(SECTOR_SIZE, sizeof(BYTE));
	for (int i = 0; i < sb.blockSize; ++i)
	{
		if (write_sector(getDataBlocksFirstSector(&partition, &sb) + i, data_buffer) != 0)
		{
			printf("ERROR: Couldn't write root folder data block.\n");
			return -1;
		}
		printf("DEBUG: Wrote root folder data on sector %d\n", getDataBlocksFirstSector(&partition, &sb) + i);
	}
	if (setBitmap2(BITMAP_DADOS, 0, 1) != 0)
	{
		printf("ERROR: Failed setting bitmap for root folder data block.\n");
		setBitmap2(BITMAP_INODE, 0, 0); // Revert changed bitmap value
		return -1;
	};
	printf("Set data bitmap for root folder.\n");

	// Remember to close bitmap
	closeBitmap2();

	// Remember to free dynamically allocated memory
	free(inode_buffer);
	free(data_buffer);

	return 0;
}

/*
	Get bitmap sectors
*/
inline DWORD getBlockBitmapFirstSector(PARTITION *p, SUPERBLOCK *sb)
{
	return p->firstSector + sb->superblockSize * sb->blockSize;
}

inline DWORD getBlockBitmapLastSector(PARTITION *p, SUPERBLOCK *sb)
{
	return getBlockBitmapFirstSector(p, sb) + sb->freeBlocksBitmapSize * sb->blockSize;
}

inline DWORD getInodeBitmapFirstSector(PARTITION *p, SUPERBLOCK *sb)
{
	return getBlockBitmapLastSector(p, sb);
}

inline DWORD getInodeBitmapLastSector(PARTITION *p, SUPERBLOCK *sb)
{
	return getInodeBitmapFirstSector(p, sb) + sb->freeInodeBitmapSize * sb->blockSize;
}

inline DWORD getInodesFirstSector(PARTITION *p, SUPERBLOCK *sb)
{
	return getInodeBitmapLastSector(p, sb);
}

inline DWORD getDataBlocksFirstSector(PARTITION *p, SUPERBLOCK *sb)
{
	return getInodesFirstSector(p, sb) + sb->inodeAreaSize * sb->blockSize;
}

/*
	Helper functions when initializing the library
*/
inline BOOL notMountedPartition()
{
	if (superblock == NULL)
	{
		printf("ERROR: There is no mounted partition. Please mount it first.\n");
		return TRUE;
	}

	return FALSE;
}

inline BOOL notRootOpened()
{
	if (!rootOpened)
	{
		printf("ERROR: You must open the root directory before running this function.");
		return TRUE;
	}

	return FALSE;
}