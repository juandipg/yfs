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

struct open_file * getFile(int fd);

static int
getLenForPath(char *pathname)
{
    if (pathname == NULL) {
        return ERROR;
    }
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
    struct open_file * file = getFile(fd);
    if (file == NULL) {
        return ERROR;
    }
    free(file);
    file_table[fd] = NULL;
    return 0;
}

struct open_file *
getFile(int fd)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES) 
    {
        return NULL;
    }
    return file_table[fd];
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
    int code = msg->num;
    free(msg);
    return code;
}

static int
sendFileMessage(int operation, int inodenum, void *buf, int size, int offset)
{
    if (size < 0 || buf == NULL) {
        return ERROR;
    }
    struct message_file * msg = malloc(sizeof(struct message_file));
    if (msg == NULL) {
        TracePrintf(1, "error allocating space for file message\n");
        return ERROR;
    }
    msg->num = operation;
    msg->inodenum = inodenum;
    msg->buf = buf;
    msg->size = size;
    msg->offset = offset;
    if (Send(msg, -FILE_SERVER) != 0) { // TODO: factor this out to a sendMessage() method?
        TracePrintf(1, "error sending message to server\n");
        free(msg);
        return ERROR;
    }
    int code = msg->num;
    free(msg);
    return code;
}

static int
sendLinkMessage(int operation, char *oldname, char *newname)
{
    int oldlen = getLenForPath(oldname);
    if (oldlen == ERROR) {
        return ERROR;
    }
    int newlen = getLenForPath(newname);
    if (newlen == ERROR) {
        return ERROR;
    }
    struct message_link * msg = malloc(sizeof(struct message_link));
    if (msg == NULL) {
        TracePrintf(1, "error allocating space for path message\n");
        return ERROR;
    }
    msg->num = operation;
    msg->current_inode = current_inode;
    msg->old_name = oldname;
    msg->new_name = newname;
    msg->old_len = oldlen;
    msg->new_len = newlen;
    if (Send(msg, -FILE_SERVER) != 0) {
        TracePrintf(1, "error sending message to server\n");
        free(msg);
        return ERROR;
    }
    // msg gets overwritten with reply message after return from Send
    int code = msg->num;
    free(msg);
    return code;
}

static int
sendReadLinkMessage(char *pathname, char *buf, int len)
{
    if (buf == NULL || len < 0) {
        return ERROR;
    }
    int path_len = getLenForPath(pathname);
    if (path_len == ERROR) {
        return ERROR;
    }
    struct message_read_link * msg = malloc(sizeof(struct message_read_link));
    if (msg == NULL) {
        TracePrintf(1, "error allocating space for read link message\n");
        return ERROR;
    }
    msg->num = YFS_READLINK;
    msg->current_inode = current_inode;
    msg->pathname = pathname;
    msg->path_len = path_len;
    msg->buf = buf;
    msg->len = len;
    if (Send(msg, -FILE_SERVER) != 0) {
        TracePrintf(1, "error sending message to server\n");
        free(msg);
        return ERROR;
    }
    // msg gets overwritten with reply message after return from Send
    int code = msg->num;
    free(msg);
    return code;
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
    struct open_file * file = getFile(fd);
    if (file == NULL) {
        return ERROR;
    }
    int bytes = sendFileMessage(YFS_READ, file->inodenum, buf, size, file->position);
    if (bytes == ERROR) {
        TracePrintf(1, "received error from server\n");
        return ERROR;
    }
    file->position += bytes;
    return bytes;
}

int
Write(int fd, void *buf, int size)
{
    struct open_file * file = getFile(fd);
    if (file == NULL) {
        return ERROR;
    }
    int bytes = sendFileMessage(YFS_WRITE, file->inodenum, buf, size, file->position);
    if (bytes == ERROR) {
        TracePrintf(1, "received error from server\n");
        return ERROR;
    }
    file->position += bytes;
    return bytes;
}

int
Seek(int fd, int offset, int whence)
{
    // TODO
    (void) fd;
    (void) offset;
    (void) whence;
    return 0;
}

int
Link(char *oldname, char *newname)
{
    int code = sendLinkMessage(YFS_LINK, oldname, newname);
    if (code == ERROR) {
        TracePrintf(1, "received error from server\n");
    }
    return code;
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
    int code = sendLinkMessage(YFS_SYMLINK, oldname, newname);
    if (code == ERROR) {
        TracePrintf(1, "received error from server\n");
    }
    return code;
}

int
ReadLink(char *pathname, char *buf, int len)
{
    int code = sendReadLinkMessage(pathname, buf, len);
    if (code == ERROR) {
        TracePrintf(1, "received error from server\n");
    }
    return code;
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
    // TODO
    (void) pathname;
    (void) statbuf;
    return 0;
}

int
Sync()
{
    // TODO
    return 0;
}

int
Shutdown()
{
    // TODO
    return 0;
}
