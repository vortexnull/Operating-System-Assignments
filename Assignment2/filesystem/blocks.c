#include<fcntl.h>
#include<errno.h>
#include<unistd.h>
#include<stdint.h>
#include<assert.h>
#include<sys/mman.h>

#include"bitmap.h"

static void* base = 0;
static int alloc_bnum = 29;

void
blocks_init(const char* path)
{
    int fd = open(path, O_CREAT | O_RDWR, 0644);
    assert(fd != -1);

    int rv = ftruncate(pages_fd, NUFS_SIZE);
    assert(rv != -1);

    base = mmap(NULL, FS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(base != MAP_FAILED);

    uint8_t* block_bitmap = get_block_bitmap();
    bitmap_put(block_bitmap, 0, 1);     // 1st block stores block bitmap
    bitmap_put(block_bitmap, 1, 1);     // 2nd block stores inodes
}

void
blocks_free()
{
    int rv = munmap(base, FS_SIZE);
    assert(rv != -1);
}

void*
get_block(int bnum)
{
    return (char*)base + BLOCK_SIZE * bnum;
}

uint8_t*
get_block_bitmap()
{
    return (uint8_t*) get_block(0);
}

void
free_block(int bnum)
{
    uint8_t* block_bitmap = get_block_bitmap();
    bitmap_put(block_bitmap, bnum, 0);
}

int
alloc_block()
{
    uint8_t* block_bitmap = get_block_bitmap();

    for(int i = 0; i < BLOCK_COUNT; i++){
        if(bitmap_get(alloc_bnum) == 0){
            bitmap_put(block_bitmap, alloc_bnum, 1);
            memset(get_block(alloc_bnum), 0, BLOCK_SIZE);
            return alloc_bnum;
        }
        
        alloc_bnum = (alloc_bnum * 13 + 5) % BLOCK_COUNT;
    }

    return -ENOSPC;
}