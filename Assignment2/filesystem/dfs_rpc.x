#include<fuse.h>
#include<time.h>
#include<errno.h>
#include<stdio.h>
#include<sys/stat.h>

struct dtype1{
    const char *path;
    struct stat *st;
};

struct dtype2{
    const char *path;
    int mask;
};

struct dtype3{
    const char* path;
    mode_t mode;
};

struct dtype4{
    const char *path;
    off_t size;
};

struct dtype5{
    const char *path;
    const struct timespec ts[2];
};

struct dtype6{
    const char *path;
    struct fuse_file_info *fi;
};

struct dtype7{
    const char *path;
    char *buf;
    size_t size;
    off_t offset;
    struct fuse_file_info *fi;
};

struct dtype8{
    const char *path;
    void *buf;
    fuse_fill_dir_t filler;
    off_t offset;
    struct fuse_file_info *fi;
};




program GETATTR_PROGRAM{
    version GETATTR_VERSION{
        int rpc_getattr(struct dtype1) = 1;
    } = 1;
} = 0x20000001;

program ACCESS_PROGRAM{
    version ACCESS_VERSION{
        int rpc_access(struct dtype2) = 1;
    } = 1;
} = 0x20000002;

program MKDIR_PROGRAM{
    version MKDIR_VERSION{
        int rpc_mkdir(struct dtype3) = 1;
    } = 1;
} = 0x20000003;

//program RMDIR_PROGRAM{
//    version RMDIR_VERSION{
//        int rpc_rmdir(const char*) = 1;
//    } = 1;
//} = 0x20000004;

program UNLINK_PROGRAM{
    version UNLINK_VERSION{
        int rpc_unlink(const char*) = 1;
    } = 1;
} = 0x20000005;

program TRUNCATE_PROGRAM{
    version TRUNCATE_VERSION{
        int rpc_truncate(struct dtype4) = 1;
    } = 1;
} = 0x20000006;

program UTIMENS_PROGRAM{
    version UTIMENS_VERSION{
        int rpc_utimens(struct dtype5) = 1;
    } = 1;
} = 0x20000007;

program OPEN_PROGRAM{
    version OPEN_VERSION{
        int rpc_open(struct dtype6) = 1;
    } = 1;
} = 0x20000008;

program READ_PROGRAM{
    version READ_VERSION{
        int rpc_read(struct dtype7) = 1;
    } = 1;
} = 0x20000009;

program WRITE_PROGRAM{
    version WRITE_VERSION{
        int rpc_write(struct dtype7) = 1;
    } = 1;
} = 0x2000000A;

program READDIR_PROGRAM{
    version READDIR_VERSION{
        int rpc_readdir(struct dtype8) = 1;
    } = 1;
} = 0x2000000B;