/*
 * Interaction between yfs and user library is done here
 */
#include <stdlib.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include "message.h"
#include "yfs.h"

// TODO: move this to yfs.h
#define CREATE_NEW -1

static char * getPathFromProcess(int pid, char *pathname, int len);

void
processRequest()
{
    int return_value;

    struct message_generic msg_rcv;
    int pid = Receive(&msg_rcv);

    if (msg_rcv.num == YFS_OPEN) {

    } else if (msg_rcv.num == YFS_CREATE) {
        struct message_path * msg = (struct message_path *) &msg_rcv;
        char *pathname = getPathFromProcess(pid, msg->pathname, msg->len);
        if (pathname == NULL) {
            return;
        }
        return_value = yfsCreate(pathname, msg->current_inode, CREATE_NEW);
    }
    
    struct message_generic msg_rply;
    msg_rply.num = return_value;
    if (Reply(&msg_rply, pid) != 0) {
        TracePrintf(1, "error sending reply to pid %d\n", pid);
    }
}

static char *
getPathFromProcess(int pid, char *pathname, int len)
{
    char *local_pathname = malloc(len * sizeof (char));
    if (pathname == NULL) {
        TracePrintf(1, "error allocating memory for pathname\n");
        return NULL;
    }
    if (CopyFrom(pid, local_pathname, pathname, len) != 0) {
        TracePrintf(1, "error copying from pid %d\n", pid);
        return NULL;
    }
    return local_pathname;
}