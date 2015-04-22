#define __USE_GNU
#include <string.h>
#include <comp421/iolib.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include "message.h"

int current_inode = ROOTINODE;

int getLenForPath(char *pathname) {
    int i;
    for (i = 0; i < MAXPATHNAMELEN; i++) {
        if (pathname[i] == '\0') {
            break;
        }
    }
    if (i == 0 || i == MAXPATHNAMELEN) {
        return ERROR;
    }
    return i + 1;
}

int Open(char *pathname) {
    (void) pathname;
    return 0;
}
int Close(int fd) {
    (void) fd;
    return 0;
}
int Create(char *pathname) {
    int len = getLenForPath(pathname);
    if (len == ERROR) {
        return ERROR;
    }
    struct message_path msg;
    msg.num = YFS_CREATE;
    msg.current_inode = current_inode;
    msg.len = len;
    if (Send(&msg, -FILE_SERVER) != 0) {
        TracePrintf(1, "error sending message to server\n");
        return ERROR;
    }
    // msg gets overwritten with reply after calling Send
    return msg.num;
}
int Read(int fd, void *buf, int size) {
    (void) fd;
    (void) buf;
    (void) size;
    return 0;
}
int Write(int fd, void *buf, int size) {
    (void) fd;
    (void) buf;
    (void) size;
    return 0;
}
int Seek(int fd, int offset, int whence) {
    (void) fd;
    (void) offset;
    (void) whence;
    return 0;
}
int Link(char *oldname, char *newname) {
    (void) oldname;
    (void) newname;
    return 0;
}
int Unlink(char *pathname) {
    (void) pathname;
    return 0;
}
int SymLink(char *oldname, char *newname) {
    (void) oldname;
    (void) newname;
    return 0;
}
int ReadLink(char *pathname, char *buf, int len) {
    (void) pathname;
    (void) buf;
    (void) len;
    return 0;
}
int MkDir(char *pathname) {
    (void) pathname;
    return 0;
}
int RmDir(char *pathname) {
    (void) pathname;
    return 0;
}
int ChDir(char *pathname) {
    (void) pathname;
    return 0;
}
int Stat(char *pathname, struct Stat *statbuf) {
    (void) pathname;
    (void) statbuf;
    return 0;
}
int Sync() {
    return 0;
}
int Shutdown() {
    return 0;
}


