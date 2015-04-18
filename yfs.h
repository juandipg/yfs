#define INODESPERBLOCK (BLOCKSIZE / INODESIZE)
#define DIRENTRIESPERBLOCK (BLOCKSIZE / sizeof(struct dir_entry))

typedef struct freeInode freeInode;

struct freeInode {
    int inodeNumber;
    freeInode *next;
};

struct inode* getInode(void *blockAddr, int inodeNum);
void addFreeInodeToList(int inodeNum);
void buildFreeInodeList();
int getDirectoryEntry(char *pathname, int inodeStartNumber, int *blockNumPtr, bool createIfNeeded);