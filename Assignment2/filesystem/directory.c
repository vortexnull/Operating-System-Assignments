#include<string.h>
#include<assert.h>

#include"slist.h"
#include"inode.h"
#include"blocks.h"
#include"directory.h"

void
set_dirent(dirent* d, const char* name, int inum)
{
    if(strlen(name) > MAX_FNAME){
        strncpy(&(d->name), name, MAX_FNAME);
        strcat(&(d->name), '\0');
    }
    else
        strcpy(&(d->name), name);

    d->inum = inum;
}

void
init_directory_block(dirent* d)
{
    for(int i = 0; i < MAX_DIR_ENTRIES; i++)
        d[i].inum = -1;
}

int
directory_init()
{
    int inum = alloc_inum();
    inode* dd = get_inode(inum);
    dd->mode = 040755;
    dd->entries++;

    dirent* d = get_inode_bnum(dd, 0);
    init_directory_block(d);

    return inum;
}

void
directory_put(inode* dd, const char* name, int inum)
{   
    int lastblock = (dd->size - 1) / BLOCK_SIZE;

    dirent *d = get_inode_block(dd, lastblock);

    if(d[MAX_DIR_ENTRIES - 1].inum != -1){
        int i = dd->entries - lastblock * MAX_DIR_ENTRIES;
        set_dirent(&d[i], name, inum);
    }
    else{
        d = (dirent*) get_block(add_inode_blocks(dd, 1));
        init_directory_block(d);
        set_dirent(d, name, inum);
    }

    dd->size += sizeof(dirent);
    dd->entries++;
}

int
directory_lookup(inode* dd, const char* name)
{
    dirent* d;
    int lastblock = (dd->size - 1) / BLOCK_SIZE;

    for(int blockindex = 0; blockindex <= lastblock; blockindex++){
        d = (dirent*) get_inode_block(dd, blockindex);
        
        for(int i = 0; i < MAX_DIR_ENTRIES; i++){
            if(d[i].inum != -1 && streq(d[i].name, name))
                return d[i].inum;
        }
    }

    return -ENOENT;
}

int
tree_lookup(const char* path)
{
    assert(path[0] = '/');

    if(streq(path, "/"))
        return 0;

    path++;
    int inum = 0;
    slist* pathlist = s_split(path, '/');

    while(pathlist){
        inode* dd = get_inode(inum);
        inum = directory_lookup(inum, pathlist->data);

        if(inum < 0)
            return -ENOENT;

        pathlist = pathlist->next;
    }

    return inum;
}

void
directory_delete(inode* dd, const char* name)
{
    dirent* d;
    int lastblock = (dd->size - 1) / BLOCK_SIZE;
    int blockindex = 0;

    for(; blockindex <= lastblock; blockindex++){
        d = (dirent*) get_inode_block(dd, blockindex);
        
        for(int i = 0; i < MAX_DIR_ENTRIES; i++){
            if(d[i].inum != -1 && streq(d[i].name, name)){
                d = &d[i];
                goto exitloop;
            }
        }
    }

    if(blockindex > lastblock){
        printf("directory_delete(): no entry found");
        return;
    }

    exitloop:

    int inum = d->inum;
    int lastindex = dd->entries - lastblock * MAX_DIR_ENTRIES - 1;
    dirent* dtmp = get_inode_block(dd, lastblock);
    set_dirent(d, dtmp[lastindex].name, dtmp[lastindex].inum);

    inode* deletednode = get_inode(inum);

    if(deletednode->entries >= 0){
        slist* dirlist = directory_list(inum);

        for(; dirlist != 0; dirlist = dirlist->next)
            directory_delete(deletednode, dirlist->data);
    }

    free_inode(inum);

    dtmp[lastindex].inum = -1;

    if(lastindex == 0)
        rem_inode_block(dd, 1);

    dd->size -= sizeof(dirent);
    dd->entries--;
}

slist*
directory_list(int inum)
{
    inode* dd = get_inode(inum);

    dirent* d;
    slist* s = 0;
    int lastblock = (dd->size - 1) / BLOCK_SIZE;

    for(int blockindex = 0; blockindex <= lastblock; blockindex++){
        d = (dirent*) get_inode_block(dd, blockindex);
        
        for(int i = 0; i < MAX_DIR_ENTRIES && d[i].inum != -1; i++)
            s = s_cons(d[i].name, s);
    }

    return s;
}