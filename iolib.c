#include <comp421/iolib.h>

int Open(char *pathname) {
    (void) pathname;
    return 0;
}
int Close(int fd) {
    (void) fd;
    return 0;
}
int Create(char *pathname) {
    (void) pathname;
    return 0;
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


