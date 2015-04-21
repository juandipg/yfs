#define INODESPERBLOCK (BLOCKSIZE / INODESIZE)
//#define DIRENTRIESPERBLOCK (BLOCKSIZE / sizeof(struct dir_entry))

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

struct inode* getInode(void *blockAddr, int inodeNum);
void addFreeInodeToList(int inodeNum);
void buildFreeInodeAndBlockLists();
int getNextFreeBlockNum();
int getDirectoryEntry(char *pathname, int inodeStartNumber, int *blockNumPtr, bool createIfNeeded);