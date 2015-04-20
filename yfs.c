#include <stdbool.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include <string.h>
#include "yfs.h"

freeInode *firstFreeInode = NULL;
freeBlock *firstFreeBlock = NULL;

int freeInodeCount = 0;
int freeBlockCount = 0;
int currentInode = ROOTINODE;


void 
init() {
    buildFreeInodeAndBlockLists();
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
getNthBlock(struct inode *inode, int n, bool allocateIfNeeded) {
    bool isOver = false;
    if (n >= NUM_DIRECT + BLOCKSIZE / (int)sizeof(int)) {
        return 0;
    }
    if (n*BLOCKSIZE >= inode->size) 
    {
        isOver = true;
    }
    if (isOver && !allocateIfNeeded) {
        return 0;
    }
    if (n < NUM_DIRECT) {
        if (isOver) {
            TracePrintf(1, "I'm allocating a new block!\n");
            inode->direct[n] = getNextFreeBlockNum();
        }
        // if getNextFreeBlockNum returned 0, return 0
        return inode->direct[n];
    } 
    //search the direct blocks
    int *indirectBlock = getBlock(inode->indirect);
    if (isOver) {
        // if getNextFreeBlockNum returned 0, return 0
        indirectBlock[n - NUM_DIRECT] = getNextFreeBlockNum();
    }
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

int getNextFreeBlockNum() {
    if (firstFreeBlock == NULL) {
        return 0;
    }
    int blockNum = firstFreeBlock->blockNumber;
    firstFreeBlock = firstFreeBlock->next;
    return blockNum;
}

void
addFreeBlockToList(int blockNum) {
    freeBlock *newHead = malloc(sizeof(freeBlock));
    newHead->blockNumber = blockNum;
    newHead->next = firstFreeBlock;
    firstFreeBlock = newHead;
    freeBlockCount++;
}

struct inode*
getInode(void *blockAddr,  int inodeNum) {
    int blockNum = (inodeNum / INODESPERBLOCK) + 1;
    return (blockAddr + (inodeNum - (blockNum - 1) * INODESPERBLOCK) * INODESIZE);
}

void
buildFreeInodeAndBlockLists() {
    
    int blockNum = 1;
    void *block = getBlock(blockNum);
    
    struct fs_header header = *((struct fs_header*) block);
    
    TracePrintf(1, "num_blocks: %d, num_inodes: %d\n", header.num_blocks,
        header.num_inodes);
    
    // create array indexed by block number
    bool takenBlocks[header.num_blocks];
    // initialize each item to false
    memset(takenBlocks, false, header.num_blocks * sizeof(bool));
    // sector 0 is taken
    takenBlocks[0] = true;
    
    // for each block that contains inodes
    int inodeNum = ROOTINODE;
    while (inodeNum < header.num_inodes) {
        // for each inode, if it's free, add it to the free list
        for (; inodeNum < INODESPERBLOCK * blockNum; inodeNum++) {
            struct inode *inode = getInode(block, inodeNum);
            if (inode->type == INODE_FREE) {
                addFreeInodeToList(inodeNum);
            } else {
                // keep track of all these blocks as taken
                int i = 0;
                int blockNum;
                while((blockNum = getNthBlock(inode, i++, false)) != 0) {
                    takenBlocks[blockNum] = true;
                }
            }
        }
        blockNum++;
        free(block);
        block = getBlock(blockNum);
    }
    TracePrintf(1, "initialized free inode list with %d free inodes\n", 
        freeInodeCount);
    
    // for each element in the block array
    int i;
    for (i = 0; i < header.num_blocks; i++) {
        if (!takenBlocks[i]) {
            // add block to list
            addFreeBlockToList(i);
        }
    }
    TracePrintf(1, "initialized free block list with %d free blocks\n", 
        freeBlockCount);
    
    free(block);
}

void
clearFile(struct inode *inode, int inodeNum, void *block) {
    int i = 0;
    int blockNum;
    while ((blockNum = getNthBlock(inode, i++, false)) != 0) {
        addFreeBlockToList(blockNum);
    }
    inode->size = 0;
    saveBlock((inodeNum / INODESPERBLOCK) + 1, block);
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
    int blockNum = getNthBlock(inode, i, false);
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
        blockNum = getNthBlock(inode, ++i, false);
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
        if (inode->size % BLOCKSIZE == 0) {
            // we're at the bottom edge of the block, so
            // we need to allocate a new block
            blockNum = getNthBlock(inode, i, true);
            currentBlock = getBlock(blockNum);
            inode->size += sizeof(struct dir_entry);
            struct dir_entry * newEntry = (struct dir_entry *) currentBlock;
            newEntry->inum = 0;
            saveBlock(blockNum, currentBlock);
            saveBlock(inodeBlockNumber, block);
            *blockNumPtr = blockNum;
            free(block);
            free(currentBlock);
            return 0;
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
getContainingDirectory(char *pathname, int currentInode, char **filenamePtr) {
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
    *filenamePtr = filename;
    
    // Get the inode of the directory the file should be created in
    int dirInodeNum = getInodeNumberForPath(path, currentInode);
    if (dirInodeNum == 0) {
        return ERROR;
    }
    return dirInodeNum;
}

int
yfsCreate(char *pathname, int currentInode, int inodeNumToSet) {

    char *filename;
    int dirInodeNum = getContainingDirectory(pathname, currentInode, &filename);
    // Search all directory entries of that inode for the file name to create
    int blockNum;
    int offset = getDirectoryEntry(filename, dirInodeNum, &blockNum, true);
    void *block = getBlock(blockNum);
    
    struct dir_entry *dir_entry = (struct dir_entry *) ((char *)block + offset);
    
    // If the file exists, get the inode, set its size to zero, and return
    // that inode number to user
    int inodeNum = dir_entry->inum;
    if (inodeNum != 0) {
        if (inodeNumToSet != -1) {
            return ERROR;
        }
        int inodeNum = dir_entry->inum;
        void *block = getBlockForInode(inodeNum);
        struct inode *inode = getInode(block, inodeNum);
        clearFile(inode, inodeNum, block);
        
        // TODO have method to calculate block number?
        saveBlock((inodeNum / INODESPERBLOCK) + 1, block);
        free(block);
        return inodeNum;
    }
    
    // If the file does not exist, find the first free directory entry, get
    // a new inode number from free list, get that inode, change the info on 
    // that inode and directory entry (name, type), then return the inode number
    memset(&dir_entry->name, '\0', MAXPATHNAMELEN);
    int i;
    for (i = 0; filename[i] != '\0'; i++) {
        dir_entry->name[i] = filename[i];
    }
    
    if (inodeNumToSet == -1) {
        inodeNum = getNextFreeInodeNum();
        dir_entry->inum = inodeNum;
        saveBlock(blockNum, block);
        block = getBlockForInode(inodeNum);
        struct inode *inode = getInode(block, inodeNum);
        inode->type = INODE_REGULAR;
        inode->size = 0;
        inode->nlink = 1;
        saveBlock((inodeNum / INODESPERBLOCK) + 1, block);
        free(block);
        return inodeNum;
    } else {
        dir_entry->inum = inodeNumToSet;
        saveBlock(blockNum, block);
        free(block);
        return inodeNumToSet;
    }
}

int
yfsRead(int inodeNum, void *buf, int size, int byteOffset, int pid) {
    (void)pid;
    void *inodeBlock = getBlockForInode(inodeNum);
    struct inode *inode = getInode(inodeBlock, inodeNum);
    
    if (byteOffset > inode->size) {
        return ERROR;
    }
    
    int bytesLeft = size;
    if (inode->size - byteOffset < size) {
        bytesLeft = inode->size - byteOffset;
    }
    
    int returnVal = bytesLeft;
    
    int blockOffset = byteOffset % BLOCKSIZE;

    int bytesToCopy = BLOCKSIZE - blockOffset;
    
    int i;
    for (i = byteOffset / BLOCKSIZE; bytesLeft > 0; i++) {
        int blockNum = getNthBlock(inode, i, false);
        if (blockNum == 0) {
            return ERROR;
        }
        void *currentBlock = getBlock(blockNum);
        
        if (bytesLeft < bytesToCopy) {
            bytesToCopy = bytesLeft;
        }
        
        memcpy(buf, (char *)currentBlock + blockOffset, bytesToCopy);
        
        buf += bytesToCopy;
        free(currentBlock);
        blockOffset = 0;
        bytesLeft -= bytesToCopy;
        bytesToCopy = BLOCKSIZE;
    }
    
    return returnVal;
}

int 
yfsWrite(int inodeNum, void *buf, int size, int byteOffset, int pid) {
    (void)pid;
    void *inodeBlock = getBlockForInode(inodeNum);
    struct inode *inode = getInode(inodeBlock, inodeNum);
    if (inode->type != INODE_REGULAR) {
        return ERROR;
    }
    
    int bytesLeft = size;
    
    int returnVal = bytesLeft;
    
    int blockOffset = byteOffset % BLOCKSIZE;

    int bytesToCopy = BLOCKSIZE - blockOffset;
    
    int i;
    for (i = byteOffset / BLOCKSIZE; bytesLeft > 0; i++) {
        int blockNum = getNthBlock(inode, i, true);
        if (blockNum == 0) {
            return ERROR;
        }
        void *currentBlock = getBlock(blockNum);
        
        if (bytesLeft < bytesToCopy) {
            bytesToCopy = bytesLeft;
        }
        
        memcpy((char *)currentBlock + blockOffset, buf, bytesToCopy);

        buf += bytesToCopy;
        saveBlock(blockNum, currentBlock);
        
        free(currentBlock);
        blockOffset = 0;
        bytesLeft -= bytesToCopy;
        bytesToCopy = BLOCKSIZE;
        int bytesWrittenSoFar = size - bytesLeft;
        if (bytesWrittenSoFar + byteOffset > inode->size) {
            inode->size = bytesWrittenSoFar + byteOffset;
        }
    }
    int blockNumber = (inodeNum / INODESPERBLOCK) + 1;
    saveBlock(blockNumber, inodeBlock);
    return returnVal;
}

int
yfsLink(char *oldName, char *newName, int currentInode) {
    if (oldName[0] == '/') {
        oldName += sizeof(char);
        currentInode = ROOTINODE;
    }
    
    int oldNameNodeNum = getInodeNumberForPath(oldName, currentInode);
    void *oldNodeBlock = getBlockForInode(oldNameNodeNum);
    struct inode *inode = getInode(oldNodeBlock, oldNameNodeNum);
    if (inode->type == INODE_DIRECTORY || oldNameNodeNum == 0) {
        return ERROR;
    }
    
    if (newName[0] == '/') {
        newName += sizeof(char);
        currentInode = ROOTINODE;
    }
    if (yfsCreate(newName, currentInode, oldNameNodeNum) == ERROR) {
        return ERROR;
    }
    inode->nlink++;
    TracePrintf(1, "just linked, new inode nlink count is: %d\n", inode->nlink);
    saveBlock((oldNameNodeNum / INODESPERBLOCK) + 1, oldNodeBlock);
    
    return 0;
}

int
yfsUnlink(char *pathname, int currentInode) {
    // Get the containind directory 
    char *filename;
    int dirInodeNum = getContainingDirectory(pathname, currentInode, &filename);

    int blockNum;
    int offset = getDirectoryEntry(filename, dirInodeNum, &blockNum, true);
    void *block = getBlock(blockNum);

    // Get the directory entry associated with the path
    struct dir_entry *dir_entry = (struct dir_entry *) ((char *)block + offset);
    
    // Get the inode associated with the directory entry
    int inodeNum = dir_entry->inum;
    void *inodeBlock = getBlockForInode(inodeNum);
    struct inode *inode = getInode(inodeBlock, inodeNum);
    
    // Decrease nlinks by 1
    inode->nlink--;
     TracePrintf(1, "just unlinked, new inode nlink count is: %d\n", inode->nlink);
    
    // If nlinks == 0, clear the file
    if (inode->nlink == 0) {
        TracePrintf(1, "about to clear the file!\n");
        clearFile(inode, inodeNum, inodeBlock);
    } 
    
    saveBlock((inodeNum / INODESPERBLOCK) + 1, inodeBlock);
    
    // Set the inum to zero
    dir_entry->inum = 0;
    saveBlock(blockNum, block);
    
    return 0;
}

int
main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    init();
    TracePrintf(1, "Num directory entries needed = %d\n", BLOCKSIZE * 13 / sizeof(struct dir_entry));
    TracePrintf(1, "I'm running!\n");
    
    int i;
    char hello[612];
    for (i = 0; i < 612; i++) 
    {
        if (i % 2 == 0) {
            hello[i] = '2';
        } else {
            hello[i] = '1';
        }
    }
    hello[610] = 'a';
    hello[611] = 'b';
    
    char *writeMe = "abcdefghijklmnopqrstuvwxyz\n";
    int writeResult = yfsWrite(20, hello, 612, 0, 0);
    TracePrintf(1, "Bytes written = %d\n", writeResult);
    int writeResult2 = yfsWrite(20, writeMe, 26, 500, 0);
    TracePrintf(1, "Bytes written = %d\n", writeResult2);
    
    int linkResult = yfsLink("/a/b/x.txt", "/a/b/z.txt", ROOTINODE);
    TracePrintf(1, "Link result = %d\n", linkResult);
    
    int unlinkResult = yfsUnlink("/a/b/z.txt", ROOTINODE);
    TracePrintf(1, "unlink result = %d\n", unlinkResult);
    
    int ztxtInode = getInodeNumberForPath("a/b/z.txt", ROOTINODE);
    
    char *readMe = malloc(612*sizeof(char));
    int readResult = yfsRead(ztxtInode, readMe, 638, 0, 0);
    TracePrintf(1, "Bytes read = %d\n", readResult);
    TracePrintf(1, "String read = %s\n", readMe);
    
    int unlinkResult2 = yfsUnlink("/a/b/x.txt", ROOTINODE);
    TracePrintf(1, "unlink result = %d\n", unlinkResult2);
    
    char *readMe2 = malloc(612*sizeof(char));
    readResult = yfsRead(20, readMe2, 638, 0, 0);
    TracePrintf(1, "Bytes read = %d\n", readResult);
    TracePrintf(1, "String read = %s\n", readMe2);
    return (0);
}