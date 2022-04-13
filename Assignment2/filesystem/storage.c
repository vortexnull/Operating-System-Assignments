#include<time.h>
#include<errno.h>
#include<alloca.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>

#include"slist.h"
#include"inode.h"
#include"blocks.h"
#include"storage.h"
#include"directory.h"

void
storage_init(const char *path)
{
    printf("storage_init()");

    blocks_init(path);
    directory_init();
}

int
storage_stat(const char *path, struct stat *st)
{
    printf("storage_stat()");

    int inum = tree_lookup(path);
    if (inum < 0){
        printf("    + invalid inum");
        return -ENOENT;
    }

    inode *node = get_inode(inum);

    st = {0};

    st_ino = inum;
    st->st_mode = node->mode;
    st->st_uid = node->uid;
    st->st_gid = node->gid;
    st->st_size = node->size;
    st->st_blksize = BLOCK_SIZE;
    st->st_blocks = (node->size - 1 + BLOCK_SIZE) / BLOCK_SIZE;
    st->st_atim = node->atime;
    st->st_ctim = node->ctime;
    st->st_mtim = node->mtime;

    return 0;
}

int
storage_read(const char *path, char *buf, size_t size, off_t offset)
{
    printf("storage_read()");

    int inum = tree_lookup(path);
    if (inum < 0){
        printf("    + invlaid inum");
        return -ENOENT;
    }

    inode *node = get_inode(inum);

    if (offset >= node->size){
        printf("    + invalid offset (offset >= node->size)");
        return 0;
    }

    if(offset + size > node->size){
        printf("    + invalid offset (offset + size > node->size)");
        size = node->size - offset;
    }

    int startblock = offset / BLOCK_SIZE;
    int endblock = (offset + size) / BLOCK_SIZE;

    char* block;
    int filebnum = startblock, bytestoread = 0;

    while(filebnum <= endblock){
        block = (char*)get_inode_block(inum, filebnum);

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
    printf("storage_write()");

    int res = storage_truncate(path, offset + size);
    if(res < 0){
        printf("    + invlaid inum");
        return res;        
    }
    
    int inum = tree_lookup(path);
    inode *node = get_inode(inum);

    int startblock = offset / BLOCK_SIZE;
    int endblock = (offset + size) / BLOCK_SIZE;

    char* block;
    int filebnum = startblock, bytestowrite = 0;

    while(filebnum <= endblock){
        block = (char*)get_inode_block(inum, filebnum);

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
    printf("storage_truncate()");
    
    int inum = tree_lookup(path);
    if(inum < 0){
        printf("    + invalid inum");
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
    printf("storage_mknod()");

    if(tree_lookup(path) >= 0){
        printf("    + file already exists at path");
        return -EEXIST;
    }

    slist* pathlist = s_split(path + 1, '/');
    int parentdd = ROOT_INUM, dd, flag = 0;

    while(pathlist != 0){
        if(flag)
            dd = -1;
        else
            dd = directory_lookup(get_inode(parentdd), pathlist->data);

        if(dd < 0){
            if(pathlist->next == 0){
                if(mode == 040755)
                    dd = directory_init();
                else
                    dd = alloc_inum();
                
                directory_put(get_inode(parentdd), pathlist->data, dd);
            }
            else{
                dd = directory_init();
                directory_put(get_inode(parentdd), pathlist->data, dd);
            }

            flag = 1;
        }

        parentdd = dd;
        pathlist = pathlist->next;
    }

    return 0; 
}

slist*
storage_list(const char *path)
{   
    printf("storage_list()");

    int inum = tree_lookup(path);

    if(inum < 0){
        printf("    + invalid path");
        return 0;
    }

    return directory_list(inum);
}

int
storage_unlink(const char *path)
{
    printf("storage_unlink()");
    
    char* name;
    int parentdd, dd = ROOT_INUM;
    slist* pathlist = s_split(path + 1, '/');

    while(pathlist != 0){
        parentdd = dd;
        name = pathlist->data;
        inode* node = get_inode(parentdd);
        dd = directory_lookup(node, pathlist->data);

        if(dd < 0){
            printf("    + invalid path");
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
    printf("storage_access()");
    
    int inum = tree_lookup(path);

    if(inum < 0){
        printf("    + invalid path");
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
    printf("storage_utimens()");

    int inum = tree_lookup(path);

    if(inum < 0){
        printf("    + invalid path");
        return -ENOENT;
    }

    inode* node = get_inode(inum);

    node->atime = ts[0].tv_sec;
    node->mtime = ts[1].tv_sec;

    return 0;
}