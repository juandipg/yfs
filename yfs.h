#define INODESPERBLOCK (BLOCKSIZE / INODESIZE)

typedef struct freeInode freeInode;

struct freeInode {
    int inodeNumber;
    freeInode *next;
};

struct inode* getInode(void *blockAddr, int inodeNum);
void addFreeInodeToList(int inodeNum);
void buildFreeInodeList();