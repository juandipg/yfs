#include <stdbool.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include "yfs.h"

freeInode *firstFreeInode = NULL;
int freeInodeCount = 0;

bool
isEqual(char *path, char dirEntryName[]) {
    int i = 0;
    while (i < DIRNAMELEN) {
        TracePrintf(1, "comparing %c to %c\n", path[i], dirEntryName[i]);
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

int
getInodeNumberForPath (char *path, int inodeStartNumber) {
    //get the inode number for the first file in path 
    // ex: if path is "/a/b/c.txt" get the indoe # for "a"
    int nextInodeNumber = 0;
    
    //Get inode corresponding to inodeStartNumber
    int blockNumber = (inodeStartNumber / INODESPERBLOCK) + 1;
    void *block = malloc(BLOCKSIZE);
    ReadSector(blockNumber, block);
    TracePrintf(1, "Read sector: %d\n", blockNumber);
    
    struct inode *inode = getInode(block, blockNumber, inodeStartNumber);
    TracePrintf(1, "Got inode of type: %d\n", inode->type);
    
    if (inode->type == INODE_DIRECTORY) {
        TracePrintf(1, "inode directory\n");
        int dirBlockNum = inode->direct[0];
        void *dirBlock = malloc(BLOCKSIZE);
        ReadSector(dirBlockNum, dirBlock);
        
        TracePrintf(1, "read sector for inode. Sector number: %d\n", dirBlockNum);
        
        int totalSize = sizeof(struct dir_entry);
        struct dir_entry *currentEntry = (struct dir_entry *)dirBlock;
        while (totalSize <= inode->size) {
            //check the currentEntry fileName to see if it matches
            TracePrintf(1, "checking to see if names are equal\n");
            TracePrintf(1, "current entry's inode num = %d\n", currentEntry->inum);
            if (isEqual(path, currentEntry->name)) {
                
                nextInodeNumber = currentEntry->inum;
                TracePrintf(1, "Just set next inode number to: %d\n", nextInodeNumber);
                break;
            }
            
            //increment current entry
            currentEntry = (struct dir_entry *)((char *)currentEntry + sizeof(struct dir_entry));
            totalSize += sizeof(struct dir_entry);
        }
    }
    if (inode->type == INODE_REGULAR) {
        return ERROR;
    }
    if (inode->type == INODE_SYMLINK) {
        TracePrintf(1, "Symlinks not implemented yet!");
        return ERROR;
    }
    
    char *nextPath = path;
    while (nextPath[0] != '/') {
        // base case
        TracePrintf(1, "path char: %c\n", nextPath[0]);
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
    int blockNum = (inodeNum / INODESPERBLOCK) + 1;
    void *block = malloc(BLOCKSIZE);
    ReadSector(blockNum, block);
    // end TODO ------------------------------
    
    // get address of this inode 
    struct inode *inode = getInode(block, blockNum, inodeNum);
    
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
getInode(void *blockAddr, int blockNum, int inodeNum) {
    return (blockAddr + (inodeNum - (blockNum - 1) * INODESPERBLOCK) * INODESIZE);
}

void
buildFreeInodeList() {
    
    void *block = malloc(BLOCKSIZE);
    int blockNum = 1;
    ReadSector(1, block);
    
    struct fs_header header = *((struct fs_header*) block);
    
    TracePrintf(1, "num_blocks: %d, num_inodes: %d\n", header.num_blocks,
        header.num_inodes);
    
    // for each block that contains inodes
    int inodeNum = ROOTINODE;
    while (inodeNum < header.num_inodes) {
        // for each inode, if it's free, add it to the free list
        for (; inodeNum < INODESPERBLOCK * blockNum; inodeNum++) {
            struct inode *inode = getInode(block, blockNum, inodeNum);
            if (inode->type == INODE_FREE) {
                addFreeInodeToList(inodeNum);
            }
        }
        blockNum++;
        ReadSector(blockNum, block);
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
    
    TracePrintf(1, "I'm running!\n");
    char *pathName = "/a/b/x.txt";
    int result = getInodeNumberForPath(pathName + sizeof(char), ROOTINODE);
    TracePrintf(1, "Resulting inode number = %d\n", result);
    return (0);
}