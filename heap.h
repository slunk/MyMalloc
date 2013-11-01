#ifndef _HEAP_H
#define _HEAP_H
#include <stddef.h>
struct heap_info;

struct alloc_algo {
    void (*init)(struct heap_info *info);
    void *(*malloc)(size_t sz);
    void (*free)(void *ptr);
    void (*print_free_list)();
};

struct heap_info {
    char *heap;
    size_t size;
    struct alloc_algo *algo;
};
#endif
