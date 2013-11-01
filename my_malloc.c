#include <stdio.h>
#include "heap.h"
#include "thin_malloc.h"
#include "fat_malloc.h"

#define HEAP_SIZE (1024 * 1024)

char heap[HEAP_SIZE] = {0};

extern struct alloc_algo fat_algo;
extern struct alloc_algo thin_algo;

static struct heap_info _info = {
    .heap = heap,
    .size = HEAP_SIZE,
#ifdef USE_FAT_MALLOC
    .algo = &fat_algo,
#else
    .algo = &thin_algo,
#endif
};

void init_heap()
{
    _info.algo->init(&_info);
}

void *malloc(size_t sz)
{
    return _info.algo->malloc(sz);
}

void free(void *ptr)
{
    _info.algo->free(ptr);
}

void print_free_list()
{
    _info.algo->print_free_list();
}

int main(int argc, char *argv[])
{
    struct block *tmp = (struct block *) heap;
    int *p, *q, *r;
    init_heap();
    print_free_list();
    p = malloc(sizeof(*p));
    print_free_list();
    q = malloc(sizeof(*q));
    print_free_list();
    r = malloc(sizeof(*r));
    print_free_list();
    free(q);
    print_free_list();
    free(p);
    print_free_list();
    free(r);
    print_free_list();
    return 0;
}
