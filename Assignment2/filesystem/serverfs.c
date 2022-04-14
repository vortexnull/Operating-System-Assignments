#include<stdio.h>
#include<rpc/rpc.h>
#include "dfs_rpc.h"
#include "fs.h"

int*
rpc_getattr_1_svc(struct dtype1 *d, struct svc_req *req)
{
    static int* res;
    res = &(fs_getattr(d->path, d->st));
    return res;
}

int*
rpc_access_1_svc(struct dtype2 *d, struct svc_req *req)
{
    static int* res;
    res = &(fs_access(d->path, d->mask));
    return res;
}

int*
rpc_mkdir_1_svc(struct dtype3 *d, struct svc_req *req)
{
    static int* res;
    res = &(fs_mkdir(d->path, d->mode);
    return res;
}

int*
rpc_open_1_svc(struct dtype6 *d, struct svc_req *req)
{
    static int* res;
    res = &(fs_open(d->path, d->fi);
    return res;
}

int*
rpc_read_1_svc(struct dtype7 *d, struct svc_req *req)
{
    static int* res;
    res = &(fs_read(d->path, d->buf, d->size, d->offset));
    return res;
}

int*
rpc_readdir_1_svc(struct dtype8 *d, struct svc_req *req)
{
    static int* res;
    res = &(fs_readdir(d->path, d->buf, d->filler, d->offset, d->fi));
    return res;
}

int*
rpc_truncate_1_svc(struct dtype4 *d, struct svc_req *req)
{
    static int* res;
    res = &(fs_truncate(d->path, d->size));
    return res;
}

int*
rpc_unlink_1_svc(const char *d, struct svc_req *req){
    static int* res;
    res = &(fs_unlink(d));
    return res;
}

int*
rpc_utimens_1_svc(struct dtype5 *d, struct svc_req *req)
{
    static int* res;
    res = &(fs_utimens(d->path, d->timespec));
    return res;
}

int*
rpc_write_1_svc(struct dtype7 *d, struct svc_req *req){
    static int* res;
    res = &(fs_write(d->path, d->buf, d->size, d->offset));
    return res;
}