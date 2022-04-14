#include<sys/types.h>

#define BLOCK_SIZE  4096                        // block size in bytes
#define BLOCK_COUNT 256                         // number of blocks
#define FS_SIZE     BLOCK_SIZE * BLOCK_COUNT    // = 1 MB

void blocks_init(const char* path);
void blocks_free();
void* get_block(int bnum);
uint8_t* get_block_bitmap();
void free_block(int bnum);
int alloc_block();