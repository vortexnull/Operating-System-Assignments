typedef struct slist {
    char* data;
    struct slist* next;
} slist;

slist* s_cons(const char* text, slist* next);
void s_free(slist* s);
slist* s_split(const char* text, char delim);