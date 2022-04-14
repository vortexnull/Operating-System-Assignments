#include"blocks.h"

#define ROOT_INUM   0

int directory_init();
int directory_put(inode* dd, const char* name, int inum);
int directory_lookup(inode* dd, const char* name);
int tree_lookup(const char* path);
int directory_delete(inode* dd, const char* name);
slist* directory_list(int inum);