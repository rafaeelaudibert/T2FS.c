#include "t2fs.h"

#ifndef _T2FSLIB_H_
#define _T2FSLIB_H_

#define MBR_SECTOR 0
#define BOOL unsigned short int
#define TRUE 1
#define FALSE 0

#define RECORD_PER_SECTOR 4
#define	INVALID_PTR	0
#define PTR_SIZE 4
#define PTR_PER_SECTOR 64
#define BLOCK_SIZE superblock->blockSize
#define	SECTOR_SIZE	256
#define RECORD_SIZE 64
#define INODE_SIZE 32
#define INODE_PER_SECTOR 8

typedef struct t2fs_superbloco SUPERBLOCK;
typedef struct t2fs_record RECORD;
typedef struct t2fs_inode I_NODE;


/*

    FUNCTIONS USED TO INITIALIZE THE LIBRARY

*/

// Initialize the needed structures
void initialize();

/*

    FUNCTIONS USED ON FORMAT2

*/
// Once called, reads the disk MBR to memory
int readMBR();

//  Formats a given partition `partition_number` with each logical block
//  occupying `sectors_per_block` physical sectors
int formatPartition(int partition_number, int sectors_per_block);

// Compute the Checksum for a given `superblock`
DWORD computeChecksum(SUPERBLOCK *superblock);

//  Initializes the needed structures to hold the root folder
//  for a given partition identified by `partition_number`
int createRootFolder(int partition_number);

/*

    FUNCTIONS USED ON MOUNT2

*/
// Mount the partition identified by the number `partition_number` on "\\",
// reading their superblock to the memory
int configureMountedPartition(int partition_number);

// Unmount the partition currently mounted on "\\" path
int unmountPartition();

/*

    FUNCTIONS USED ON OPENDIR2

*/
// Open the folder located at "\\" initializing the used variables
void openRoot();

/*

    FUNCTIONS USED ON CLOSEDIR2

*/
// Close the folder located at "\\" freeing the used variables
void closeRoot();

/*

    FUNCTIONS USED ON READDIR2

*/
// Verifies if all the entries on the directory have already been read
BOOL finishedEntries(I_NODE *inode);

// Returns the current directory entry, which is constantly incremented in the `readdir2` function
// by calling `nextDirectoryEntry`
int getCurrentDirectoryEntryIndex();

// Increments the current `directoryEntryIndex`
void nextDirectoryEntry();

/*

    GENERIC FUNCTIONS

*/
// Returns the number of the block where the Block Bitmap starts
DWORD getBlockBitmapFirstSector(PARTITION *, SUPERBLOCK *);

// Returns the number of the block where the Block Bitmap ends
DWORD getBlockBitmapLastSector(PARTITION *, SUPERBLOCK *);

// Returns the number of the block where the Inode Bitmap starts
DWORD getInodeBitmapFirstSector(PARTITION *, SUPERBLOCK *);

// Returns the number of the block where the Inode Bitmap ends
DWORD getInodeBitmapLastSector(PARTITION *, SUPERBLOCK *);

// Returns the number of the block where the Inodes start
DWORD getInodesFirstSector(PARTITION *, SUPERBLOCK *);

// Returns the number of the block where the Data blocks start
DWORD getDataBlocksFirstSector(PARTITION *, SUPERBLOCK *);

// Checks if a partition is already mounted
BOOL notMountedPartition();

// Checks if the root folder "\\" is already opened
BOOL notRootOpened();

// Returns a `MBR` pointer to the disk MBR
MBR *getMBR();

// Returns a `SUPERBLOCK` pointer to the superblock
// of the currently mounted partition
SUPERBLOCK *getSuperblock();

// Returns a `PARTITION` pointer to the currently
// mounted partition
PARTITION *getPartition();

// Returns the size of the block in bytes
int getBlocksize();

// Reads the sector `sector_number` from the block `block_number` from a file
// identified by the inode `inode`.
// The sector information is copied to the `buffer` pointer.
int readDataBlockSector(int block_number, int sector_number, I_NODE *inode, BYTE *buffer);

// Writes to the sector `sector_number` from the block `block_number` from a file
// identified by the inode `inode`.
// The sector information is copied from the `write_buffer` pointer.
int writeDataBlockSector(int block_number, int sector_number, I_NODE *inode, BYTE *write_buffer);

// Returns a newly allocated buffer, with size `size` (similar to malloc)
BYTE *getBuffer(size_t size);

// Returns a newly allocated buffer, with their content zeroed, with size `size` (similar to calloc)
BYTE *getZeroedBuffer(size_t size);

// Returns the inode of number `inodeNumber` in the inode blocks
I_NODE *getInode(DWORD inodeNumber);

// Gets a record by its number, filling the `record` structure
int getRecordByNumber(int number, RECORD *structure);

// Quantity of direct blocks that an INODE can hold
DWORD getInodeDirectQuantity();

// Quantity of simple indirection block that an INODE can hold
DWORD getInodeSimpleIndirectQuantity();

// Quantity of double indirection blocks that an INODE can hold
DWORD getInodeDoubleIndirectQuantity();

#endif
