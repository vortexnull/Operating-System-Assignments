#include<errno.h>
#include<stdio.h>
#include<string.h>
#include<assert.h>

#include"slist.h"
#include"inode.h"
#include"blocks.h"
#include"directory.h"

char*
skip_string(char* data)
{
    while (*data != 0)
        data++;

    return data + 1;
}

int
directory_init()
{
    int inum = alloc_inode();

    inode* node = get_inode(inum);
    node->mode = 040755;

    char* selfname = ".";
    char* parentname = "..";

    directory_put(node, selfname, inum);
    directory_put(node, parentname, inum);

    return inum;
}

int
directory_lookup(inode* dd, const char* name)
{
    char* data = get_inode_block(dd, 0);
    char* text = data;

    for (int i = 0; i < dd->entries; i++) {
        printf(" ++ lookup '%s' =? '%s' (%p)\n", name, text, text);

        if (strcmp(text, name) == 0) {
            text = skip_string(text);
            int inum = *(int*)(text);
            // printf("directory_lookup @ found inum: %d\n", inum);
            return inum;
        }

        text = skip_string(text);
        text += sizeof(int);
    }

    return -ENOENT;
}


int
tree_lookup(const char* path)
{
    assert(path[0] == '/');

    if (strcmp(path, "/") == 0)
        return 0;

    path += 1;

    int dd = 0;
    slist* pathlist = s_split(path, '/');

    while(pathlist){
        printf("\ntree_lookup -> %s\n", pathlist->data);
        inode* node = get_inode(dd);
        dd = directory_lookup(node, pathlist->data);

        if (dd < 0)
            return -1;
	 
        pathlist = pathlist->next;
    }

    return dd;
}

int
directory_put(inode* dd, const char* name, int inum)
{
    // printf("\ndirectory_put -> %s %d\n", name, inum);
    int len = strlen(name) + 1;
    if (dd->size + len + sizeof(inum) > BLOCK_SIZE)
        return -ENOSPC;

    char* data = get_inode_block(dd, 0);
    memcpy(data + dd->size, name, len);
    dd->size += len;

    memcpy(data + dd->size, &inum, sizeof(inum));
    dd->size += sizeof(inum);
    dd->entries += 1;

    return 0;
}

int
directory_delete(inode* dd, const char* name)
{
    char* data = get_inode_block(dd, 0);
    char* text = data;
    char* tmp = 0;

    for (int i = 0; i < dd->entries; i++) {
        if (strcmp(text, name) == 0)
            goto delete_found;

        text = skip_string(text);
        text += sizeof(int);
    }

    return -ENOENT;

delete_found: ;

    tmp = skip_string(text);
    int inum = *((int*) tmp);
    inode* deletednode = get_inode(inum);

    if(deletednode->entries >= 0){
        slist* dirlist = directory_list(inum);

        for(; dirlist != 0; dirlist = dirlist->next)
            directory_delete(deletednode, dirlist->data);
    }

    tmp += sizeof(int);

    int pos = (int)(tmp - data);
    // printf("text: %s | pos: %d | tmp : %s | sub: %d\n", text, pos, tmp, dd->size - pos);
    memmove(text, tmp, dd->size - pos);
    int len = (int)(tmp - text);
    dd->size -= len;
    dd->entries -= 1;

    free_inode(inum);

    return 0;
}

slist*
directory_list(int inum)
{
    inode* dd = get_inode(inum);
    char* data = get_inode_block(dd, 0);
    char* text = data;

    // printf("+ directory_list()\n");
    slist* s = 0;

    for (int i = 0; i < dd->entries; i++) {
        char* name = text;
        text = skip_string(text);
        int inum = *((int*) text);
        text += sizeof(int);

        // printf(" - %d: %s [%d]\n", i, name, inum);
        s = s_cons(name, s);
    }

    return s;
}