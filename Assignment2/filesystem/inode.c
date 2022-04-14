#include<time.h>
#include<errno.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<assert.h>
#include<stdlib.h>

#include"inode.h"
#include"blocks.h"

inode* 
get_inode(int inum)
{   
    assert(inum < INODE_COUNT);
    inode* nodes = get_block(1);
    return &nodes[inum];
}

int
alloc_inode()
{
    for(int i = 0; i < INODE_COUNT; i++){
        inode* node = get_inode(i);

        if(node->valid == 0){                   // if valid = 0 => inode is free
            memset(node, 0, sizeof(inode));
            node->valid = 1;
            node->entries = -1;                 // -1 for regualar file, non-negative for directories
            node->size = 0;
            node->mode = 010644;
            node->dptr[0] = alloc_block();
            node->dptr[1] = 0;
            node->iptr = 0;
            node->uid = getuid();
            node->gid = getgid();
            time_t ctime = time(NULL);
            time_t atime = time(NULL);
            time_t mtime = time(NULL);
            
            return i;
        }
    }

    return -ENOSPC;
}

void
free_inode(int inum)
{
    inode* node = get_inode(inum);
    
    node->valid = 0;
    shrink_inode(node, 0);
}

void
rem_inode_block(inode* node)
{   
    int node_numblocks = (node->size - 1 + BLOCK_SIZE) / BLOCK_SIZE;
    int lastblock = node_numblocks - 1;

    assert(node_numblocks > 0);

    if(node->iptr){
        int* iptr_block = get_block(node->iptr);
        free_block(iptr_block[lastblock - 2]);
        iptr_block[lastblock - 2] = 0;

        if(iptr_block[0] == 0){
            free_block(node->iptr);
            node->iptr = 0;
        }
    }
    else if(node->dptr[1]){
        free_block(node->dptr[1]);
        node->dptr[1] = 0;
    }
    else{
        free_block(node->dptr[0]);
        node->dptr[0] = 0;
    }        
}

int
add_inode_block(inode* node)
{   
    int node_numblocks = (node->size - 1 + BLOCK_SIZE) / BLOCK_SIZE;
    int bnum;

    if(!node->dptr[0]){
        bnum = alloc_block();

        if(bnum == -ENOSPC){
            printf("no space left\n");
            abort();
        }

        node->dptr[0] = bnum;
    }
    else if(!node->dptr[1]){
        bnum = alloc_block();

        if(bnum == -ENOSPC){
            printf("no space left\n");
            abort();
        }

        node->dptr[1] = bnum;
    }
    else{
        if(!node->iptr){
            node->iptr = alloc_block();

            if(node->iptr == -ENOSPC){
                printf("no space left\n");
                abort();
            }
        }
        
        int* iptr_block = get_block(node->iptr);
        bnum = alloc_block();

        if(bnum == -ENOSPC){
            printf("no space left\n");
            abort();
        }

        iptr_block[node_numblocks - 2] = bnum;
    }

    return bnum;
}

void
shrink_inode(inode* node, int size)
{
    int bnum;
    int numblocks, newnumblocks, fileptr;

    numblocks = (node->size - 1 + BLOCK_SIZE) / BLOCK_SIZE;
    newnumblocks = (size - 1 + BLOCK_SIZE) / BLOCK_SIZE;  

    assert(newnumblocks <= numblocks);

    for(; numblocks > newnumblocks; numblocks--)
        rem_inode_block(node);

    node->size = size;
}

void
grow_inode(inode* node, int size)
{
    int numblocks, newnumblocks;
    
    numblocks = (node->size - 1 + BLOCK_SIZE) / BLOCK_SIZE;
    newnumblocks = (size - 1 + BLOCK_SIZE) / BLOCK_SIZE;  

    assert(newnumblocks >= numblocks);

    for(; numblocks < newnumblocks; numblocks++)
        add_inode_block(node);

    node->size = size;    
}

int
get_inode_bnum(inode* node, int fileptr)
{
    int filebnum = fileptr / BLOCK_SIZE;

    if(filebnum == 0)
        return node->dptr[0];
    else if(filebnum == 1)
        return node->dptr[1];
    else{
        int* iptr_block = get_block(node->iptr);
        return iptr_block[filebnum - 2];
    }
}

void*
get_inode_block(inode* node, int blockindex)
{
    int lastblock = (node->size - 1) / BLOCK_SIZE;

    assert(blockindex <= lastblock && blockindex >= 0);
    
    if(blockindex == 0)
        return get_block(node->dptr[0]);
    else if(blockindex == 1)
        return get_block(node->dptr[1]);
    else{
        int* iptr_block = get_block(node->iptr);
        return get_block(iptr_block[blockindex - 2]);
    }
}


