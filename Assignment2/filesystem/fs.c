#include<time.h>
#include<errno.h>
#include<stdio.h>
#include<assert.h>
#include<string.h>
#include<sys/stat.h>

#include"inode.h"
#include"storage.h"
#include"directory.h"

#define FUSE_USE_VERSION 26

#include<fuse.h>

int
fs_getattr(const char *path, struct stat *st)
{
    printf("getattr @ %s\n", path);
    return storage_stat(path, st);
}

int
fs_access(const char *path, int mask)
{
    printf("access @ %s\n", path);
    return storage_access(path, mask);
}

int
fs_mkdir(const char* path, mode_t mode)
{
    printf("mkdir @ %s\n", path);
    return storage_mknod(path, 040000 | mode);
}

int
fs_rmdir(const char* path)
{
    printf("rmdir @ %s\n", path);
    return storage_unlink(path);    
}

int
fs_unlink(const char *path)
{
    printf("unlink @ %s\n", path);
    return storage_unlink(path);
}

int
fs_truncate(const char *path, off_t size)
{
    printf("unlink @ %s\n", path);
    return storage_unlink(path);
}

int
fs_utimens(const char *path, const struct timespec ts[2])
{
    printf("utimens @ %s\n", path);
    return storage_utimens(path, ts);
}

int
fs_open(const char *path, struct fuse_file_info *fi)
{
    printf("open @ %s\n", path);
    int res;
    struct timespec ts;

    if(clock_gettime(CLOCK_REALTIME, &ts) < 0)
        return -1;
    
    int inum = tree_lookup(path);

    if(inum < 0)
        return -ENOENT;
    
    inode* node = get_inode(inum);
    node->atime = ts.tv_sec;

    return 0;
}

int
fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("read @ %s\n", path);
    return storage_read(path, buf, size, offset);
}

int
fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("write @ %s\n", path);
    return storage_write(path, buf, size, offset);
}

int
fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    printf("readdir @ %s\n", path);
    int res;
    struct stat st;
    char itempath[200];

    slist* items = storage_list(path);

    if(items == 0)
        return -1;

    for(slist* s = items; s != 0; s = s->next){

        if(strcmp(path, "/") == 0){
            itempath[0] = '/';

            int len = strlen(s->data);
            strncpy(itempath + 1, s->data, 200);

            if(len > 200)
                len = 200;
            
            itempath[len + 1] = 0;

            res = storage_stat(itempath, &st);

            if(res < 0)
                return res;

            filler(buf, s->data, &st, 0);
        }
        else{
            int pathlen = strlen(path);
            strncpy(itempath, path, pathlen);
            itempath[pathlen] = '/';


            int len = strlen(s->data);
            strncpy(itempath + pathlen + 1, s->data, 200);

            if(len > 200)
                len = 200;
            
            itempath[pathlen + len + 1] = 0;
    
            res = storage_stat(itempath, &st);

            if(res < 0)
                return res;

            filler(buf, s->data, &st, 0);
        }
    }

    return 0;
}

struct fuse_operations fs_ops;

void
fs_init_ops(struct fuse_operations* ops)
{
    memset(ops, 0, sizeof(struct fuse_operations));

    ops->getattr     = fs_getattr;
    ops->access      = fs_access;
    ops->mkdir       = fs_mkdir;
    ops->rmdir       = fs_rmdir;
    ops->unlink      = fs_unlink;
    ops->truncate    = fs_truncate;
    ops->utimens     = fs_utimens;
    ops->open        = fs_open;
    ops->read        = fs_read;
    ops->write       = fs_write;
    ops->readdir     = fs_readdir;
    ops->truncate    = fs_truncate;
}   

int
main(int argc, char* argv[])
{
    assert(argc > 2 && argc < 6);
    storage_init(argv[--argc]);
    fs_init_ops(&fs_ops);
    return fuse_main(argc, argv, &fs_ops, NULL);
}

