#include <comp421/iolib.h>
#include <comp421/hardware.h>

int
main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    
    int fd = Create("/a/i.txt");
    TracePrintf(1, "Successfully returned from Create with val: %d\n", fd);
    
    return (0);
}
