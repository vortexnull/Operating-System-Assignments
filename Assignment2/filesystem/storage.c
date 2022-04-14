#include<time.h>
#include<errno.h>
#include<stdio.h>
#include<alloca.h>
#include<string.h>
#include<unistd.h>
#include <stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>

#include"inode.h"
#include"blocks.h"
#include"storage.h"
#include"directory.h"

void
storage_init(const char *path)
{
    // printf("storage_init()\n");

    blocks_init(path);
    directory_init();
}

int
storage_stat(const char *path, struct stat *st)
{
    // printf("storage_stat()\n");

    int inum = tree_lookup(path);
    if (inum < 0){
        // printf("    + invalid inum\n");
        return -ENOENT;
    }

    inode *node = get_inode(inum);

    memset(st, 0, sizeof(struct stat));

    st->st_ino = inum;
    st->st_mode = node->mode;
    st->st_uid = node->uid;
    st->st_gid = node->gid;
    st->st_size = node->size;
    st->st_blksize = BLOCK_SIZE;
    st->st_blocks = (node->size - 1 + BLOCK_SIZE) / BLOCK_SIZE;
    (st->st_atim).tv_sec = node->atime;
    (st->st_ctim).tv_sec = node->ctime;
    (st->st_mtim).tv_sec = node->mtime;

    return 0;
}

int
storage_read(const char *path, char *buf, size_t size, off_t offset)
{
    // printf("storage_read()\n");

    int inum = tree_lookup(path);
    if (inum < 0){
        // printf("    + invlaid inum\n");
        return -ENOENT;
    }

    inode *node = get_inode(inum);

    if (offset >= node->size){
        // printf("    + invalid offset (offset >= node->size)\n");
        return 0;
    }

    if(offset + size > node->size){
        // printf("    + invalid offset (offset + size > node->size)\n");
        size = node->size - offset;
    }

    int startblock = offset / BLOCK_SIZE;
    int endblock = (offset + size) / BLOCK_SIZE;

    char* block;
    int filebnum = startblock, bytestoread = 0;

    while(filebnum <= endblock){
        block = (char*)get_inode_block(get_inode(inum), filebnum);

        if(filebnum == startblock){
            if(startblock == endblock)
                bytestoread = size;
            else
                bytestoread = BLOCK_SIZE - offset % BLOCK_SIZE;
            
            block = block + offset;
        }
        else if(filebnum == endblock)
            bytestoread = (offset + size) % BLOCK_SIZE;
        else
            bytestoread = BLOCK_SIZE;

        memcpy(buf, block, bytestoread);
        buf += bytestoread;

        filebnum++;
    }

    return size;
}

int
storage_write(const char *path, const char *buf, size_t size, off_t offset)
{
    // printf("storage_write()\n");

    int res = storage_truncate(path, offset + size);
    if(res < 0){
        // printf("    + invlaid inum\n");
        return res;        
    }
    
    int inum = tree_lookup(path);
    inode *node = get_inode(inum);

    int startblock = offset / BLOCK_SIZE;
    int endblock = (offset + size) / BLOCK_SIZE;

    char* block;
    int filebnum = startblock, bytestowrite = 0;

    while(filebnum <= endblock){
        block = (char*)get_inode_block(get_inode(inum), filebnum);

        if(filebnum == startblock){
            if(startblock == endblock)
                bytestowrite = size;
            else
                bytestowrite = BLOCK_SIZE - offset % BLOCK_SIZE;
            
            block = block + offset;
        }
        else if(filebnum == endblock)
            bytestowrite = (offset + size) % BLOCK_SIZE;
        else
            bytestowrite = BLOCK_SIZE;

        memcpy(block, buf, bytestowrite);
        buf += bytestowrite;

        filebnum++;
    } 
    
    return size;
}

int
storage_truncate(const char *path, off_t size)
{   
    // printf("storage_truncate()\n");
    
    int inum = tree_lookup(path);
    if(inum < 0){
        // printf("    + invalid inum\n");
        return -ENOENT;
    }

    inode* node = get_inode(inum);

    if(size >= node->size)
        grow_inode(node, size);
    else
        shrink_inode(node, size);
    
    return 0;
}

int
storage_mknod(const char *path, int mode)
{
    printf("\nstorage_mknod()\n");

    if(tree_lookup(path) >= 0){
        // printf("    + file already exists at path\n");
        return -EEXIST;
    }

    char *builtpath = malloc(strlen(path) + 1);
    memset(builtpath, 0, strlen(path) + 1);
    builtpath[0] = '/';

    slist* pathlist = s_split(path + 1, '/');

    for(slist* s = pathlist; s != 0; s = s->next){
        printf("storage_mknod() -> builtpath: %s\n", builtpath);
        int parentdd = tree_lookup(builtpath);
        int dd = directory_lookup(get_inode(parentdd), s->data);

        if(s->next == 0 && dd < 0){
            int inum = alloc_inode();
            printf("storage_mknod() -> allocating inum: %d\n", inum);
            inode* node = get_inode(inum);
            node->mode = mode;

            if(node->mode & 040000 == 040000){
                printf("storage_mknod -> directory check\n");
                char *selfname = ".";
                char *parentname = "..";

                directory_put(node, parentname, parentdd);
                directory_put(node, selfname, inum);
            }

            int res = directory_put(get_inode(parentdd), s->data, inum);

            if(res < 0)
                return -ENOSPC;
            
            free(builtpath);
        }
        else if(dd >= 0 && get_inode(dd)->mode == 040755){
            if(strcmp(builtpath, "/") == 0)
                strcat(builtpath, s->data);
            else{
                strcat(builtpath, "/");
                strcat(builtpath, s->data); 
            }
        }
        else{
            int inum = alloc_inode();
            inode* node = get_inode(inum);
            node->mode = 040755;

            char *selfname = ".";
            char *parentname = "..";

            directory_put(node, parentname, parentdd);
            directory_put(node, selfname, inum);

            int res = directory_put(get_inode(parentdd), s->data, dd);

            if(res < 0)
                return -ENOSPC;

            if(strcmp(builtpath, "/") == 0)
                strcat(builtpath, s->data);
            else{
                strcat(builtpath, "/");
                strcat(builtpath, s->data); 
            }            
        }
    }

    return 0; 
}

slist*
storage_list(const char *path)
{   
    // printf("storage_list()\n");

    int inum = tree_lookup(path);

    if(inum < 0){
        // printf("    + invalid path\n");
        return 0;
    }

    return directory_list(inum);
}

int
storage_unlink(const char *path)
{
    // printf("storage_unlink()\n");
    
    char* name;
    inode* node;
    int parentdd, dd = ROOT_INUM;
    slist* pathlist = s_split(path + 1, '/');

    while(pathlist != 0){
        parentdd = dd;
        name = pathlist->data;
        node = get_inode(parentdd);
        dd = directory_lookup(node, pathlist->data);

        if(dd < 0){
            // printf("    + invalid path\n");
            return -ENOENT;
        }

        pathlist = pathlist->next;
    }

    directory_delete(node, name);

    return 0;
}

int
storage_access(const char *path, int mode)
{
    // printf("storage_access()\n");
    
    int inum = tree_lookup(path);

    if(inum < 0){
        // printf("    + invalid path\n");
        return -ENOENT;
    }

    if(mode == 0)
        return 0;

    inode* node = get_inode(inum);
    uid_t uid = getuid();
    gid_t gid = getgid();
    int res;

    if(node->uid == uid)
        res = (node->mode >> 2*3) & mode;
    else if(node->gid == gid)
        res = (node->mode >> 3) & mode;
    else
        res = node->mode & mode;

    if(res == mode)
        return 0;
    
    return -EACCES;
}

int
storage_utimens(const char *path, const struct timespec ts[2])
{
    // printf("storage_utimens()\n");

    int inum = tree_lookup(path);

    if(inum < 0){
        // printf("    + invalid path\n");
        return -ENOENT;
    }

    inode* node = get_inode(inum);

    node->atime = ts[0].tv_sec;
    node->mtime = ts[1].tv_sec;

    return 0;
}