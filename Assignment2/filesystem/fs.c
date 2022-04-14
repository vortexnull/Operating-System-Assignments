#include<fuse.h>
#include<time.h>
#include<errno.h>
#include<stdio.h>
#include<sys/stat.h>

#include"slist.h"
#include"inode.h"
#include"storage.h"
#include"directory.h"

#define FUSE_USE_VERSION 26

int
fs_getattr(const char *path, struct stat *st) //dtype1
{
    printf("getattr @ %s", path);
    return storage_stat(path, st);
}

int
fs_access(const char *path, int mask) //dtype2
{
    printf("access @ %s", path);
    return storage_access(path, mask);
}

int
fs_mkdir(const char* path, mode_t mode) //dtype3
{
    printf("mkdir @ %s", path);
    return storage_mknod(path, 040000 | mode);
}

int
fs_rmdir(const char* path)
{
    printf("rmdir @ %s", path);
    return storage_unlink(path);    
}

int
fs_unlink(const char *path)
{
    printf("unlink @ %s", path);
    return storage_unlink(path);
}

int
fs_truncate(const char *path, off_t size) //dtype4
{
    printf("truncate @ %s", path);
    return storage_truncate(path, size);
}

int
fs_utimens(const char *path, const struct timespec ts[2]) //dtype5
{
    printf("utimens @ %s", path);
    return storage_utimens(path, timespec);
}

int
fs_open(const char *path, struct fuse_file_info *fi) //dtype6
{
    printf("open @ %s", path);
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
fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) //dtype7
{
    printf("read @ %s", path);
    return storage_read(path, buf, size, offset);
}

int
fs_write(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) //dtype7
{
    printf("write @ %s", path);
    return storage_write(path, buf, size, offset);
}

int
fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)) //dtype8
{
    printf("readdir @ %s", path);
    int res;
    struct stat st;
    char itempath[200];

    slist* items = storage_list(path);

    if(s == 0)
        return -1;

    for(slist* s = items; s != 0; s = s->next){

        if(strcmp(path, "/")){
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

//int
//fs_truncate(const char *path, off_t size) //dtype4
//{
//    return storage_truncate(part, size);
//}

struct fuse_operations fs_ops;

void
fs_init_ops(struct fuse_operations* op)
{
    op = {0};

    op->getattr     = fs_getattr;
    op->access      = fs_access;
    op->mkdir       = fs_mkdir;
    op->rmdir       = fs_rmdir;
    op->unlink      = fs_unlink;
    op->truncate    = fs_truncate;
    op->utimens     = fs_utimens;
    op->open        = fs_open;
    op->read        = fs_read;
    op->write       = fs_write;
    op->readdir     = fs_readdir;
    op->truncate    = fs_truncate;
}   

int
main(int argc, char* argv[])
{
    assert(argc > 2 && argc < 6);
    storage_init(argv[--argc]);
    fs_init_ops(&fs_ops);
    return fuse_main(argc, argv, &fs_ops, NULL);
}

