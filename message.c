/*
 * Interaction between yfs and user library is done here
 */
#include <stdlib.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include "message.h"
#include "yfs.h"

static char * getPathFromProcess(int pid, char *pathname, int len);

void
processRequest()
{
    int return_value;

    struct message_generic msg_rcv;
    
    // receive the message as a generic type first
    int pid = Receive(&msg_rcv);
    if (pid == ERROR) {
        TracePrintf(1, "unable to receive message\n");
        Exit(1);
    }

    // determine message type based on requested operation
    if (msg_rcv.num == YFS_OPEN) {
        struct message_path * msg = (struct message_path *) &msg_rcv;
        char *pathname = getPathFromProcess(pid, msg->pathname, msg->len);
        return_value = yfsOpen(pathname, msg->current_inode);
        free(pathname);
    } else if (msg_rcv.num == YFS_CREATE) {
        struct message_path * msg = (struct message_path *) &msg_rcv;
        char *pathname = getPathFromProcess(pid, msg->pathname, msg->len);
        return_value = yfsCreate(pathname, msg->current_inode, CREATE_NEW);
        free(pathname);
    } else if (msg_rcv.num == YFS_READ) {
        struct message_file * msg = (struct message_file *) &msg_rcv;
        return_value = yfsRead(msg->inodenum, msg->buf, msg->size, msg->offset, pid);
    } else if (msg_rcv.num == YFS_WRITE) {
        struct message_file * msg = (struct message_file *) &msg_rcv;
        return_value = yfsWrite(msg->inodenum, msg->buf, msg->size, msg->offset, pid);
    } else if (msg_rcv.num == YFS_SEEK) {

    } else if (msg_rcv.num == YFS_LINK) {

    } else if (msg_rcv.num == YFS_UNLINK) {
        struct message_path * msg = (struct message_path *) &msg_rcv;
        char *pathname = getPathFromProcess(pid, msg->pathname, msg->len);
        return_value = yfsUnlink(pathname, msg->current_inode);
        free(pathname);
    } else if (msg_rcv.num == YFS_SYMLINK) {

    } else if (msg_rcv.num == YFS_READLINK) {

    } else if (msg_rcv.num == YFS_MKDIR) {
        struct message_path * msg = (struct message_path *) &msg_rcv;
        char *pathname = getPathFromProcess(pid, msg->pathname, msg->len);
        return_value = yfsMkDir(pathname, msg->current_inode);
        free(pathname);
    } else if (msg_rcv.num == YFS_RMDIR) {
        struct message_path * msg = (struct message_path *) &msg_rcv;
        char *pathname = getPathFromProcess(pid, msg->pathname, msg->len);
        return_value = yfsRmDir(pathname, msg->current_inode);
        free(pathname);
    } else if (msg_rcv.num == YFS_CHDIR) {
        struct message_path * msg = (struct message_path *) &msg_rcv;
        char *pathname = getPathFromProcess(pid, msg->pathname, msg->len);
        return_value = yfsChDir(pathname, msg->current_inode);
        free(pathname);
    } else if (msg_rcv.num == YFS_STAT) {

    } else if (msg_rcv.num == YFS_SYNC) {

    } else if (msg_rcv.num == YFS_SHUTDOWN) {
        
    } else {
        TracePrintf(1, "unknown operation %d\n", msg_rcv.num);
        return_value = ERROR;
    }
    
    // send reply
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
    if (local_pathname == NULL) {
        TracePrintf(1, "error allocating memory for pathname\n");
        return NULL;
    }
    if (CopyFrom(pid, local_pathname, pathname, len) != 0) {
        TracePrintf(1, "error copying %d bytes from %p in pid %d to %p locally\n", 
                len, pathname, pid, local_pathname);
        return NULL;
    }
    return local_pathname;
}