#include <stdbool.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include "yfs.h"

freeInode *firstFreeInode = NULL;
int freeInodeCount = 0;

//void *globalBlock;

void 
init() {
//    globalBlock = malloc(BLOCKSIZE);
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
        TracePrintf(1, "blockNumber = %d\n", inode->direct[n]);
        return getBlock(inode->direct[n]);
    } 
    //search the direct blocks
    int *indirectBlock = getBlock(inode->indirect);
    return getBlock(indirectBlock[n - NUM_DIRECT]);
}

int
getInodeNumberForPath (char *path, int inodeStartNumber) {
    //get the inode number for the first file in path 
    // ex: if path is "/a/b/c.txt" get the indoe # for "a"
    int nextInodeNumber = 0;
    
    //Get inode corresponding to inodeStartNumber
    void *block = getBlockForInode(inodeStartNumber);
    
    struct inode *inode = getInode(block, inodeStartNumber);
    TracePrintf(1, "inode start number %d\n", inodeStartNumber);
    if (inode->type == INODE_DIRECTORY) {
        int i = 0;
        void *currentBlock = getNthBlock(inode, i);
        TracePrintf(1, "got %dth block\n", i);
        int totalSize = sizeof(struct dir_entry);
        while (currentBlock != NULL) {
            struct dir_entry *currentEntry = (struct dir_entry *)currentBlock;
            TracePrintf(1, "1\n");
            while (totalSize <= inode->size) {
                TracePrintf(1, "2\n");
                //check the currentEntry fileName to see if it matches        
                TracePrintf(1, "currentEntry->name addr %p\n", &currentEntry->name);
                if (isEqual(path, currentEntry->name)) {
                    nextInodeNumber = currentEntry->inum;
                    TracePrintf(1, "3\n");
                    break;
                }
                TracePrintf(1, "4\n");
                //increment current entry
                currentEntry = (struct dir_entry *)((char *)currentEntry + sizeof(struct dir_entry));
                totalSize += sizeof(struct dir_entry);
            }
            TracePrintf(1, "5\n");
            free(currentBlock);
            currentBlock = getNthBlock(inode, ++i);
            TracePrintf(1, "6\n");
        }
    }
    TracePrintf(1, "about to check for type regular\n");
    if (inode->type == INODE_REGULAR) {
        return ERROR;
    }
    if (inode->type == INODE_SYMLINK) {
        return ERROR;
    }
    free(block);
    char *nextPath = path;
    while (nextPath[0] != '/') {
        // base case
        if (nextPath[0] == '\0') {
            return nextInodeNumber;
        }
        nextPath += sizeof(char);
    }
    nextPath += sizeof(char);
    TracePrintf(1, "About to make recursive call\n");
    TracePrintf(1, "Next inode number = %d\n", nextInodeNumber);
    return getInodeNumberForPath(nextPath, nextInodeNumber);
}


/*
 * Set inode to free and add it to the list
 */
void
freeUpInode(int inodeNum) {
    // number of inodes per block
    
    // TODO: get inode from cache or disk ----
    // get Block where this inode is contained
    void *block = getBlockForInode(inodeNum);
    // end TODO ------------------------------
    
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

int
main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
//    init();
    TracePrintf(1, "Num directory entries needed = %d\n", BLOCKSIZE * 13 / sizeof(struct dir_entry));
    TracePrintf(1, "I'm running!\n");
    char *pathName = "/a/b/x.txt";
    int result = getInodeNumberForPath(pathName + sizeof(char), ROOTINODE);
    TracePrintf(1, "Resulting inode number = %d\n", result);
    return (0);
}