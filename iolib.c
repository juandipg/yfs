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

int
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

int
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

int
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

struct message_path *
createPathMessage(int operation, char *pathname)
{
    int len = getLenForPath(pathname);
    if (len == ERROR) {
        return NULL;
    }
    struct message_path * msg = malloc(sizeof(struct message_path));
    if (msg == NULL) {
        TracePrintf(1, "error allocating space for path message\n");
        return NULL;
    }
    msg->num = operation;
    msg->current_inode = current_inode;
    msg->pathname = pathname;
    msg->len = len;
    return msg;
}

int
Open(char *pathname)
{
    (void) pathname;
    return 0;
}

int
Close(int fd)
{
    (void) fd;
    return 0;
}

int
Create(char *pathname)
{
    // TODO: store message in the stack instead?
    struct message_path * msg = createPathMessage(YFS_CREATE, pathname);
    if (msg == NULL) {
        return ERROR;
    }
    if (Send(msg, -FILE_SERVER) != 0) {
        TracePrintf(1, "error sending message to server\n");
        free(msg);
        return ERROR;
    }
    // msg gets overwritten with reply message after return from Send
    int inodenum = msg->num;
    free(msg);
    if (inodenum == ERROR) {
        TracePrintf(1, "received error from server\n");
        return ERROR;
    }
    // try to add a file to the array and return fd or error
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
    (void) pathname;
    return 0;
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
    (void) pathname;
    return 0;
}

int
RmDir(char *pathname)
{
    (void) pathname;
    return 0;
}

int
ChDir(char *pathname)
{
    (void) pathname;
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
