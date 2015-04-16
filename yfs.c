#include <stdbool.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include "yfs.h"

freeInode *firstFreeInode = NULL;
int freeInodeCount = 0;
int currentInode = ROOTINODE;


void 
init() {
    buildFreeInodeList();
}

bool
isEqual(char *path, char dirEntryName[]) {
    int i = 0;
    while (i < DIRNAMELEN) {
        if ((path[i] == '/' || path[i] == '\0') && dirEntryName[i] == '\0') {
            return true;
        }
        if (path[i] != dirEntryName[i]) {
            return false;
        }
        i++;
    }
    return true;
}

void
saveBlock(int blockNumber, void *block) {
    WriteSector(blockNumber, block);
}

void *
getBlock(int blockNumber) {
    void *block = malloc(BLOCKSIZE);
    ReadSector(blockNumber, block);
    return block;
}

void *
getBlockForInode(int inodeNumber) {
    int blockNumber = (inodeNumber / INODESPERBLOCK) + 1;
    return getBlock(blockNumber);
}

/*
 * Expects: inode, n
 * Results: mallocs that block into memory
 */
void *
getNthBlock(struct inode *inode, int n) {
    if ((n*BLOCKSIZE > inode->size)
            || (n > NUM_DIRECT + BLOCKSIZE / (int)sizeof(int))) 
    {
        return NULL;
    }
    if (n < NUM_DIRECT) {
        return getBlock(inode->direct[n]);
    } 
    //search the direct blocks
    int *indirectBlock = getBlock(inode->indirect);
    return getBlock(indirectBlock[n - NUM_DIRECT]);
}

/**
 * 
 * @param path the file path to get the inode number for
 * @param inodeStartNumber the inode to start looking for the next part
 *       of the file path in
 * @return the inode number of the path, or 0 if it's an invalid path
 */
int
getInodeNumberForPath (char *path, int inodeStartNumber) {
    //get the inode number for the first file in path 
    // ex: if path is "/a/b/c.txt" get the indoe # for "a"
    int nextInodeNumber = 0;
    
    //Get inode corresponding to inodeStartNumber
    void *block = getBlockForInode(inodeStartNumber);
    
    struct inode *inode = getInode(block, inodeStartNumber);
    if (inode->type == INODE_DIRECTORY) {
        int i = 0;
        void *currentBlock = getNthBlock(inode, i);
        int totalSize = sizeof(struct dir_entry);
        bool isFound = false;
        while (currentBlock != NULL && !isFound) {
            struct dir_entry *currentEntry = (struct dir_entry *)currentBlock;
            while (totalSize <= inode->size && ((char *)currentEntry < ((char *)currentBlock + BLOCKSIZE))) {
                //check the currentEntry fileName to see if it matches        
                if (isEqual(path, currentEntry->name)) {
                    nextInodeNumber = currentEntry->inum;
                    isFound = true;
                    break;
                }
                //increment current entry
                currentEntry = (struct dir_entry *)((char *)currentEntry + sizeof(struct dir_entry));
                totalSize += sizeof(struct dir_entry);
            }
            free(currentBlock);
            currentBlock = getNthBlock(inode, ++i);
        }
    }
    if (inode->type == INODE_REGULAR) {
        return ERROR;
    }
    if (inode->type == INODE_SYMLINK) {
        return ERROR;
    }
    free(block);
    char *nextPath = path;
    if (nextInodeNumber == 0) {
        // Return error
        return 0;
    }
    while (nextPath[0] != '/') {
        // base case
        if (nextPath[0] == '\0') {
            return nextInodeNumber;
        }
        nextPath += sizeof(char);
    }
    nextPath += sizeof(char);
    TracePrintf(1, "About to make recursive call\n");
    return getInodeNumberForPath(nextPath, nextInodeNumber);
}


/*
 * Set inode to free and add it to the list
 */
void
freeUpInode(int inodeNum) {
    // number of inodes per block

    void *block = getBlockForInode(inodeNum);
    
    // get address of this inode 
    struct inode *inode = getInode(block, inodeNum);
    
    TracePrintf(1, "freeing up inode %d, with type %d\n", inodeNum, inode->type);
    // modify the type of inode to free
    inode->type = INODE_FREE;
    // TODO: remove this free
    free(block);
    // add it to linked list
    addFreeInodeToList(inodeNum);
    
    // TODO mark inode as dirty
    // TODO mark block as dirty
}

int
getNextFreeInodeNum() {
    if (firstFreeInode == NULL) {
        return 0;
    }
    int inodeNum = firstFreeInode->inodeNumber;
    firstFreeInode = firstFreeInode->next;
    return inodeNum;
}

void
addFreeInodeToList(int inodeNum) {
    freeInode *newHead = malloc(sizeof(freeInode));
    newHead->inodeNumber = inodeNum;
    newHead->next = firstFreeInode;
    firstFreeInode = newHead;
    freeInodeCount++;
}

struct inode*
getInode(void *blockAddr,  int inodeNum) {
    int blockNum = (inodeNum / INODESPERBLOCK) + 1;
    return (blockAddr + (inodeNum - (blockNum - 1) * INODESPERBLOCK) * INODESIZE);
}

void
buildFreeInodeList() {
    
    int blockNum = 1;
    void *block = getBlock(blockNum);
    
    struct fs_header header = *((struct fs_header*) block);
    
    TracePrintf(1, "num_blocks: %d, num_inodes: %d\n", header.num_blocks,
        header.num_inodes);
    
    // for each block that contains inodes
    int inodeNum = ROOTINODE;
    while (inodeNum < header.num_inodes) {
        // for each inode, if it's free, add it to the free list
        for (; inodeNum < INODESPERBLOCK * blockNum; inodeNum++) {
            struct inode *inode = getInode(block, inodeNum);
            if (inode->type == INODE_FREE) {
                addFreeInodeToList(inodeNum);
            }
        }
        blockNum++;
        free(block);
        block = getBlock(blockNum);
    }
    TracePrintf(1, "initialized free inode list with %d free inodes\n", 
        freeInodeCount);
    free(block);
}

void
clearFile(struct inode *inode) {
    //TODO iterate through blocks and add them to free block list
    
    inode->size = 0;
}


/*
 * Returns offset within blocknum block
 */
int
getDirectoryEntry(char *pathname, int inodeStart, int *blockNum) {
    (void)pathname;
    (void)inodeStart;
    (void)blockNum;
    return NULL;
}

int
yfsOpen(char *pathname, int currentInode) {
    if (pathname[0] == '/') {
        pathname += sizeof(char);
        currentInode = ROOTINODE;
    }
    return getInodeNumberForPath(pathname, currentInode);
}

int
yfsCreate(char *pathname, int currentInode) {
    if (pathname[0] == '/') {
        pathname += sizeof(char);
        currentInode = ROOTINODE;
    }
    
    // Truncate the pathname to NOT include the name of the file to create
    int lastSlashIndex = 0;
    int i = 0;
    while ((pathname[i] != '\0') && (i < MAXPATHNAMELEN)) {
        if (pathname[i] == '/') {
            lastSlashIndex = i;
        }
    }
    
    char path[lastSlashIndex + 1];
    for (i = 0; i < lastSlashIndex; i++) {
        path[i] = pathname[i];
    }
    path[i] = '\0';
    
    char *filename = pathname + (sizeof(char) * (lastSlashIndex + 1));
    
    // Get the inode of the directory the file should be created in
    int dirInodeNum = getInodeNumberForPath(&path, currentInode);
    if (dirInodeNum == 0) {
        return 0;
    }
    
    // Search all directory entries of that inode for the file name to create
    int blockNum;
    int offset = getDirectoryEntry(filename, dirInodeNum, &blockNum);
    void *block = getBlock(blockNum);
    
    dir_entry *dir_entry = (dir_entry *) (block + (char *)offset);
    
    // If the file exists, get the inode, set its size to zero, and return
    // that inode number to user
    int inodeNum = dir_entry->inum;
    if (inodeNum != 0) {
        int inodeNum = dir_entry->inum;
        void *block = getBlockForInode(inodeNum);
        struct inode *inode = getInode(block, inodeNum);
        clearFile(inode);
        
        // TODO have method to calculate block number?
        saveBlock((inodeNum / INODESPERBLOCK) + 1, block);
        free(block);
        return inodeNum;
    }
    
    // If the file does not exist, find the first free directory entry, get
    // a new inode number from free list, get that inode, change the info on 
    // that inode and directory entry (name, type), then return the inode number
    inodeNum = getNextFreeInodeNum();
    void *block = getBlockForInode(inodeNum);
    struct inode *inode = getInode(block, inodeNum);
    inode->type = INODE_REGULAR;
    inode->size = 0;
    saveBlock((inodeNum / INODESPERBLOCK) + 1, block);
    free(block);
    return inodeNum;
}

int
main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    init();
    TracePrintf(1, "Num directory entries needed = %d\n", BLOCKSIZE * 13 / sizeof(struct dir_entry));
    TracePrintf(1, "I'm running!\n");
    char *pathName = "/a/b/x.txt";
    int result = yfsOpen(pathName, ROOTINODE);
    TracePrintf(1, "Resulting inode number = %d\n", result);
    return (0);
}