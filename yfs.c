#include <stdbool.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include <string.h>
#include "yfs.h"

freeInode *firstFreeInode = NULL;
freeBlock *firstFreeBlock = NULL;

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
int
getNthBlock(struct inode *inode, int n) {
    if ((n*BLOCKSIZE > inode->size)
            || (n > NUM_DIRECT + BLOCKSIZE / (int)sizeof(int))) 
    {
        return 0;
    }
    if (n < NUM_DIRECT) {
        return inode->direct[n];
    } 
    //search the direct blocks
    int *indirectBlock = getBlock(inode->indirect);
    int blockNum = indirectBlock[n - NUM_DIRECT];
    free(indirectBlock);
    return blockNum;
}

/**
 * 
 * @param path the file path to get the inode number for
 * @param inodeStartNumber the inode to start looking for the next part
 *       of the file path in
 * @return the inode number of the path, or 0 if it's an invalid path
 */
int
getInodeNumberForPath(char *path, int inodeStartNumber)
{
    //get the inode number for the first file in path 
    // ex: if path is "/a/b/c.txt" get the indoe # for "a"
    int nextInodeNumber = 0;

    //Get inode corresponding to inodeStartNumber
    void *block = getBlockForInode(inodeStartNumber);
    struct inode *inode = getInode(block, inodeStartNumber);
    if (inode->type == INODE_DIRECTORY) {
        // go get the directory entry in this directory
        // that has that name
        int blockNum;
        int offset = getDirectoryEntry(path, inodeStartNumber, &blockNum, false);
        if (offset != -1) {
            free(block);
            block = getBlock(blockNum);
            struct dir_entry * dir_entry = (struct dir_entry *) ((char *) block + offset);
            nextInodeNumber = dir_entry->inum;
            free(block);
        }
    } else if (inode->type == INODE_REGULAR) {
        free(block);
        return ERROR;
    } else if (inode->type == INODE_SYMLINK) {
        free(block);
        return ERROR;
    }
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
        nextPath += sizeof (char);
    }
    nextPath += sizeof (char);
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
buildFreeBlockList() {
    // For each inode, if it is not free
        // 
    
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
getDirectoryEntry(char *pathname, int inodeStartNumber, int *blockNumPtr, bool createIfNeeded) {
    int freeEntryOffset = -1;
    int freeEntryBlockNum = 0;
    void * currentBlock;
    struct dir_entry *currentEntry;
    
    int inodeBlockNumber = (inodeStartNumber / INODESPERBLOCK) + 1;
    void *block = getBlock(inodeBlockNumber);
    
    struct inode *inode = getInode(block, inodeStartNumber);
    int i = 0;
    int blockNum = getNthBlock(inode, i);
    int currBlockNum = 0;
    int totalSize = sizeof (struct dir_entry);
    bool isFound = false;
    while (blockNum != 0 && !isFound) {
        currentBlock = getBlock(blockNum);
        currentEntry = (struct dir_entry *) currentBlock;
        while (totalSize <= inode->size 
                && ((char *) currentEntry < ((char *) currentBlock + BLOCKSIZE))) 
        {
            if (freeEntryOffset == -1 && currentEntry->inum == 0) {
                freeEntryBlockNum = blockNum;
                freeEntryOffset = (int)((char *)currentEntry - (char *)currentBlock);
            }
            //check the currentEntry fileName to see if it matches 
            if (isEqual(pathname, currentEntry->name)) {
                isFound = true;
                break;
            }
            //increment current entry
            currentEntry = (struct dir_entry *) ((char *) currentEntry + sizeof (struct dir_entry));
            totalSize += sizeof (struct dir_entry);
        }
        free(currentBlock);
        if (isFound) {
            break;
        }
        currBlockNum = blockNum;
        blockNum = getNthBlock(inode, ++i);
    }
    
    *blockNumPtr = blockNum;

    if (isFound) {
        int offset = (int)((char *)currentEntry - (char *)currentBlock);
        free(block);
        return offset;
    } 
    if (createIfNeeded) {
        if (freeEntryBlockNum != 0) {
            *blockNumPtr = freeEntryBlockNum;
            free(block);
            return freeEntryOffset;
        }
        if (DIRENTRIESPERBLOCK % inode->size == 0) {
            // we need to allocate a new block
                //TODO
            inode->size += sizeof(struct dir_entry);
            free(block);
            return -1;
        } 
        inode->size += sizeof(struct dir_entry);
        saveBlock(inodeBlockNumber, block);
        *blockNumPtr = currBlockNum;
        int offset = (int)((char *)currentEntry - (char *)currentBlock);
        free(block);
        return offset;
    }

    return -1;
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
    // error checking
    int i;
    for (i = 0; i < MAXPATHNAMELEN; i++) {
        if (pathname[i] == '\0') {
            break;
        }
    }
    if (i == MAXPATHNAMELEN) {
        return ERROR;
    }
    
    // adjust pathname
    if (pathname[0] == '/') {
        pathname += sizeof(char);
        currentInode = ROOTINODE;
    }
    
    // Truncate the pathname to NOT include the name of the file to create
    int lastSlashIndex = 0;
    i = 0;
    while ((pathname[i] != '\0') && (i < MAXPATHNAMELEN)) {
        if (pathname[i] == '/') {
            lastSlashIndex = i;
        }
        i++;
    }
    
    char path[lastSlashIndex + 1];
    for (i = 0; i < lastSlashIndex; i++) {
        path[i] = pathname[i];
    }
    path[i] = '\0';
    
    char *filename = pathname + (sizeof(char) * (lastSlashIndex + 1));
    
    // Get the inode of the directory the file should be created in
    int dirInodeNum = getInodeNumberForPath(path, currentInode);
    if (dirInodeNum == 0) {
        return 0;
    }
    
    // Search all directory entries of that inode for the file name to create
    int blockNum;
    int offset = getDirectoryEntry(filename, dirInodeNum, &blockNum, true);
    void *block = getBlock(blockNum);
    
    struct dir_entry *dir_entry = (struct dir_entry *) ((char *)block + offset);
    
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
    memset(&dir_entry->name, '\0', MAXPATHNAMELEN);
    for (i = 0; filename[i] != '\0'; i++) {
        dir_entry->name[i] = filename[i];
    }
    inodeNum = getNextFreeInodeNum();
    dir_entry->inum = inodeNum;
    saveBlock(blockNum, block);
    block = getBlockForInode(inodeNum);
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
    char *pathName = "a/b/w.txt";
    int result = yfsCreate(pathName, ROOTINODE);
    TracePrintf(1, "Resulting inode number = %d\n", result);
    return (0);
}