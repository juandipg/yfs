#include <stdbool.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include "yfs.h"

freeInode *firstFreeInode = NULL;
int freeInodeCount = 0;



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
    
    buildFreeInodeList();
    return (0);
}