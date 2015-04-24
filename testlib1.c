#include <comp421/iolib.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
//#include "message.h"

int
main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    
//    int fd1 = Create("/a/i.txt");
//    TracePrintf(1, "returned from Create with fd: %d\n", fd1);
//    int fd2 = Open("/a/i.txt");
//    TracePrintf(1, "opened file with fd: %d\n", fd2);    
//    int val = Close(fd1);
//    TracePrintf(1, "close file with fd: %d, return %d\n", fd1, val);    
//    val = Close(fd2);
//    TracePrintf(1, "close file with fd: %d, return %d\n", fd2, val);    
    TracePrintf(1, "called mkdir, return %d\n", MkDir("/a/j"));
    TracePrintf(1, "called chdir, return %d\n", ChDir("/a/j"));
    int fd = Create("x.txt");
//    TracePrintf(1, "creating file x.txt, got fd %d\n", fd);
//    TracePrintf(1, "opened file /a/j/x.txt, got fd %d\n", Open("/a/j/x.txt"));
//    TracePrintf(1, "opened file x.txt, got fd %d\n", Open("x.txt"));
//    TracePrintf(1, "unlinking /a/j/x.txt, got %d\n", Unlink("/a/j/x.txt"));
//    TracePrintf(1, "removing directory /a/j, got %d\n", RmDir("/a/j"));
    TracePrintf(1, "writing to file, got %d\n", Write(fd, "hello world\n", 13));
//    int fd0 = Open("x.txt");
//    TracePrintf(1, "opening file, got fd %d\n", fd0);
    TracePrintf(1, "calling seek, got %d\n", Seek(fd, -14, SEEK_END));
    char buf[13];
    TracePrintf(1, "reading from file, got %d\n", Read(fd, buf, 13));
    TracePrintf(1, "got %s\n", buf);
//    TracePrintf(1, "calling symlink, got %d\n", SymLink("x.txt", "/a/myfile.txt"));
//    int fd3 = Open("/a/myfile.txt");
//    TracePrintf(1, "opening symlinked file, got fd %d\n", fd3);
//    char buf3[13];
//    TracePrintf(1, "reading from file, got %d\n", Read(fd3, buf3, 13));
//    TracePrintf(1, "got %s\n", buf3);
//    char buff[6];
//    TracePrintf(1, "reading symlink, got %d\n", ReadLink("/a/myfile.txt", buff, 6));
//    TracePrintf(1, "symlink points to %s\n", buff);
//    TracePrintf(1, "calling link, got %d\n", Link("x.txt", "d.txt"));
//    int fd1 = Open("d.txt");
//    TracePrintf(1, "opening linked file, got fd %d\n", fd1);
//    char buf2[13];
//    TracePrintf(1, "reading from file, got %d\n", Read(fd1, buf2, 13));
//    TracePrintf(1, "got %s\n", buf2);
//    TracePrintf(1, "size of message_stat %d\n", sizeof(struct message_stat));
//    struct Stat stat;
//    TracePrintf(1, "calling stat, got %d\n", Stat("x.txt", &stat));
//    TracePrintf(1, "inum %d, nlnk %d, size %d, type %d\n",
//            stat.inum, stat.nlink, stat.size, stat.type);
    return (0);
}
