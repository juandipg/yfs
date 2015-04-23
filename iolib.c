//#include <string.h>
#include <stdlib.h>
#include <comp421/iolib.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include "message.h"

struct open_file {
    int inodenum;
    int position;
};
struct open_file * file_table[MAX_OPEN_FILES] = {NULL};
int files_open = 0;
int current_inode = ROOTINODE;

static int
getLenForPath(char *pathname)
{
    int i;
    for (i = 0; i < MAXPATHNAMELEN; i++) {
        if (pathname[i] == '\0') {
            break;
        }
    }
    if (i == 0 || i == MAXPATHNAMELEN) {
        TracePrintf(1, "invalid pathname\n");
        return ERROR;
    }
    return i + 1;
}

static int
addFile(int inodenum)
{
    int fd;
    for (fd = 0; fd < MAX_OPEN_FILES; fd++) {
        if (file_table[fd] == NULL) {
            break;
        }
    }
    if (fd == MAX_OPEN_FILES) {
        TracePrintf(1, "file table full\n");
        return ERROR;
    }
    file_table[fd] = malloc(sizeof (struct open_file));
    if (file_table[fd] == NULL) {
        TracePrintf(1, "error allocating space for open file\n");
    }
    file_table[fd]->inodenum = inodenum;
    file_table[fd]->position = 0;
    return fd;
}

static int
removeFile(int fd)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES
            || file_table[fd] == NULL) 
    {
        return ERROR;
    }
    free(file_table[fd]);
    file_table[fd] = NULL;
    return 0;
}

static int
sendPathMessage(int operation, char *pathname)
{
    int len = getLenForPath(pathname);
    if (len == ERROR) {
        return ERROR;
    }
    struct message_path * msg = malloc(sizeof(struct message_path));
    if (msg == NULL) {
        TracePrintf(1, "error allocating space for path message\n");
        return ERROR;
    }
    msg->num = operation;
    msg->current_inode = current_inode;
    msg->pathname = pathname;
    msg->len = len;
    if (Send(msg, -FILE_SERVER) != 0) {
        TracePrintf(1, "error sending message to server\n");
        free(msg);
        return ERROR;
    }
    // msg gets overwritten with reply message after return from Send
    int inodenum = msg->num;
    free(msg);
    return inodenum;
}

int
Open(char *pathname)
{
    int inodenum = sendPathMessage(YFS_OPEN, pathname);
    if (inodenum == ERROR) {
        TracePrintf(1, "received error from server\n");
        return ERROR;
    }
    // try to add a file to the array and return fd or error
    TracePrintf(2, "inode num %d\n", inodenum);
    return addFile(inodenum);
}

int
Close(int fd)
{
    return removeFile(fd);
}

int
Create(char *pathname)
{
    int inodenum = sendPathMessage(YFS_CREATE, pathname);
    if (inodenum == ERROR) {
        TracePrintf(1, "received error from server\n");
        return ERROR;
    }
    // try to add a file to the array and return fd or error
    TracePrintf(2, "inode num %d\n", inodenum);
    return addFile(inodenum);
}

int
Read(int fd, void *buf, int size)
{
    (void) fd;
    (void) buf;
    (void) size;
    return 0;
}

int
Write(int fd, void *buf, int size)
{
    (void) fd;
    (void) buf;
    (void) size;
    return 0;
}

int
Seek(int fd, int offset, int whence)
{
    (void) fd;
    (void) offset;
    (void) whence;
    return 0;
}

int
Link(char *oldname, char *newname)
{
    (void) oldname;
    (void) newname;
    return 0;
}

int
Unlink(char *pathname)
{
    int code = sendPathMessage(YFS_UNLINK, pathname);
    if (code == ERROR) {
        TracePrintf(1, "received error from server\n");
    }
    return code;
}

int
SymLink(char *oldname, char *newname)
{
    (void) oldname;
    (void) newname;
    return 0;
}

int
ReadLink(char *pathname, char *buf, int len)
{
    (void) pathname;
    (void) buf;
    (void) len;
    return 0;
}

int
MkDir(char *pathname)
{
    int code = sendPathMessage(YFS_MKDIR, pathname);
    if (code == ERROR) {
        TracePrintf(1, "received error from server\n");
    }
    return code;
}

int
RmDir(char *pathname)
{
    int code = sendPathMessage(YFS_RMDIR, pathname);
    if (code == ERROR) {
        TracePrintf(1, "received error from server\n");
    }
    return code;
}

int
ChDir(char *pathname)
{
    int inodenum = sendPathMessage(YFS_CHDIR, pathname);
    if (inodenum == ERROR) {
        TracePrintf(1, "received error from server\n");
        return ERROR;
    }
    current_inode = inodenum;
    return 0;
}

int
Stat(char *pathname, struct Stat *statbuf)
{
    (void) pathname;
    (void) statbuf;
    return 0;
}

int
Sync()
{
    return 0;
}

int
Shutdown()
{
    return 0;
}
