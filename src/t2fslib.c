#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "t2fs.h"
#include "t2disk.h"
#include "apidisk.h"
#include "bitmap2.h"
#include "t2fslib.h"

// Debug variables
BOOL debug = TRUE;

// Global variables
MBR *mbr = NULL;
SUPERBLOCK *superblock = NULL;
int mounted_partition = -1;
BOOL rootOpened = FALSE;
DWORD rootFolderFileIndex = 0;

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
    BYTE *buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
    BYTE *zeroed_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);

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
    BYTE superblock_id[] = "T2FS";
    memcpy(sb.id, superblock_id, 4);
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
    BYTE *buffer = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
    if (read_sector(partition.firstSector, (BYTE *)buffer) != 0)
    {
        printf("ERROR: Failed reading superblock of partition %d\n", partition_number);
        return -1;
    }
    memcpy(&sb, buffer, sizeof(sb));

    // Create inode and mark it on the bitmap, automatically pointing to the first entry in the data block
    BYTE *inode_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
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
    BYTE *data_buffer = getZeroedBuffer(sizeof(BYTE) * SECTOR_SIZE);
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

int configureMountedPartition(int partition_number)
{
    BYTE *buffer = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
    if (read_sector(getMBR()->partitions[partition_number].firstSector, buffer) != 0)
    {
        printf("ERROR: Failed reading superblock.\n");
        return -1;
    }

    superblock = (SUPERBLOCK *)malloc(sizeof(SUPERBLOCK));
    memcpy(superblock, buffer, sizeof(SUPERBLOCK));

    // Mark mounted partition
    mounted_partition = partition_number;

    // Remember to clean up buffer allocated memory
    free(buffer);

    return 0;
}

inline int unmountPartition()
{
    if (superblock != NULL)
    {
        free(superblock);
        superblock = NULL;
    }

    // Unmark mounted partition
    mounted_partition = -1;

    return 0;
}

inline void openRoot()
{
    rootOpened = TRUE;
    rootFolderFileIndex = 0;

    return;
}

inline void closeRoot()
{
    rootOpened = FALSE;

    return;
}

inline BOOL finishedEntries(I_NODE *inode)
{
    return rootFolderFileIndex * sizeof(RECORD) >= inode->bytesFileSize;
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
inline BOOL isPartitionMounted()
{
    if (superblock == NULL)
    {
        printf("ERROR: There is no mounted partition. Please mount it first.\n");
        return FALSE;
    }

    return TRUE;
}

inline BOOL isRootOpened()
{
    if (!rootOpened)
    {
        printf("ERROR: You must open the root directory.");
        return FALSE;
    }

    return TRUE;
}

/* Getters */
inline MBR *getMBR()
{
    return mbr;
}

inline SUPERBLOCK *getSuperblock()
{
    return superblock;
}

inline PARTITION *getPartition()
{
    if (mbr == NULL)
    {
        return NULL;
    }

    return &(getMBR()->partitions[mounted_partition]);
}

inline int getBlocksize()
{
    return superblock->blockSize * SECTOR_SIZE;
}

int readDataBlockSector(int block_number, int sector_number, I_NODE *inode, BYTE *buffer)
{
    // Doesn't try to access not existent blocks
    if (block_number >= (int)inode->blocksFileSize)
    {
        printf("ERROR: Trying to acess not existent block");
        return -1;
    }

    DWORD direct_quantity = getInodeDirectQuantity();
    DWORD simple_indirect_quantity = getInodeSimpleIndirectQuantity();
    // DWORD double_indirect_quantity = getInodeDoubleIndirectQuantity();

    DWORD double_ind_sector = inode->doubleIndPtr * getSuperblock()->blockSize;
    DWORD simple_ind_sector = inode->singleIndPtr * getSuperblock()->blockSize;
    DWORD no_ind_sector = inode->dataPtr[block_number > 2 ? 0 : block_number] * getSuperblock()->blockSize; // We fill with a 0 in the default case, because it will be filled later

    if (block_number >= (int)direct_quantity + (int)simple_indirect_quantity)
    {
        // Read with double indirection
        int shifted_block_number = block_number - direct_quantity - simple_indirect_quantity;

        // We need to find where is the block with the simple indirection
        DWORD double_indirect_block = (shifted_block_number * sizeof(DWORD)) / (getBlocksize() / sizeof(DWORD));
        DWORD double_indirect_block_sector_offset = double_indirect_block / SECTOR_SIZE;
        DWORD double_indirect_sector_offset = double_indirect_block % SECTOR_SIZE;
        if ((read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + double_ind_sector + double_indirect_block_sector_offset, buffer)) != 0)
        {
            printf("ERROR: Couldn't read double indirection first data block.\n");
            return -1;
        }

        simple_ind_sector = *((DWORD *)(buffer + double_indirect_sector_offset)) * getSuperblock()->blockSize;
        block_number = (shifted_block_number % simple_indirect_quantity) + direct_quantity; // We add direct_quantity to make sense to discount it in the next indirection
    }

    if (block_number >= (int)direct_quantity)
    {
        // Read with simple indirection
        DWORD shifted_block_number = block_number - direct_quantity; // To find out which block inside the indirection we should read

        // We need to find where is the direct block
        DWORD indirect_sector = (shifted_block_number * sizeof(DWORD)) / SECTOR_SIZE;
        DWORD indirect_sector_position = (shifted_block_number * sizeof(DWORD)) % SECTOR_SIZE;
        if ((read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + simple_ind_sector + indirect_sector, buffer)) != 0)
        {
            printf("ERROR: Couldn't read simple indirection data block.\n");
            return -1;
        }

        // Update this value to know where is the block to read
        no_ind_sector = *((DWORD *)(buffer + indirect_sector_position)) * getSuperblock()->blockSize;
    }

    // Read without indirection
    DWORD block_to_read = no_ind_sector;
    DWORD sector_to_read = block_to_read + sector_number;
    if ((read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + sector_to_read, buffer)) != 0)
    {
        printf("ERROR: Failed to read folder data sector.\n");
        return -1;
    }

    return 0;
}

int writeDataBlockSector(int block_number, int sector_number, I_NODE *inode, BYTE *write_buffer)
{
    // Doesn't try to access not existent blocks
    if (block_number >= (int)inode->blocksFileSize)
    {
        printf("ERROR: Trying to acess not existent block");
        return -1;
    }

    BYTE *buffer = getBuffer(sizeof(BYTE) * SECTOR_SIZE);

    DWORD direct_quantity = getInodeDirectQuantity();
    DWORD simple_indirect_quantity = getInodeSimpleIndirectQuantity();
    // DWORD double_indirect_quantity = getInodeDoubleIndirectQuantity();

    DWORD double_ind_sector = inode->doubleIndPtr * getSuperblock()->blockSize;
    DWORD simple_ind_sector = inode->singleIndPtr * getSuperblock()->blockSize;
    DWORD no_ind_sector = inode->dataPtr[block_number > 2 ? 0 : block_number] * getSuperblock()->blockSize; // We fill with a 0 in the default case, because it will be filled later

    if (block_number >= (int)direct_quantity + (int)simple_indirect_quantity)
    {
        // Read with double indirection
        int shifted_block_number = block_number - direct_quantity - simple_indirect_quantity;

        // We need to find where is the block with the simple indirection
        DWORD double_indirect_block = (shifted_block_number * sizeof(DWORD)) / (getBlocksize() / sizeof(DWORD));
        DWORD double_indirect_block_sector_offset = double_indirect_block / SECTOR_SIZE;
        DWORD double_indirect_sector_offset = double_indirect_block % SECTOR_SIZE;
        if ((read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + double_ind_sector + double_indirect_block_sector_offset, buffer)) != 0)
        {
            printf("ERROR: Couldn't read double indirection first data block.\n");
            return -1;
        }

        simple_ind_sector = *((DWORD *)(buffer + double_indirect_sector_offset)) * getSuperblock()->blockSize;
        block_number = (shifted_block_number % simple_indirect_quantity) + direct_quantity; // We add direct_quantity to make sense to discount it in the next indirection
    }

    if (block_number >= (int)direct_quantity)
    {
        // Read with simple indirection
        DWORD shifted_block_number = block_number - direct_quantity; // To find out which block inside the indirection we should read

        // We need to find where is the direct block
        DWORD indirect_sector = (shifted_block_number * sizeof(DWORD)) / SECTOR_SIZE;
        DWORD indirect_sector_position = (shifted_block_number * sizeof(DWORD)) % SECTOR_SIZE;
        if ((read_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + simple_ind_sector + indirect_sector, buffer)) != 0)
        {
            printf("ERROR: Couldn't read simple indirection data block.\n");
            return -1;
        }

        // Update this value to know where is the block to read
        no_ind_sector = *((DWORD *)(buffer + indirect_sector_position)) * getSuperblock()->blockSize;
    }

    // Read without indirection
    DWORD block_to_write = no_ind_sector;
    DWORD sector_to_write = block_to_write + sector_number;
    if ((write_sector(getDataBlocksFirstSector(getPartition(), getSuperblock()) + sector_to_write, write_buffer)) != 0)
    {
        printf("ERROR: Failed to read folder data sector.\n");
        return -1;
    }

    return 0;
}

inline BYTE *getBuffer(size_t size)
{
    return (BYTE *)malloc(size);
}

inline BYTE *getZeroedBuffer(size_t size)
{
    BYTE *buffer = getBuffer(size);
    memset(buffer, 0, size);

    return buffer;
}

I_NODE *getInode(DWORD inodeNumber)
{
    I_NODE *inode = (I_NODE *)malloc(sizeof(I_NODE));
    BYTE *buffer = getBuffer(sizeof(BYTE) * SECTOR_SIZE);

    // We need to compute what is the position of the Inode
    // We can take in consideration that all inode sectors are consecutive,
    // so we don't need to take blocks in consideration now
    DWORD inodeSector = (inodeNumber * sizeof(I_NODE)) / SECTOR_SIZE;
    DWORD inodeSectorOffset = (inodeNumber * sizeof(I_NODE)) % SECTOR_SIZE;

    if ((read_sector(getInodesFirstSector(getPartition(), getSuperblock()) + inodeSector, buffer)) != 0)
    {
        printf("ERROR: Couldn't read inode.\n");
        return NULL;
    }
    memcpy((BYTE *)inode, (BYTE *)(buffer + inodeSectorOffset), sizeof(I_NODE));

    // Remember to free dynamically allocated memory
    free(buffer);

    return inode;
}

inline int getCurrentDirectoryEntryIndex()
{
    return rootFolderFileIndex;
}

inline void nextDirectoryEntry()
{
    rootFolderFileIndex++;

    return;
}

int getRecordByNumber(int number, RECORD *record)
{
    // Get in which block and sector of the block we should look for
    DWORD byte_position = number * sizeof(RECORD);
    DWORD block = byte_position / getBlocksize();
    DWORD block_position = byte_position % getBlocksize();
    DWORD sector = block_position / SECTOR_SIZE;
    DWORD sector_position = block_position % SECTOR_SIZE;

    I_NODE *rootFolderInode = getInode(0);
    BYTE *buffer = getBuffer(sizeof(BYTE) * SECTOR_SIZE);
    if (readDataBlockSector(block, sector, rootFolderInode, buffer) != 0)
    {
        printf("ERROR: Couldn't read directory entry");
        return -1;
    }
    memcpy(record, buffer + sector_position, sizeof(RECORD));

    return 0;
}

int getPointers(DWORD blockNumber, DWORD *pointers){
	unsigned char buffer[SECTOR_SIZE];
	int i, j;

	for(i = 0; i < getSuperblock()->blockSize; i++){ // For all sector of block
		int sectorNumber = blockNumber*getSuperblock()->blockSize + i;
		read_sector(sectorNumber, buffer);
		for(j = 0; j < PTR_PER_SECTOR; j++){  // For all record of sector
			pointers[j + i*PTR_PER_SECTOR] = *((DWORD*)(buffer + j*PTR_SIZE));
		}
	}
	return 0;
}


// iNodePointersQuantities
inline DWORD getInodeDirectQuantity()
{
    // Constant 2
    return 2;
}

inline DWORD getInodeSimpleIndirectQuantity()
{
    // Bytes in a block / size of each pointer in the file
    return superblock->blockSize * SECTOR_SIZE / sizeof(DWORD);
}

inline DWORD getInodeDoubleIndirectQuantity()
{
    // For each pointer in the first indirection file (getInodeSimpleIndirectQuantity),
    // we have another getInodeSimpleIndirectQuantity entries
    return getInodeSimpleIndirectQuantity() * getInodeSimpleIndirectQuantity();
}
