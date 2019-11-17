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
	opendir2();

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
	if (notMountedPartition())
		return -1;
	if (notRootOpened())
		return -1;

	I_NODE *dirInode = getInode(0);
	RECORD record;
	RECORD newRecord;

	int filesQuantity = dirInode->bytesFileSize/RECORD_SIZE;

	for (int i= 0; i<filesQuantity; i ++){
		getRecordByNumber(i, &record);
		if(strcmp(record.name, filename) == 0){
			//delete
		}
	}

	openBitmap2(getPartition()->firstSector);

	DWORD newRecordBlock = dirInode->bytesFileSize / getBlocksize();
	DWORD newRecordSector = dirInode->bytesFileSize / SECTOR_SIZE;
	newRecordSector = (newRecordSector) / getSuperblock()->blockSize;
	DWORD newRecordSectorOffset = newRecordSector % getSuperblock()->blockSize;

	strcpy(record.name, filename);
	record.TypeVal = TYPEVAL_REGULAR;
	DWORD inodeNumber = searchBitmap2(BITMAP_INODE, 0);
	record.inodeNumber = inodeNumber;
	DWORD blockNum = searchBitmap2(BITMAP_DADOS, 0);
	setBitmap2(BITMAP_DADOS, blockNum, 1);

	BYTE *record_buffer = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
	if (readDataBlockSector(newRecordBlock, newRecordSector, dirInode, (BYTE *)record_buffer) != 0)
	{
			printf("ERROR: Failed reading record\n");
			return -1;
	}
	memcpy(record_buffer + newRecordSectorOffset, &record, sizeof(RECORD));


	if(writeDataBlockSector(newRecordBlock, newRecordSector, dirInode, (BYTE *)record_buffer) != 0)
	{
			printf("ERROR: Failed writing record\n");
			return -1;
	}

	setBitmap2(BITMAP_INODE, inodeNumber, 1);
	DWORD inodeSector = (inodeNumber * sizeof(I_NODE)) / (SECTOR_SIZE / sizeof(I_NODE));
	DWORD inodeSectorOffset = (inodeNumber * sizeof(I_NODE)) % (SECTOR_SIZE / sizeof(I_NODE));

	BYTE *buffer_inode = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
	I_NODE inode = {(DWORD)1, (DWORD)0, {blockNum, (DWORD)0}, (DWORD)0, (DWORD)0, (DWORD)1, (DWORD)0};
	if(read_sector(getInodesFirstSector(getPartition(), getSuperblock()) + inodeSector, buffer_inode)!= 0)
	{
			printf("ERROR: Failed reading record\n");
			return -1;
	}

	memcpy(buffer_inode + inodeSectorOffset, &inode, sizeof(I_NODE));

	if(write_sector(getInodesFirstSector(getPartition(), getSuperblock()) + inodeSector, buffer_inode)!= 0)
	{
			printf("ERROR: Failed writing record\n");
			return -1;
	}

	free(record_buffer);
	free(buffer_inode);
	free(dirInode);
	closeBitmap2();

	return open2(filename);
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
		return -1;
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

	openRoot();

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

	PARTITION *partition = getPartition();
	I_NODE *rootFolderInode = getInode(0);

	// Check if we already finished reading the entries
	if (finishedEntries(rootFolderInode))
		return -1;

	RECORD record;
	if (getRecordByNumber(getCurrentDirectoryEntryIndex(), &record) != 0)
	{
		printf("ERROR: Couldn't retrieve record.\n");
		return -1;
	};

	memcpy(dentry->name, record.name, sizeof(BYTE) * 51);
	dentry->fileType = record.TypeVal;
	dentry->fileSize = getInode(record.inodeNumber)->bytesFileSize;

	nextDirectoryEntry();

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

	closeRoot();

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
