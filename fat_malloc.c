#include <stdio.h>
#include "heap.h"

#define FAT_BLOCK(ptr) ((struct fat_block *) ptr - 1)

struct fat_block {
    struct fat_block *prev;
    struct fat_block *next;
    int sz;
    char *buffer;
};

static struct fat_block *free_list = NULL;

void fat_init_heap(struct heap_info *info)
{
    struct fat_block *tmp = (struct fat_block *) info->heap;
    tmp->prev = NULL;
    tmp->next = NULL;
    tmp->sz = info->size - sizeof(*tmp);
    tmp->buffer = info->heap + sizeof(*tmp);
    free_list = tmp;
}

void fat_print_free_list()
{
    struct fat_block *tmp = free_list;
    printf("----------\n");
    printf("Free list:\n");
    printf("----------\n");
    while (tmp) {
        printf("addr: %p\n", tmp);
        printf("size: %d\n", tmp->sz);
        printf("buffer: %p\n", tmp->buffer);
        printf("\n");
        tmp = tmp->next;
    }
}

void *fat_malloc(size_t sz)
{
    struct fat_block *tmp = free_list;
    struct fat_block *next = NULL;
    if (sz % 8)
        sz = sz - (sz % 8) + 8;
    while (tmp) {
        if (tmp->sz > sz) {
            next = tmp->next;
            if (tmp->sz > sz + sizeof(*next)) {
                next = (struct fat_block *) (tmp->buffer + sz);
                next->prev = tmp->prev;
                next->next = tmp->next;
                next->sz = tmp->sz - sz - sizeof(*next);
                next->buffer = (char *)(next + 1);
            }
            if (tmp->prev) {
                tmp->prev->next = next;
            }
            if (tmp->next) {
                tmp->next->prev = next;
            }
            if (free_list == tmp) {
                free_list = next;
            }
            tmp->sz = sz;
            return tmp->buffer;
        }
        tmp = tmp->next;
    };
    return NULL;
}

void fat_coalesce_free_list()
{
    struct fat_block *tmp = free_list;
    while (tmp->next) {
        if ((struct fat_block *) (tmp->buffer + tmp->sz) == tmp->next) {
            tmp->sz += tmp->next->sz + sizeof(*tmp->next);
            tmp->next = tmp->next->next;
            if (tmp->next) {
                tmp->next->prev = tmp;
            }
        } else {
            tmp = tmp->next;
        }
    }
}

void fat_free(void *ptr)
{
    struct fat_block *tmp = free_list;
    while (tmp && (void *) tmp < ptr) {
        tmp = tmp->next;
    }
    FAT_BLOCK(ptr)->next = tmp;
    FAT_BLOCK(ptr)->prev = tmp->prev;
    if (tmp->prev) {
        tmp->prev->next = FAT_BLOCK(ptr);
    } else {
        free_list = FAT_BLOCK(ptr);
    }
    tmp->prev = FAT_BLOCK(ptr);
    fat_coalesce_free_list();
}

struct alloc_algo fat_algo = {
    .init = fat_init_heap,
    .malloc = fat_malloc,
    .free = fat_free,
    .print_free_list = fat_print_free_list,
};
