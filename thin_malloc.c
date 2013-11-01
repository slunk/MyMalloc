#include <stdio.h>
#include "heap.h"

#define THIN_BLOCK(ptr) ((struct thin_block *) ptr - 1)
#define BUFF(blk) ((char *) (blk + 1))

struct thin_block {
    struct thin_block *next;
    size_t sz;
};

static struct thin_block *free_list = NULL;

void thin_init_heap(struct heap_info *info)
{
    struct thin_block *tmp = (struct thin_block *) info->heap;
    tmp->next = NULL;
    tmp->sz = info->size - sizeof(*tmp);
    free_list = tmp;
}

void thin_print_free_list()
{
    struct thin_block *tmp = free_list;
    printf("----------\n");
    printf("Free list:\n");
    printf("----------\n");
    while (tmp) {
        printf("addr: %p\n", tmp);
        printf("size: %ld\n", tmp->sz);
        printf("buffer: %p\n", BUFF(tmp));
        printf("\n");
        tmp = tmp->next;
    }
}

void *thin_malloc(size_t sz)
{
    struct thin_block *last = NULL;
    struct thin_block *tmp = free_list;
    struct thin_block *next = NULL;
    if (sz % 8)
        sz = sz - (sz % 8) + 8;
    while (tmp) {
        if (tmp->sz > sz) {
            next = tmp->next;
            if (tmp->sz > sz + sizeof(*next)) {
                next = (struct thin_block *) (BUFF(tmp) + sz);
                next->next = tmp->next;
                next->sz = tmp->sz - sz - sizeof(*next);
            }
            if (last) {
                last->next = next;
            } else {
                free_list = next;
            }
            tmp->sz = sz;
            return BUFF(tmp);
        }
        last = tmp;
        tmp = tmp->next;
    };
    return NULL;
}

void thin_coalesce_free_list()
{
    struct thin_block *tmp = free_list;
    while (tmp->next) {
        if ((struct thin_block *) (BUFF(tmp) + tmp->sz) == tmp->next) {
            tmp->sz += tmp->next->sz + sizeof(*tmp->next);
            tmp->next = tmp->next->next;
        } else {
            tmp = tmp->next;
        }
    }
}

void thin_free(void *ptr)
{
    struct thin_block *last = NULL;
    struct thin_block *tmp = free_list;
    while (tmp && (void *) tmp < ptr) {
        last = tmp;
        tmp = tmp->next;
    }
    THIN_BLOCK(ptr)->next = tmp;
    if (last) {
        last->next = THIN_BLOCK(ptr);
    } else {
        free_list = THIN_BLOCK(ptr);
    }
    thin_coalesce_free_list();
}

struct alloc_algo thin_algo = {
    .init = thin_init_heap,
    .malloc = thin_malloc,
    .free = thin_free,
    .print_free_list = thin_print_free_list,
};
