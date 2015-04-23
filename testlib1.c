#include <comp421/iolib.h>
#include <comp421/hardware.h>

int
main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    
    int fd1 = Create("/a/i.txt");
    TracePrintf(1, "returned from Create with fd: %d\n", fd1);
    int fd2 = Open("/a/i.txt");
    TracePrintf(1, "opened file with fd: %d\n", fd2);    
    int val = Close(fd1);
    TracePrintf(1, "close file with fd: %d, return %d\n", fd1, val);    
    val = Close(fd2);
    TracePrintf(1, "close file with fd: %d, return %d\n", fd2, val);    
    val = MkDir("/a/j");
    TracePrintf(1, "called mkdir, return %d\n", val);
    TracePrintf(1, "creating file /a/j/x.txt, got fd %d\n", Create("/a/j/x.txt"));
    TracePrintf(1, "opened file /a/j/x.txt, got fd %d\n", Open("/a/j/x.txt"));
    return (0);
}
