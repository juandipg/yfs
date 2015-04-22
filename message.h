/*
 * message types and IPC API
 */

#define YFS_OPEN        0
#define YFS_CREATE      1
#define YFS_READ        2
#define YFS_WRITE       3
#define YFS_SEEK        4
#define YFS_LINK        5
#define YFS_UNLINK      6
#define YFS_SYMLINK     7
#define YFS_READLINK    8
#define YFS_MKDIR       9
#define YFS_RMDIR       10
#define YFS_CHDIR       11
#define YFS_STAT        12
#define YFS_SYNC        13
#define YFS_SHUTDOWN    14

/*
 * A generic message that can only hold 
 * a single integer. Used for initial casting
 * to determine desired operation, and to
 * reply with a return value.
 * 
 * Also useful for sync and shutdown.
 */
struct message_generic {
    int num;
    char padding[28];
};

/*
 * A message useful for sending a pathname
 */
struct message_path {
    int num;
    int current_inode;
    char *pathname;
    int len;
    char padding[12];
};

/*
 * A message useful for requesting file access
 */
struct message_file {
    int num;
    int inode;
    void *buf;
    int size;
    int offset;
    char padding[8];
};

/*
 * A message useful for sending two names
 */
struct message_link {
    int num;
    int current_inode;
    char *old_name;
    char *new_name;
    int old_len;
    int new_len;
};

void processRequest();
