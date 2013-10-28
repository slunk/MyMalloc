#include <stdio.h>

#define HEAP_SIZE (1024 * 1024)
#define HEAP_END (heap + HEAP_SIZE)
#define BLOCK(ptr) ((struct block *) ptr - 1)

char heap[HEAP_SIZE] = {0};

struct block {
    struct block *prev;
    struct block *next;
    int sz;
    char *buffer;
};

struct block *free_list = NULL;

void init_heap()
{
    struct block *tmp = (struct block *) heap;
    tmp->prev = NULL;
    tmp->next = NULL;
    tmp->sz = HEAP_SIZE - sizeof(*tmp);
    tmp->buffer = heap + sizeof(*tmp);
    free_list = tmp;
}

void print_free_list()
{
    struct block *tmp = free_list;
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

void *my_malloc(size_t sz)
{
    struct block *tmp = free_list;
    struct block *next = NULL;
    if (sz % 8)
        sz = sz - (sz % 8) + 8;
    while (tmp) {
        if (tmp->sz > sz) {
            next = tmp->next;
            if (tmp->sz > sz + sizeof(*next)) {
                next = (struct block *) (tmp->buffer + sz);
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

void coalesce_free_list()
{
    struct block *tmp = free_list;
    while (tmp->next) {
        if (tmp->buffer + tmp->sz == (char *) tmp->next) {
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

void my_free(void *ptr)
{
    struct block *tmp = free_list;
    while (tmp && (void *) tmp < ptr) {
        tmp = tmp->next;
    }
    BLOCK(ptr)->next = tmp;
    BLOCK(ptr)->prev = tmp->prev;
    if (tmp->prev) {
        tmp->prev->next = BLOCK(ptr);
    } else {
        free_list = BLOCK(ptr);
    }
    tmp->prev = BLOCK(ptr);
    coalesce_free_list();
}

int main(int argc, char *argv[])
{
    struct block *tmp = (struct block *) heap;
    int *p, *q, *r;
    init_heap();
    print_free_list();
    p = my_malloc(sizeof(*p));
    print_free_list();
    q = my_malloc(sizeof(*q));
    print_free_list();
    r = my_malloc(sizeof(*r));
    print_free_list();
    my_free(q);
    print_free_list();
    my_free(p);
    print_free_list();
    my_free(r);
    print_free_list();
    return 0;
}
