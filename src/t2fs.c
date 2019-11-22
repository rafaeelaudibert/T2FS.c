#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "t2fs.h"
#include "t2disk.h"
#include "apidisk.h"
#include "bitmap2.h"
#include "t2fslib.h"

/*-----------------------------------------------------------------------------
Função:	Informa a identificação dos desenvolvedores do T2FS.
-----------------------------------------------------------------------------*/
int identify2(char *name, int size)
{
	initialize();

	BYTE identification[] = "Ana Carolina Pagnoncelli - 00287714\nAugusto Zanella Bardini  - 00278083\nRafael Baldasso Audibert - 00287695";
	memcpy(name, identification, size);

	mount(0);

	// TODO: Remove this
	// format2(0, 2);
	// create2("arq1");
	// for (int i = 0; i < 4; i++) {
	// 	writeFile((FILE2)0, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa_____", 256);
	// 	//writeFile((FILE2)0, "augustolindo", 10);
	// }
	//   // TODO: Remove this
	//
	// for (int i = 0; i < 128; i++) {
	// 	//writeFile((FILE2)0, "augustolindo", 10);
	// 	writeFile((FILE2)0, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb_____", 256);
	// }
	//
	// for (int i = 0; i < 128; i++) {
	// 	//writeFile((FILE2)0, "augustolindo", 10);
	// 	writeFile((FILE2)0, "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc_____", 256);
	// }
	//
	// for (int i = 0; i < 128; i++) {
	// 	//writeFile((FILE2)0, "augustolindo", 10);
	// 	writeFile((FILE2)0, "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd_____", 256);
	// }
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

	if (partition >= (int)(getMBR()->partitionQuantity))
	{
		printf("ERROR: ERROR: There is no partition %d in disk.\n", partition);
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
	initialize();

	if (getSuperblock() != NULL)
	{
		printf("ERROR: There is already a mounted partition. Please unmount it first.\n");
		return -1;
	}

	// Configure mounting
	if (configureMountedPartition(partition) != 0)
	{
		printf("ERROR: Error while mounting partition.\n");
		return -1;
	};

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
	if (unmountPartition() != 0)
	{
		printf("ERROR: Couldn't unmount partition.\n");
		return -1;
	}

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

	// TODO: Remove this line later on
	opendir2();

	if (!isPartitionMounted() || !isRootOpened() || strlen(filename) > 50)
		return -1;

	// Configure bitmap
	openBitmap2(getPartition()->firstSector);

	I_NODE *dirInode = getInode(0);
	RECORD record;

	// Remove old file with same name
	int filesQuantity = dirInode->bytesFileSize / RECORD_SIZE;
	for (int i = 0; i < filesQuantity; i++)
	{
		getRecordByNumber(i, &record);
		if ((strcmp(record.name, filename) == 0) && (record.TypeVal != TYPEVAL_INVALIDO))
		{
			if (delete2(filename) != 0)
			{
				printf("ERROR: There was an error while trying to override a file with the same name.\n");
				return -1;
			};
			break;
		}
	}

	// Fetch and set bitmaps info
	int inodeNumber = searchBitmap2(BITMAP_INODE, 0);
	int blockNum = searchBitmap2(BITMAP_DADOS, 0);
	if (inodeNumber == -1)
	{
		printf("ERROR: ERROR: There is no space left to create a new inode.\n");
		return -1;
	}
	if (blockNum == -1)
	{
		printf("ERROR: ERROR: There is no space left to allocate a new block.\n");
		return -1;
	}
	setBitmap2(BITMAP_INODE, inodeNumber, 1);
	setBitmap2(BITMAP_DADOS, blockNum, 1);

	// Copy information to the new record
	strcpy(record.name, filename);
	record.TypeVal = TYPEVAL_REGULAR;
	record.inodeNumber = inodeNumber;

	// Compute where we will save the new record
	DWORD newRecordBlock = dirInode->bytesFileSize / getBlocksize();
	DWORD newRecordSector = dirInode->bytesFileSize % getBlocksize() / SECTOR_SIZE;
	DWORD newRecordSectorOffset = dirInode->bytesFileSize % SECTOR_SIZE;

	// Save it
	BYTE *record_buffer = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
	if (readDataBlockSector(newRecordBlock, newRecordSector, dirInode, (BYTE *)record_buffer) != 0)
	{
		printf("ERROR: Failed reading record\n");
		return -1;
	}
	memcpy(record_buffer + newRecordSectorOffset, &record, sizeof(RECORD));
	if (writeDataBlockSector(newRecordBlock, newRecordSector, dirInode, (BYTE *)record_buffer) != 0)
	{
		printf("ERROR: Failed writing record\n");
		return -1;
	}

	// Compute where we will save the inode
	int inodeSector = inodeNumber / (SECTOR_SIZE / sizeof(I_NODE));
	int inodeSectorOffset = (inodeNumber % (SECTOR_SIZE / sizeof(I_NODE))) * sizeof(I_NODE);

	// Create and save inode
	I_NODE inode = {(DWORD)1, (DWORD)0, {blockNum, (DWORD)0}, (DWORD)0, (DWORD)0, (DWORD)1, (DWORD)0};
	BYTE *buffer_inode = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
	if (read_sector(getInodesFirstSector(getPartition(), getSuperblock()) + inodeSector, buffer_inode) != 0)
	{
		printf("ERROR: Failed reading record\n");
		return -1;
	}
	memcpy(buffer_inode + inodeSectorOffset, &inode, sizeof(I_NODE));
	if (write_sector(getInodesFirstSector(getPartition(), getSuperblock()) + inodeSector, buffer_inode) != 0)
	{
		printf("ERROR: Failed writing record\n");
		return -1;
	}

	// We need to update the direntry inode
	dirInode->bytesFileSize += sizeof(RECORD);

	// Need to create a new block for the directory
	if (dirInode->bytesFileSize % getBlocksize() == 0)
	{
		printf("DEBUG: Will allocate new block for dir entries.\n");
		dirInode->blocksFileSize++;

		int newBlock = searchBitmap2(BITMAP_DADOS, 0);
		if (newBlock == -1)
		{
			printf("ERROR: There is no space left to create a new directory entry.\n");
			return -1;
		}
		setBitmap2(BITMAP_DADOS, newBlock, 1);

		DWORD direct_quantity = getInodeDirectQuantity();
		DWORD simple_indirect_quantity = getInodeSimpleIndirectQuantity();
		DWORD double_indirect_quantity = getInodeDoubleIndirectQuantity();

		if (dirInode->blocksFileSize == direct_quantity)
		{
			// Second block
			dirInode->dataPtr[1] = newBlock;
		}
		else if (dirInode->blocksFileSize == direct_quantity + 1)
		{
			// Allocate block for the simple indirection block

			// Find bitmap entry
			int newSimpleIndirectionBlock = searchBitmap2(BITMAP_DADOS, 0);
			if (newSimpleIndirectionBlock == -1)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}
			if (setBitmap2(BITMAP_DADOS, newSimpleIndirectionBlock, 1) != 0)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}
			dirInode->singleIndPtr = newSimpleIndirectionBlock;

			BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			memcpy(simple_ind_buffer, &newBlock, sizeof(newBlock));
			if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->singleIndPtr * getSuperblock()->blockSize, simple_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
		}
		else if (dirInode->blocksFileSize <= direct_quantity + simple_indirect_quantity)
		{
			// Middle single indirection block

			BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			if (read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->singleIndPtr * getSuperblock()->blockSize, simple_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
			memcpy(simple_ind_buffer + (dirInode->blocksFileSize - direct_quantity - 1) * sizeof(newBlock), &newBlock, sizeof(newBlock));
			if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->singleIndPtr * getSuperblock()->blockSize, simple_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
		}
		else if (dirInode->blocksFileSize == direct_quantity + simple_indirect_quantity + 1) // TODO: Need to allocate blocks for the double indirection
		{
			// Allocate bitmap for doubleIndirectionBlock
			int newDoubleIndirectionBlock = searchBitmap2(BITMAP_DADOS, 0);
			if (newDoubleIndirectionBlock == -1)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}
			if (setBitmap2(BITMAP_DADOS, newDoubleIndirectionBlock, 1) != 0)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}
			dirInode->doubleIndPtr = newDoubleIndirectionBlock;

			// Allocate bitmap for simpleIndirectionBlock
			int newSimpleIndirectionBlock = searchBitmap2(BITMAP_DADOS, 0);
			if (newSimpleIndirectionBlock == -1)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}
			if (setBitmap2(BITMAP_DADOS, newSimpleIndirectionBlock, 1) != 0)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}

			BYTE *double_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			memcpy(double_ind_buffer, &newSimpleIndirectionBlock, sizeof(newSimpleIndirectionBlock));
			if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->doubleIndPtr * getSuperblock()->blockSize, double_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}

			BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			memcpy(simple_ind_buffer, &newBlock, sizeof(newBlock));
			if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + newSimpleIndirectionBlock * getSuperblock()->blockSize, simple_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
		}
		else if ((dirInode->blocksFileSize - 3) % simple_indirect_quantity == 0)
		{
			// Allocate bitmap for simpleIndirectionBlock
			int newSimpleIndirectionBlock = searchBitmap2(BITMAP_DADOS, 0);
			if (newSimpleIndirectionBlock == -1)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}
			if (setBitmap2(BITMAP_DADOS, newSimpleIndirectionBlock, 1) != 0)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}

			BYTE *double_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			if (read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->doubleIndPtr * getSuperblock()->blockSize, double_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
			memcpy(double_ind_buffer + (dirInode->blocksFileSize - direct_quantity - simple_indirect_quantity - 1) / simple_indirect_quantity * sizeof(newSimpleIndirectionBlock), &newSimpleIndirectionBlock, sizeof(newSimpleIndirectionBlock));
			if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->doubleIndPtr * getSuperblock()->blockSize, double_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}

			BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			memcpy(simple_ind_buffer, &newBlock, sizeof(newBlock));
			if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + newSimpleIndirectionBlock * getSuperblock()->blockSize, simple_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
		}
		else
		{
			// Discover where is the simpleIndBlock
			BYTE *double_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			if (read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->doubleIndPtr * getSuperblock()->blockSize, double_ind_buffer))
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
			DWORD simple_ind_ptr = *((DWORD *)(double_ind_buffer + (dirInode->blocksFileSize - direct_quantity - simple_indirect_quantity - 1) / simple_indirect_quantity * sizeof(DWORD)));

			BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			if (read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + simple_ind_ptr, simple_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
			memcpy(simple_ind_buffer + (dirInode->blocksFileSize - direct_quantity - simple_indirect_quantity - 1) % simple_indirect_quantity * sizeof(newBlock), &newBlock, sizeof(newBlock));
			if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + simple_ind_ptr, simple_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
		}
	}

	BYTE *dir_inode_buffer = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
	if (read_sector(getInodesFirstSector(getPartition(), getSuperblock()), dir_inode_buffer) != 0)
	{
		printf("ERROR: There was an error while trying to create a new directory entry.\n");
		return -1;
	}
	memcpy(dir_inode_buffer, dirInode, sizeof(I_NODE));
	if (write_sector(getInodesFirstSector(getPartition(), getSuperblock()), dir_inode_buffer) != 0)
	{
		printf("ERROR: There was an error while trying to create a new directory entry.\n");
		return -1;
	}

	// Free dynamically allocated memory
	free(record_buffer);
	free(buffer_inode);
	free(dirInode);

	// Remember to close the opened bitmap
	closeBitmap2();

	// Return a handler to this file
	return open2(filename);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um arquivo do disco.
-----------------------------------------------------------------------------*/
int delete2(char *filename)
{
	initialize();

	// TODO: Remove this line later on
	opendir2();

	if (!isPartitionMounted() || !isRootOpened())
		return -1;

	// Configure bitmap
	openBitmap2(getPartition()->firstSector);

	RECORD *record = NULL;
	I_NODE *dirInode = getInode(0);
	RECORD *recordAux = (RECORD *)malloc(sizeof(RECORD));
	DWORD bytesFileSizeUntilRecord = 0;

	// Search for the record
	int filesQuantity = dirInode->bytesFileSize / RECORD_SIZE;
	for (int i = 0; i < filesQuantity; i++)
	{
		getRecordByNumber(i, recordAux);
		if ((strcmp(recordAux->name, filename) == 0) && (recordAux->TypeVal != TYPEVAL_INVALIDO))
		{
			record = recordAux;
			break;
		}

		bytesFileSizeUntilRecord = RECORD_SIZE + bytesFileSizeUntilRecord;
	}

	//Test if the record was found
	if (record == NULL)
	{
		printf("ERROR: There is no file with name %s.\n", filename);
		return -1;
	}

	//update the record to invalid
	record->TypeVal = TYPEVAL_INVALIDO;

	// Compute where is the record
	DWORD recordBlock = bytesFileSizeUntilRecord / getBlocksize();
	DWORD recordSector = bytesFileSizeUntilRecord % getBlocksize() / SECTOR_SIZE;
	DWORD recordSectorOffset = bytesFileSizeUntilRecord % SECTOR_SIZE;

	// Save it
	BYTE *record_buffer = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
	if (readDataBlockSector(recordBlock, recordSector, dirInode, (BYTE *)record_buffer) != 0)
	{
		printf("ERROR: Failed reading record\n");
		return -1;
	}

	memcpy((BYTE *)record_buffer + recordSectorOffset, (BYTE *)record, sizeof(RECORD));
	if (writeDataBlockSector(recordBlock, recordSector, dirInode, (BYTE *)record_buffer) != 0)
	{
		printf("ERROR: Failed writing record\n");
		return -1;
	}

	//get the inode of the record
	I_NODE *inode = getInode(record->inodeNumber);

	//updates RefCounter and test if exists any hardlink.
	inode->RefCounter = inode->RefCounter - 1;
	if (inode->RefCounter > 0)
	{
		// Compute where the inode is
		DWORD inodeSector = record->inodeNumber / (SECTOR_SIZE / sizeof(I_NODE));
		DWORD inodeSectorOffset = (record->inodeNumber % (SECTOR_SIZE / sizeof(I_NODE))) * sizeof(I_NODE);

		// Update and save inode
		BYTE *buffer_inode = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
		if (read_sector(getInodesFirstSector(getPartition(), getSuperblock()) + inodeSector, buffer_inode) != 0)
		{
			printf("ERROR: Failed reading record\n");
			return -1;
		}
		memcpy(buffer_inode + inodeSectorOffset, inode, sizeof(I_NODE));
		if (write_sector(getInodesFirstSector(getPartition(), getSuperblock()) + inodeSector, buffer_inode) != 0)
		{
			printf("ERROR: Failed writing record\n");
			return -1;
		}

		printf("The file was successfuly removed.\n");
		return 0;
	}

	//If there is no link to the file anymore, clear the pointers
	clearPointers(inode);

	//Clear the inode bitmap
	setBitmap2(BITMAP_INODE, record->inodeNumber, 0);

	// Free dynamically allocated memory
	free(record_buffer);
	free(dirInode);
	free(recordAux);

	// Remember to close the opened bitmap
	closeBitmap2();

	printf("The file was successfuly removed.\n");
	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um arquivo existente no disco.
-----------------------------------------------------------------------------*/
FILE2 open2(char *filename)
{
	//TODO remove this
	opendir2();

	initialize();
	if (!isPartitionMounted() || !isRootOpened())
		return -1;

	if (countOpenedFiles() == MAX_OPEN_FILES)
	{
		printf("There is no more handlers available to open a file.\n");
		return -1;
	}

	RECORD *record = (RECORD *)malloc(sizeof(RECORD));
	if (getRecordByName(filename, record) != 0)
	{
		printf("Couldn't find file with name %s.\n", filename);
		return -1;
	}

	// Get the handler
	FILE2 handler = openFile(record);

	// If it is a link, open recursively
	if (record->TypeVal == TYPEVAL_LINK)
	{
		BYTE *link_filename = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
		I_NODE *link_inode = getInode(record->inodeNumber);
		if (read2(handler, link_filename, link_inode->bytesFileSize) != link_inode->bytesFileSize)
		{
			printf("ERROR: Error while trying to open a link to another file.\n");
			return -1;
		};

		// Close this file
		close2(handler);

		// And try to open the other file
		FILE2 link_handler = open2(link_filename);

		// Free dynamically allocated buffer
		free(link_filename);

		return link_handler;
	}

	// Else, return the handler acquired by now
	return handler;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um arquivo.
-----------------------------------------------------------------------------*/
int close2(FILE2 handle)
{
	//TODO remove
	opendir2();

	if (!isPartitionMounted() || !isRootOpened())
		return -1;

	return closeFile(handle);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a leitura de uma certa quantidade
		de bytes (size) de um arquivo.
-----------------------------------------------------------------------------*/
int read2(FILE2 handle, char *buffer, int size)
{
	initialize();
	if (!isPartitionMounted() || !isRootOpened())
		return -1;

	int bytesRead = readFile(handle, buffer, size);
	return bytesRead;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a escrita de uma certa quantidade
		de bytes (size) de  um arquivo.
-----------------------------------------------------------------------------*/
int write2(FILE2 handle, char *buffer, int size)
{
	// TODO: Remove this line later on
	opendir2();

	initialize();

	if (!isPartitionMounted() || !isRootOpened())
		return -1;

	int bytesWritten = writeFile(handle, buffer, size);
	return bytesWritten;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um diretório existente no disco.
-----------------------------------------------------------------------------*/
//Se a opera��o foi realizada com sucesso,
//a fun��o deve posicionar o ponteiro de entradas (current entry) na primeira posi��o v�lida do diret�rio.

int opendir2(void)
{
	initialize();
	if (!isPartitionMounted())
		return -1;

	openRoot();

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para ler as entradas de um diretório.
-----------------------------------------------------------------------------*/
int readdir2(DIRENT2 *dentry)
{
	initialize();
	if (!isPartitionMounted() || !isRootOpened())
		return -1;

	// Check if we already finished reading the entries
	if (finishedEntries(getInode(0)))
		return -1;

	// Try to read the record
	RECORD record;
	if (getRecordByNumber(getCurrentDirectoryEntryIndex(), &record) != 0)
	{
		printf("ERROR: Couldn't retrieve record.\n");
		return -1;
	};

	// Update to the next directory entry for the next function call
	nextDirectoryEntry();

	// If read an invalid record, go to the next
	if (record.TypeVal == TYPEVAL_INVALIDO)
		return readdir2(dentry);

	// Copy the record information to the `DIRENT2` structure
	memcpy(dentry->name, record.name, sizeof(BYTE) * 51);
	dentry->fileType = record.TypeVal;
	dentry->fileSize = getInode(record.inodeNumber)->bytesFileSize;

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um diretório.
-----------------------------------------------------------------------------*/
int closedir2(void)
{
	initialize();
	if (!isPartitionMounted())
		return -1;

	closeRoot();

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (softlink)
-----------------------------------------------------------------------------*/
int sln2(char *linkname, char *filename)
{
	initialize();

	// TODO: Remove this line later on
	opendir2();

	if (!isPartitionMounted() || !isRootOpened() || strlen(linkname) > 50 || strlen(filename) > 50)
	{
		printf("ERROR: Possible invalid filename or not opened partition or root folder.\n");
		return -1;
	}

	// Configure bitmap
	openBitmap2(getPartition()->firstSector);

	I_NODE *dirInode = getInode(0);
	RECORD record;

	// Remove old file with same name
	int filesQuantity = dirInode->bytesFileSize / RECORD_SIZE;
	for (int i = 0; i < filesQuantity; i++)
	{
		getRecordByNumber(i, &record);
		if (strcmp(record.name, linkname) == 0)
		{
			printf("ERROR: There is a file with the same name of the link.\n");
			return -1;
		}
	}

	// Fetch and set bitmaps info
	int inodeNumber = searchBitmap2(BITMAP_INODE, 0);
	int blockNum = searchBitmap2(BITMAP_DADOS, 0);
	if (inodeNumber == -1)
	{
		printf("ERROR: There is no space left to create a new inode.\n");
		return -1;
	}
	if (blockNum == -1)
	{
		printf("ERROR: There is no space left to allocate a new block.\n");
		return -1;
	}
	setBitmap2(BITMAP_INODE, inodeNumber, 1);
	setBitmap2(BITMAP_DADOS, blockNum, 1);

	// Copy information to the new record
	strcpy(record.name, linkname);
	record.TypeVal = TYPEVAL_LINK;
	record.inodeNumber = inodeNumber;

	// Compute where we will save the new record
	DWORD newRecordBlock = dirInode->bytesFileSize / getBlocksize();
	DWORD newRecordSector = dirInode->bytesFileSize % getBlocksize() / SECTOR_SIZE;
	DWORD newRecordSectorOffset = dirInode->bytesFileSize % SECTOR_SIZE;

	// Save it
	BYTE *record_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);

	if (readDataBlockSector(newRecordBlock, newRecordSector, dirInode, (BYTE *)record_buffer) != 0)
	{
		printf("ERROR: Failed reading record\n");
		return -1;
	}
	memcpy(record_buffer + newRecordSectorOffset, &record, sizeof(RECORD));
	if (writeDataBlockSector(newRecordBlock, newRecordSector, dirInode, (BYTE *)record_buffer) != 0)
	{
		printf("ERROR: Failed writing record\n");
		return -1;
	}

	// Compute where we will save the inode
	int inodeSector = inodeNumber / (SECTOR_SIZE / sizeof(I_NODE));
	int inodeSectorOffset = (inodeNumber % (SECTOR_SIZE / sizeof(I_NODE))) * sizeof(I_NODE);

	// Create and save inode
	I_NODE inode = {(DWORD)1, (DWORD)strlen(filename) + 1, {blockNum, (DWORD)0}, (DWORD)0, (DWORD)0, (DWORD)1, (DWORD)0};
	BYTE *buffer_inode = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
	if (read_sector(getInodesFirstSector(getPartition(), getSuperblock()) + inodeSector, buffer_inode) != 0)
	{
		printf("ERROR: Failed reading record\n");
		return -1;
	}
	memcpy(buffer_inode + inodeSectorOffset, &inode, sizeof(I_NODE));
	if (write_sector(getInodesFirstSector(getPartition(), getSuperblock()) + inodeSector, buffer_inode) != 0)
	{
		printf("ERROR: Failed writing record\n");
		return -1;
	}

	// We need to update the direntry inode
	dirInode->bytesFileSize += sizeof(RECORD);

	// Need to create a new block for the directory
	if (dirInode->bytesFileSize % getBlocksize() == 0)
	{
		printf("DEBUG: Will allocate new block for dir entries.\n");
		dirInode->blocksFileSize++;

		DWORD newBlock = searchBitmap2(BITMAP_DADOS, 0);
		if (newBlock == -1)
		{
			printf("ERROR: There is no space left to create a new directory entry.\n");
			return -1;
		}
		setBitmap2(BITMAP_DADOS, newBlock, 1);

		DWORD direct_quantity = getInodeDirectQuantity();
		DWORD simple_indirect_quantity = getInodeSimpleIndirectQuantity();
		DWORD double_indirect_quantity = getInodeDoubleIndirectQuantity();

		if (dirInode->blocksFileSize == direct_quantity)
		{
			// Second block
			dirInode->dataPtr[1] = newBlock;
		}
		else if (dirInode->blocksFileSize == direct_quantity + 1)
		{
			// Allocate block for the simple indirection block

			// Find bitmap entry
			DWORD newSimpleIndirectionBlock = searchBitmap2(BITMAP_DADOS, 0);
			if (newSimpleIndirectionBlock == -1)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}
			if (setBitmap2(BITMAP_DADOS, newSimpleIndirectionBlock, 1) != 0)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}
			dirInode->singleIndPtr = newSimpleIndirectionBlock;

			BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);

			memcpy(simple_ind_buffer, &newBlock, sizeof(newBlock));
			if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->singleIndPtr * getSuperblock()->blockSize, simple_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
		}
		else if (dirInode->blocksFileSize <= direct_quantity + simple_indirect_quantity)
		{
			// Middle single indirection block

			BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			if (read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->singleIndPtr * getSuperblock()->blockSize, simple_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
			memcpy(simple_ind_buffer + (dirInode->blocksFileSize - direct_quantity - 1) * sizeof(newBlock), &newBlock, sizeof(newBlock));
			if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->singleIndPtr * getSuperblock()->blockSize, simple_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
		}
		else if (dirInode->blocksFileSize == direct_quantity + simple_indirect_quantity + 1) // TODO: Need to allocate blocks for the double indirection
		{
			// Allocate bitmap for doubleIndirectionBlock
			DWORD newDoubleIndirectionBlock = searchBitmap2(BITMAP_DADOS, 0);
			if (newDoubleIndirectionBlock == -1)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}
			if (setBitmap2(BITMAP_DADOS, newDoubleIndirectionBlock, 1) != 0)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}
			dirInode->doubleIndPtr = newDoubleIndirectionBlock;

			// Allocate bitmap for simpleIndirectionBlock
			DWORD newSimpleIndirectionBlock = searchBitmap2(BITMAP_DADOS, 0);
			if (newSimpleIndirectionBlock == -1)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}
			if (setBitmap2(BITMAP_DADOS, newSimpleIndirectionBlock, 1) != 0)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}

			BYTE *double_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			memcpy(double_ind_buffer, &newSimpleIndirectionBlock, sizeof(newSimpleIndirectionBlock));
			if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->doubleIndPtr * getSuperblock()->blockSize, double_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}

			BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			memcpy(simple_ind_buffer, &newBlock, sizeof(newBlock));
			if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + newSimpleIndirectionBlock * getSuperblock()->blockSize, simple_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
		}
		else if ((dirInode->blocksFileSize - 3) % simple_indirect_quantity == 0)
		{
			// Allocate bitmap for simpleIndirectionBlock
			DWORD newSimpleIndirectionBlock = searchBitmap2(BITMAP_DADOS, 0);
			if (newSimpleIndirectionBlock == -1)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}
			if (setBitmap2(BITMAP_DADOS, newSimpleIndirectionBlock, 1) != 0)
			{
				printf("ERROR: There is no space left to create a new directory entry.\n");
				return -1;
			}

			BYTE *double_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			if (read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->doubleIndPtr * getSuperblock()->blockSize, double_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
			memcpy(double_ind_buffer + (dirInode->blocksFileSize - direct_quantity - simple_indirect_quantity - 1) / simple_indirect_quantity * sizeof(newSimpleIndirectionBlock), &newSimpleIndirectionBlock, sizeof(newSimpleIndirectionBlock));
			if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->doubleIndPtr * getSuperblock()->blockSize, double_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}

			BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			memcpy(simple_ind_buffer, &newBlock, sizeof(newBlock));
			if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + newSimpleIndirectionBlock * getSuperblock()->blockSize, simple_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
		}
		else
		{
			// Discover where is the simpleIndBlock
			BYTE *double_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			if (read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->doubleIndPtr * getSuperblock()->blockSize, double_ind_buffer))
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
			DWORD simple_ind_ptr = *((DWORD *)(double_ind_buffer + (dirInode->blocksFileSize - direct_quantity - simple_indirect_quantity - 1) / simple_indirect_quantity * sizeof(DWORD)));

			BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
			if (read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + simple_ind_ptr, simple_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
			memcpy(simple_ind_buffer + (dirInode->blocksFileSize - direct_quantity - simple_indirect_quantity - 1) % simple_indirect_quantity * sizeof(newBlock), &newBlock, sizeof(newBlock));
			if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + simple_ind_ptr, simple_ind_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
				return -1;
			}
		}
	}

	BYTE *dir_inode_buffer = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
	if (read_sector(getInodesFirstSector(getPartition(), getSuperblock()), dir_inode_buffer) != 0)
	{
		printf("ERROR: There was an error while trying to create a new directory entry.\n");
		return -1;
	}
	memcpy(dir_inode_buffer, dirInode, sizeof(I_NODE));
	if (write_sector(getInodesFirstSector(getPartition(), getSuperblock()), dir_inode_buffer) != 0)
	{
		printf("ERROR: There was an error while trying to create a new directory entry.\n");
		return -1;
	}

	//Encontra o inode:
	I_NODE *fileInode = getInode(inodeNumber);
	DWORD fileBlock = fileInode->dataPtr[0];

	//Copia o nome do arquivo para o buffer de escrita
	BYTE *data_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
	//TO-DO: Trocar pra memcpy (ou nao)
	for (int i = 0; i < sizeof(BYTE) * SECTOR_SIZE; i++)
	{
		data_buffer[i] = filename[i];
	}

	//Writes in the first block/sector of the file.
	if (writeDataBlockSector(0, 0, fileInode, (BYTE *)data_buffer) != 0)
	{
		printf("ERROR: Failed writing record\n");
		return -1;
	}

	// Free dynamically allocated memory
	free(record_buffer);
	free(data_buffer);
	free(buffer_inode);
	free(dirInode);

	// Remember to close the opened bitmap
	closeBitmap2();

	// Return a handler to this file
	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (hardlink)
-----------------------------------------------------------------------------*/
int hln2(char *linkname, char *filename)
{
	initialize();

	opendir2();

	if (!isPartitionMounted() || !isRootOpened())
		return -1;

	// Configure bitmap
	openBitmap2(getPartition()->firstSector);

	I_NODE *dirInode = getInode(0);
	RECORD record, hardLinkRecord;

	// Cancel operatino if link has same name as other file
	int filesQuantity = dirInode->bytesFileSize / RECORD_SIZE;
	for (int i = 0; i < filesQuantity; i++)
	{
		getRecordByNumber(i, &record);
		if (strcmp(record.name, linkname) == 0 && record.TypeVal != TYPEVAL_INVALIDO)
		{
			printf("ERROR: Trying to create hard link with same name as other file.\n");
			return -1;
		}
	}

	for (int i = 0; i < filesQuantity; i++)
	{
		getRecordByNumber(i, &record);

		//If found a file with the given name
		if (strcmp(record.name, filename) == 0 && record.TypeVal != TYPEVAL_INVALIDO)
		{

			// Copy information from file Record to hardLinkRecord
			memcpy(&hardLinkRecord, &record, sizeof(record));
			strcpy(hardLinkRecord.name, linkname);

			//Get file Inode and increment 1 in the reference counter)
			DWORD inodeNumber = hardLinkRecord.inodeNumber;
			I_NODE *inode = getInode(inodeNumber);
			inode->RefCounter = inode->RefCounter + 1;

			// Compute where we will save the new record
			DWORD newRecordBlock = dirInode->bytesFileSize / getBlocksize();
			DWORD newRecordSector = dirInode->bytesFileSize % getBlocksize() / SECTOR_SIZE;
			DWORD newRecordSectorOffset = dirInode->bytesFileSize % SECTOR_SIZE;

			// Save it
			BYTE *record_buffer = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
			if (readDataBlockSector(newRecordBlock, newRecordSector, dirInode, (BYTE *)record_buffer) != 0)
			{
				printf("ERROR: Failed reading record\n");
				return -1;
			}
			memcpy(record_buffer + newRecordSectorOffset, &hardLinkRecord, sizeof(RECORD));
			if (writeDataBlockSector(newRecordBlock, newRecordSector, dirInode, (BYTE *)record_buffer) != 0)
			{
				printf("ERROR: Failed writing record\n");
				return -1;
			}

			// Compute where we will save the inode
			DWORD inodeSector = inodeNumber / (SECTOR_SIZE / sizeof(I_NODE));
			DWORD inodeSectorOffset = (inodeNumber % (SECTOR_SIZE / sizeof(I_NODE))) * sizeof(I_NODE);

			// Save it
			BYTE *buffer_inode = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
			if (read_sector(getInodesFirstSector(getPartition(), getSuperblock()) + inodeSector, buffer_inode) != 0)
			{
				printf("ERROR: Failed reading record\n");
				return -1;
			}

			memcpy(buffer_inode + inodeSectorOffset, inode, sizeof(I_NODE));
			if (write_sector(getInodesFirstSector(getPartition(), getSuperblock()) + inodeSector, buffer_inode) != 0)
			{
				printf("ERROR: Failed writing record\n");
				return -1;
			}

			// We need to update the direntry inode
			dirInode->bytesFileSize += sizeof(RECORD);

			// Need to create a new block for the directory
			if (dirInode->bytesFileSize % getBlocksize() == 0)
			{
				printf("DEBUG: Will allocate new block for dir entries.\n");
				dirInode->blocksFileSize++;

				DWORD newBlock = searchBitmap2(BITMAP_DADOS, 0);
				if (newBlock == -1)
				{
					printf("ERROR: There is no space left to create a new directory entry.\n");
					return -1;
				}
				setBitmap2(BITMAP_DADOS, newBlock, 1);

				DWORD direct_quantity = getInodeDirectQuantity();
				DWORD simple_indirect_quantity = getInodeSimpleIndirectQuantity();
				DWORD double_indirect_quantity = getInodeDoubleIndirectQuantity();

				if (dirInode->blocksFileSize == direct_quantity)
				{
					// Second block
					dirInode->dataPtr[1] = newBlock;
				}
				else if (dirInode->blocksFileSize == direct_quantity + 1)
				{
					// Allocate block for the simple indirection block

					// Find bitmap entry
					DWORD newSimpleIndirectionBlock = searchBitmap2(BITMAP_DADOS, 0);
					if (newSimpleIndirectionBlock == -1)
					{
						printf("ERROR: There is no space left to create a new directory entry.\n");
						return -1;
					}
					if (setBitmap2(BITMAP_DADOS, newSimpleIndirectionBlock, 1) != 0)
					{
						printf("ERROR: There is no space left to create a new directory entry.\n");
						return -1;
					}
					dirInode->singleIndPtr = newSimpleIndirectionBlock;

					BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
					memcpy(simple_ind_buffer, &newBlock, sizeof(newBlock));
					if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->singleIndPtr * getSuperblock()->blockSize, simple_ind_buffer) != 0)
					{
						printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
						return -1;
					}
				}
				else if (dirInode->blocksFileSize <= direct_quantity + simple_indirect_quantity)
				{
					// Middle single indirection block

					BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
					if (read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->singleIndPtr * getSuperblock()->blockSize, simple_ind_buffer) != 0)
					{
						printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
						return -1;
					}
					memcpy(simple_ind_buffer + (dirInode->blocksFileSize - direct_quantity - 1) * sizeof(newBlock), &newBlock, sizeof(newBlock));
					if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->singleIndPtr * getSuperblock()->blockSize, simple_ind_buffer) != 0)
					{
						printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
						return -1;
					}
				}
				else if (dirInode->blocksFileSize == direct_quantity + simple_indirect_quantity + 1) // TODO: Need to allocate blocks for the double indirection
				{
					// Allocate bitmap for doubleIndirectionBlock
					DWORD newDoubleIndirectionBlock = searchBitmap2(BITMAP_DADOS, 0);
					if (newDoubleIndirectionBlock == -1)
					{
						printf("ERROR: There is no space left to create a new directory entry.\n");
						return -1;
					}
					if (setBitmap2(BITMAP_DADOS, newDoubleIndirectionBlock, 1) != 0)
					{
						printf("ERROR: There is no space left to create a new directory entry.\n");
						return -1;
					}
					dirInode->doubleIndPtr = newDoubleIndirectionBlock;

					// Allocate bitmap for simpleIndirectionBlock
					DWORD newSimpleIndirectionBlock = searchBitmap2(BITMAP_DADOS, 0);
					if (newSimpleIndirectionBlock == -1)
					{
						printf("ERROR: There is no space left to create a new directory entry.\n");
						return -1;
					}
					if (setBitmap2(BITMAP_DADOS, newSimpleIndirectionBlock, 1) != 0)
					{
						printf("ERROR: There is no space left to create a new directory entry.\n");
						return -1;
					}

					BYTE *double_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
					memcpy(double_ind_buffer, &newSimpleIndirectionBlock, sizeof(newSimpleIndirectionBlock));
					if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->doubleIndPtr * getSuperblock()->blockSize, double_ind_buffer) != 0)
					{
						printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
						return -1;
					}

					BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
					memcpy(simple_ind_buffer, &newBlock, sizeof(newBlock));
					if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + newSimpleIndirectionBlock * getSuperblock()->blockSize, simple_ind_buffer) != 0)
					{
						printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
						return -1;
					}
				}
				else if ((dirInode->blocksFileSize - 3) % simple_indirect_quantity == 0)
				{
					// Allocate bitmap for simpleIndirectionBlock
					DWORD newSimpleIndirectionBlock = searchBitmap2(BITMAP_DADOS, 0);
					if (newSimpleIndirectionBlock == -1)
					{
						printf("ERROR: There is no space left to create a new directory entry.\n");
						return -1;
					}
					if (setBitmap2(BITMAP_DADOS, newSimpleIndirectionBlock, 1) != 0)
					{
						printf("ERROR: There is no space left to create a new directory entry.\n");
						return -1;
					}

					BYTE *double_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
					if (read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->doubleIndPtr * getSuperblock()->blockSize, double_ind_buffer) != 0)
					{
						printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
						return -1;
					}
					memcpy(double_ind_buffer + (dirInode->blocksFileSize - direct_quantity - simple_indirect_quantity - 1) / simple_indirect_quantity * sizeof(newSimpleIndirectionBlock), &newSimpleIndirectionBlock, sizeof(newSimpleIndirectionBlock));
					if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->doubleIndPtr * getSuperblock()->blockSize, double_ind_buffer) != 0)
					{
						printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
						return -1;
					}

					BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
					memcpy(simple_ind_buffer, &newBlock, sizeof(newBlock));
					if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + newSimpleIndirectionBlock * getSuperblock()->blockSize, simple_ind_buffer) != 0)
					{
						printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
						return -1;
					}
				}
				else
				{
					// Discover where is the simpleIndBlock
					BYTE *double_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
					if (read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + dirInode->doubleIndPtr * getSuperblock()->blockSize, double_ind_buffer))
					{
						printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
						return -1;
					}
					DWORD simple_ind_ptr = *((DWORD *)(double_ind_buffer + (dirInode->blocksFileSize - direct_quantity - simple_indirect_quantity - 1) / simple_indirect_quantity * sizeof(DWORD)));

					BYTE *simple_ind_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
					if (read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + simple_ind_ptr, simple_ind_buffer) != 0)
					{
						printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
						return -1;
					}
					memcpy(simple_ind_buffer + (dirInode->blocksFileSize - direct_quantity - simple_indirect_quantity - 1) % simple_indirect_quantity * sizeof(newBlock), &newBlock, sizeof(newBlock));
					if (write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + simple_ind_ptr, simple_ind_buffer) != 0)
					{
						printf("ERROR: There was an error while trying to allocate space for a new directory entry.\n");
						return -1;
					}
				}
			}

			BYTE *dir_inode_buffer = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
			if (read_sector(getInodesFirstSector(getPartition(), getSuperblock()), dir_inode_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to create a new directory entry.\n");
				return -1;
			}
			memcpy(dir_inode_buffer, dirInode, sizeof(I_NODE));
			if (write_sector(getInodesFirstSector(getPartition(), getSuperblock()), dir_inode_buffer) != 0)
			{
				printf("ERROR: There was an error while trying to create a new directory entry.\n");
				return -1;
			}

			// Free dynamically allocated memory
			free(record_buffer);
			free(buffer_inode);
			free(dirInode);

			// Remember to close the opened bitmap
			closeBitmap2();

			return 0;
		}
	}

	return -1;
}
