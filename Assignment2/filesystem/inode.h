#include<stdint.h>
#include<sys/types.h>

#include"blocks.h"

typedef struct inode{
    char valid;         // valid bit
    int entries;        // number of entries (valid for directories)
    int size;           // size
    int mode;           // permission & type
    int dptr[2];        // direct pointer
    int iptr;           // indirect pointer
    uid_t uid;          // user id
    gid_t gid;          // group id
    time_t ctime;       // time created
    time_t atime;       // time last accessed
    time_t mtime;       // time last modified
} inode;

#define INODE_COUNT (int)(BLOCK_SIZE / sizeof(inode))

inode* get_inode(int inum);
int alloc_inode();
void free_inode();
void rem_inode_block(inode* node);
int add_inode_block(inode* node);
void shrink_inode(inode* node, int size);
void grow_inode(inode* node, int size);
int get_inode_bnum(inode* node, int fileptr);
void* get_inode_block(inode* node, int blockindex);