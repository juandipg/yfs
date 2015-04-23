#include <comp421/iolib.h>
#include <comp421/hardware.h>

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
    char buf[13];
    TracePrintf(1, "reading from file, got %d\n", Read(fd, buf, 13));
    TracePrintf(1, "got %s\n", buf);
    return (0);
}
