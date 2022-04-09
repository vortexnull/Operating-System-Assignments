#include"blocks.h"

#define MAX_FNAME   20
#define ROOT_INUM   0

typedef struct dirent{
    char name[MAX_FNAME + 1];
    int inum;
} dirent;

#define MAX_DIR_ENTRIES (int)(BLOCK_SIZE / sizeof(dirent))

void set_dirent(dirent* dd, const char* name, int inum);
void init_directory_block(dirent* d);
int directory_init();
void directory_put(inode* dd, const char* name, int inum);
int directory_lookup(inode* dd, const char* name);
int tree_lookup(const char* path);
void directory_delete(inode* dd, const char* name);
slist* directory_list(int inum);