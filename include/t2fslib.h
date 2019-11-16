#include "t2fs.h"

#ifndef _T2FSLIB_H_
#define _T2FSLIB_H_

#define MBR_SECTOR 0
#define BOOL unsigned short int
#define TRUE 1
#define FALSE 0

typedef struct t2fs_superbloco SUPERBLOCK;
typedef struct t2fs_record RECORD;
typedef struct t2fs_inode I_NODE;

// "Private" functions
void initialize();
int readMBR();
int formatPartition(int, int);
DWORD computeChecksum(SUPERBLOCK *);
int createRootFolder(int);
int configureMountedPartition(int);
int unmountPartition();
void openRoot();
void closeRoot();
BOOL finishedEntries(I_NODE *);
DWORD getBlockBitmapFirstSector(PARTITION *, SUPERBLOCK *);
DWORD getBlockBitmapLastSector(PARTITION *, SUPERBLOCK *);
DWORD getInodeBitmapFirstSector(PARTITION *, SUPERBLOCK *);
DWORD getInodeBitmapLastSector(PARTITION *, SUPERBLOCK *);
DWORD getInodesFirstSector(PARTITION *, SUPERBLOCK *);
DWORD getDataBlocksFirstSector(PARTITION *, SUPERBLOCK *);
BOOL notMountedPartition();
BOOL notRootOpened();
MBR *getMBR();
SUPERBLOCK *getSuperblock();
PARTITION *getPartition();
int getBlocksize();
int readDataBlockSector(int, int, I_NODE *, BYTE *);
BYTE *getBuffer(size_t);
BYTE *getZeroedBuffer(size_t);
I_NODE *getInode(DWORD);
int getCurrentDirectoryEntryIndex();
void nextDirectoryEntry();
int getRecordByNumber(int, RECORD *);

// iNodePointersQuantities
DWORD getInodeDirectQuantity();
DWORD getInodeSimpleIndirectQuantity();
DWORD getInodeDoubleIndirectQuantity();

#endif