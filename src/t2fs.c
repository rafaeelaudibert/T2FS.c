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
	mount(0);

	BYTE identification[] = "Ana Carolina Pagnoncelli - 00287714\nAugusto Zanella Bardini  - 00278083\nRafael Baldasso Audibert - 00287695";
	memcpy(name, identification, size);

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
	opendir2();
	if (!isPartitionMounted() || !isRootOpened())
		return -1;

	I_NODE *root_inode = getInode(0);
	RECORD record;
	I_NODE file_inode;
	I_NODE *directory_inode;
	BYTE *buffer;
	BYTE *folder_buffer;
	char name[10];
	char numstr[5];

	for(int counter = 1; counter <= 4; counter++){
		strcpy(name, filename);
		printf("chegou %d\n", counter);
		snprintf(numstr, sizeof(numstr), "%d",counter);
		strcat(name, numstr);
		strcpy(record.name, name);
		printf("chegou2 %d\n", counter);
		printf("%s\n", record.name);

		record.TypeVal = 0x01;
		record.inodeNumber = counter;
		file_inode.blocksFileSize = 1;
		file_inode.bytesFileSize = 0;
		file_inode.dataPtr[0] = counter;
		file_inode.dataPtr[1] = 0;
		file_inode.singleIndPtr = 0;
		file_inode.doubleIndPtr = 0;
		file_inode.RefCounter = 1;

		printf("chegou3 data %d %d\n", file_inode.dataPtr[0], file_inode.dataPtr[1]);
		openBitmap2(getPartition()->firstSector);

		setBitmap2(BITMAP_INODE, counter, 1);
		setBitmap2(BITMAP_DADOS, counter, 1);

		//writing inodes
		directory_inode = getInode(0);
		directory_inode->bytesFileSize = sizeof(record)*counter;
		buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
		read_sector(getInodesFirstSector(getPartition(), getSuperblock()), buffer);
		memcpy(buffer, directory_inode, sizeof(I_NODE));
		printf("chegou4 %d\n", counter);

		memcpy(buffer + counter*sizeof(I_NODE), &file_inode, sizeof(I_NODE));
		write_sector(getInodesFirstSector(getPartition(), getSuperblock()), buffer);

		folder_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
		printf("chegou5 %d\n", counter);

		if(counter > 4){
			read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + directory_inode->dataPtr[1]*getSuperblock()->blockSize, folder_buffer);
			printf("chegou44 %d\n", counter);
			memcpy(folder_buffer+sizeof(RECORD)*(counter-1), &record, sizeof(RECORD));
			printf("chegou44 %d\n", counter);
			write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + directory_inode->dataPtr[1]*getSuperblock()->blockSize, folder_buffer);
			printf("chegou44 %d\n", counter);
		} else {
			read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + directory_inode->dataPtr[0]*getSuperblock()->blockSize, folder_buffer);
			printf("chegou00 %d\n", counter);
			memcpy(folder_buffer+sizeof(RECORD)*(counter-1), &record, sizeof(RECORD));
			printf("chegou00 %d\n", counter);
			write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + directory_inode->dataPtr[0]*getSuperblock()->blockSize, folder_buffer);
			printf("chegou00 %d\n", counter);
		}
		closeBitmap2();

	}

	return open2("hue1");
}

/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um arquivo do disco.
-----------------------------------------------------------------------------*/
int delete2(char *filename)
{
	initialize();
	if (!isPartitionMounted() || !isRootOpened())
		return -1;

	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um arquivo existente no disco.
-----------------------------------------------------------------------------*/
FILE2 open2(char *filename)
{
	initialize();
	if (!isPartitionMounted() || !isRootOpened())
		return -1;

	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um arquivo.
-----------------------------------------------------------------------------*/
int close2(FILE2 handle)
{
	if (!isPartitionMounted() || !isRootOpened())
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
	if (!isPartitionMounted() || !isRootOpened())
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
	if (!isPartitionMounted() || !isRootOpened())
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
	if (finishedEntries(getInode(0))){
		printf("Finished\n");
		return -1;
	}

	// Try to read the record
	RECORD record;
	if (getRecordByNumber(getCurrentDirectoryEntryIndex(), &record) != 0)
	{
		printf("ERROR: Couldn't retrieve record.\n");
		return -1;
	};

	// Copy the record information to the `DIRENT2` structure
	memcpy(dentry->name, record.name, sizeof(BYTE) * 51);
	dentry->fileType = record.TypeVal;
	dentry->fileSize = getInode(record.inodeNumber)->bytesFileSize;

	// Update to the next directory entry for the next function call
	nextDirectoryEntry();

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
	if (!isPartitionMounted() || !isRootOpened())
		return -1;

	return -9;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (hardlink)
-----------------------------------------------------------------------------*/
int hln2(char *linkname, char *filename)
{
	initialize();
	if (!isPartitionMounted() || !isRootOpened())
		return -1;

	return -9;
}
