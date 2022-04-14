#include<fuse.h>
#include<time.h>
#include<errno.h>
#include<stdio.h>
#include<sys/stat.h>
#include<rpc/rpc.h>
#include "dfs_rpc.h"


#define FUSE_USE_VERSION 26

char *server;
CLIENT clnt;

int
clnt_getattr(const char *path, struct stat *st) //dtype1
{
    printf("getattr @ %s", path);
    struct dtype1 d = {.path = path, .st = st};
    clnt = clnt_create(server, GETATTR_PROGRAM, GETATTR_VERSION, "tcp");
		if(clnt == NULL){
			clnt_pcreateerror(server);
			exit(1);
		}
    return (int)rpc_getattr_1(&d, clnt);
}

int
clnt_access(const char *path, int mask) //dtype2
{
    printf("access @ %s", path);
    struct dtype2 d = {path, mask};
    clnt = clnt_create(server, ACCESS_PROGRAM, ACCESS_VERSION, "tcp");
		if(clnt == NULL){
			clnt_pcreateerror(server);
			exit(1);
		}
    return (int)rpc_access_1(&d, clnt);
}

int
clnt_mkdir(const char* path, mode_t mode) //dtype3
{
    printf("mkdir @ %s", path);
    struct dtype3 d = {path, mode};
    clnt = clnt_create(server, MKDIR_PROGRAM, MKDIR_VERSION, "tcp");
		if(clnt == NULL){
			clnt_pcreateerror(server);
			exit(1);
		}
    return (int)rpc_mkdir_1(&d, clnt);
}

int
clnt_rmdir(const char* path)
{
    printf("rmdir @ %s", path);
    clnt = clnt_create(server, RMDIR_PROGRAM, RMDIR_VERSION, "tcp");
		if(clnt == NULL){
			clnt_pcreateerror(server);
			exit(1);
		}
    return (int)rpc_unlink_1(path);    
}

int
clnt_unlink(const char *path)
{
    printf("unlink @ %s", path);
    clnt = clnt_create(server, RMDIR_PROGRAM, RMDIR_VERSION, "tcp");
		if(clnt == NULL){
			clnt_pcreateerror(server);
			exit(1);
		}
    return (int)rpc_unlink_1(path);
}

int
clnt_truncate(const char *path, off_t size) //dtype4
{
    printf("truncate @ %s", path);
    struct dtype4 d = {path, size};
    clnt = clnt_create(server, UNLINK_PROGRAM, UNLINK_VERSION, "tcp");
		if(clnt == NULL){
			clnt_pcreateerror(server);
			exit(1);
		}
    return (int)rpc_truncate_1(&d, clnt);
}

int
clnt_utimens(const char *path, const struct timespec ts[2]) //dtype5
{
    printf("utimens @ %s", path);
    struct dtype5 d = {path, timespec};
    clnt = clnt_create(server, UTIMENS_PROGRAM, UTIMENS_VERSION, "tcp");
		if(clnt == NULL){
			clnt_pcreateerror(server);
			exit(1);
		}
    return (int)rpc_utimens_1(&d, clnt);
}

int
clnt_open(const char *path, struct fuse_file_info *fi) //dtype6
{
    printf("open @ %s", path);
    struct dtype6 d = {path, fi};
    clnt = clnt_create(server, OPEN_PROGRAM, OPEN_VERSION, "tcp");
		if(clnt == NULL){
			clnt_pcreateerror(server);
			exit(1);
		}
    return (int)rpc_open_1(&d, clnt);
}

int
clnt_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) //dtype7
{
    printf("read @ %s", path);
    struct dtype7 d = {path, buf, size, offset};
    clnt = clnt_create(server, READ_PROGRAM, READ_VERSION, "tcp");
		if(clnt == NULL){
			clnt_pcreateerror(server);
			exit(1);
		}
    return (int)rpc_read_1(&d, clnt);
}

int
clnt_write(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) //dtype7
{
    printf("write @ %s", path);
    struct dtype7 d = {path, buf, size, offset};
    clnt = clnt_create(server, WRITE_PROGRAM, WRITE_VERSION, "tcp");
		if(clnt == NULL){
			clnt_pcreateerror(server);
			exit(1);
		}
    return (int)rpc_write_1(&d, clnt);
}

int
clnt_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)) //dtype8
{
    printf("readdir @ %s", path);
    struct dtype8 d = {path, buf, filler, offset};
    clnt = clnt_create(server, READDIR_PROGRAM, READDIR_VERSION, "tcp");
		if(clnt == NULL){
			clnt_pcreateerror(server);
			exit(1);
		}
    return (int)rpc_readdir_1(&d, clnt);
}

//int
//clnt_truncate(const char *path, off_t size) //dtype4
//{
//    return rpc_truncate(part, size);
//}

struct fuse_operations clnt_ops;

void
clnt_init_ops(struct fuse_operations* op)
{
    op = {0};

    op->getattr     = clnt_getattr;
    op->access      = clnt_access;
    op->mkdir       = clnt_mkdir;
    op->rmdir       = clnt_rmdir;
    op->unlink      = clnt_unlink;
    op->truncate    = clnt_truncate;
    op->utimens     = clnt_utimens;
    op->open        = clnt_open;
    op->read        = clnt_read;
    op->write       = clnt_write;
    op->readdir     = clnt_readdir;
    op->truncate    = clnt_truncate;
}   

int
main(int argc, char* argv[])
{
    assert(argc > 2 && argc < 6);
    //storage_init(argv[--argc]);
    server = "localhost";
    clnt_init_ops(&clnt_ops);
    return fuse_main(argc, argv, &clnt_ops, NULL);
}