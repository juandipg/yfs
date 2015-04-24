#include <stdbool.h>
#include <comp421/iolib.h>

#define INODESPERBLOCK (BLOCKSIZE / INODESIZE)
#define CREATE_NEW -1

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
int yfsRead(int inodeNum, void *buf, int size, int byteOffset, int pid);
int yfsWrite(int inodeNum, void *buf, int size, int byteOffset, int pid);
int yfsLink(char *oldName, char *newName, int currentInode);
int yfsUnlink(char *pathname, int currentInode);
int yfsSymLink(char *oldname, char *newname, int currentInode);
int yfsReadLink(char *pathname, char *buf, int len, int currentInode, int pid);
int yfsMkDir(char *pathname, int currentInode);
int yfsRmDir(char *pathname, int currentInode);
int yfsChDir(char *pathname, int currentInode);
int yfsStat(char *pathname, int currentInode, struct Stat *statbuf);
int yfsSync(void);
int yfsShutdown(void);
