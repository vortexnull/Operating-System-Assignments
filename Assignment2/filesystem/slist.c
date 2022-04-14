#include<alloca.h>
#include<stdlib.h>
#include<string.h>

#include"slist.h"

slist*
s_cons(const char* text, slist* next)
{
    slist* new = malloc(sizeof(slist));
    new->data = strdup(text);
    new->next = next;
    
    return new;
}

void
s_free(slist* s)
{
    if(s == 0)
        return;

    s_free(s->next);
    free(s->data);
    free(s);
}


slist*
s_split(const char* text, char delim)
{
    if(*text == 0)
        return 0;
    
    int partlen = 0;
    while(text[partlen] != delim && text[partlen] != 0)
        partlen++;
    
    int skip = 0;
    if(text[partlen] == delim)
        skip++;
    
    slist* next = s_split(text + partlen + skip, delim);
    char *part = alloca(partlen + 1);
    memcpy(part, text, partlen);
    part[partlen] = 0;

    return s_cons(part, next);
}