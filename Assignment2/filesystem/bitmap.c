#include<stdint.h>

#include"bitmap.h"

int
bitmap_get(uint8_t* bm, int i)
{
    int offset = i % 8, res;
    uint8_t byte = bm[i / 8];

    res = byte >> (7 - offset) & 1;

    return res;
}

void
bitmap_put(uint8_t* bm, int i, int v)
{
    int offset = i % 8;

    if(bitmap_get(bm, i) == v)
        return;
    
    bm[i / 8] = bm[i / 8] ^ (1 << (7 - offset));
}