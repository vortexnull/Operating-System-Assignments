#include<time.h>
#include<sys/stat.h>
#include<sys/types.h>

#include"slist.h"

void storage_init(const char *path);
int storage_stat(const char *path, struct stat *st);
int storage_read(const char *path, char *buf, size_t size, off_t offset);
int storage_write(const char *path, const char *buf, size_t size, off_t offset);
int storage_truncate(const char *path, off_t size);
int storage_mknod(const char *path, int mode);
slist* storage_list(const char *path);
int storage_unlink(const char *path);
int storage_access(const char *path, int mode);
int storage_utimens(const char *path, const struct timespec ts[2]);