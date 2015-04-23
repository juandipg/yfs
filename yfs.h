#include <stdbool.h>

#define INODESPERBLOCK (BLOCKSIZE / INODESIZE)

typedef struct freeInode freeInode;
typedef struct freeBlock freeBlock;
typedef struct cacheItem cacheItem;
typedef struct queue queue;

struct cacheItem {
    int number;
    bool dirty;
    void *addr;
    cacheItem *prevItem;
    cacheItem *nextItem;
};

struct freeInode {
    int inodeNumber;
    freeInode *next;
};

struct freeBlock {
    int blockNumber;
    freeBlock *next;
};

struct queue {
    cacheItem *firstItem;
    cacheItem *lastItem;
};

void *getBlock(int blockNumber);
void destroyCacheItem(cacheItem *item);
struct inode* getInode(int inodeNum);
void addFreeInodeToList(int inodeNum);
void buildFreeInodeAndBlockLists();
int getNextFreeBlockNum();
int getDirectoryEntry(char *pathname, int inodeStartNumber, int *blockNumPtr, bool createIfNeeded);
int yfsCreate(char *pathname, int currentInode, int inodeNumToSet);
int yfsOpen(char *pathname, int currentInode);
