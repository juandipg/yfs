#define INODESPERBLOCK (BLOCKSIZE / INODESIZE)

typedef struct freeInode freeInode;

struct freeInode {
    int inodeNumber;
    freeInode *next;
};

struct inode* getInode(void *blockAddr, int blockNum, int inodeNum);
void addFreeInodeToList(int inodeNum);