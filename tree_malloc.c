/* Copyright (c) 2015 Peter Enns
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* So... I tried to come up with something original and ended up with something
 * that is probably roughly equivalent to "buddy-allocation." At least now I
 * know that buddy-allocation is.
 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include "heap.h"

/* Note: MAX_SPLITS currenty must be <= 7 */
#define MAX_SPLITS 2
#define MIN_BLOCK_SIZE (2 * (sizeof(tree_block_t)))
#define RIGHT(block, level) ((tree_block_t *)(((size_t)block) + (block->size >> (level + 1))))
#define TREE_BUFFER(block) ((void *)(block + 1))
#define TREE_BLOCK(buffer) ((tree_block_t *)buffer - 1)

/* TODO: see if we can actually make do with less metadata.
 */
typedef struct _tree_block {
    /* It's convenient, but allocated bit is redundant with free_space
     * as far as I can tell.
     */
    unsigned char allocated : 1;
    /* Necessary but could be used more cleverly.
     */
    unsigned char split_flags : 7;
    /* It might be possible to calculate the parent pointer
     * on the fly... Not sure.
     */
    struct _tree_block *parent;
    /* Note: I think you can get rid of parent index
     * if you resign yourself to doing a little more
     * work on frees by always starting cleanups at
     * lowest split
     */
    int parent_index;
    /* I think size might actually be necessary. Not for allocations,
     * but for frees. Even if it isn't necessary, it's extremely convenient.
     */
    size_t size;
    /* 100% necessary.
     */
    size_t free_space;
} tree_block_t;

tree_block_t *free_tree = NULL;

/* TODO: split flags should really just be treated as a number
 * since you will never have a node that is split at a lower node
 * and not a higher one. This will allow more splits with less bits.
 */
static int is_split_at(tree_block_t *block, int level) {
    return level < MAX_SPLITS && ((block->split_flags >> level) & 0x1);
}

static void mark_split(tree_block_t *block, int level) {
    if (level < MAX_SPLITS) {
        block->split_flags |= 0x1 << level;
    }
}

static void unmark_split(tree_block_t *block, int level) {
    if (level < MAX_SPLITS) {
        block->split_flags &= ~(0x1 << level);
    }
}

static tree_block_t *left(tree_block_t *block, int level) {
    if (!is_split_at(block, level)) {
        return NULL;
    }
    return block;
}

static tree_block_t *right(tree_block_t *block, int level) {
    if (!is_split_at(block, level)) {
        return NULL;
    }
    return RIGHT(block, level);
}

/* Note: This MUST be used instead of the size
 * field whenever you need the size of a node
 * in an expression.
 */
static size_t tree_block_size(tree_block_t *block, int level) {
    return block->size >> level;
}

static int can_split(tree_block_t *block, int level) {
    return tree_block_size(block, level) >= MIN_BLOCK_SIZE && !is_split_at(block, MAX_SPLITS - 1);
}

static int lowest_split(tree_block_t *block) {
    int i = 0;
    for (i = 0; i < MAX_SPLITS - 1; i++) {
        if (!is_split_at(block, i)) {
            break;
        }
    }
    return i;
}

static void tree_block_init(tree_block_t *block, tree_block_t *parent, int parent_index, size_t size) {
    block->allocated = 0;
    block->split_flags = 0;
    block->parent = parent;
    block->parent_index = parent_index;
    block->size = size;
    block->free_space = size - sizeof(tree_block_t);
}

/* Note: This MUST be used instead of the free_space
 * field whenever you need the free space of a node
 * in an expression.
 */
static size_t free_space(tree_block_t *block, int level) {
    int i;
    size_t space = block->free_space;
    tree_block_t *tmp = NULL;
    for (i = 0; i < level; i++) {
        tmp = right(block, i);
        space -= tmp->free_space;
    }
    return space;
}

static int fits(tree_block_t *block, int level, size_t size) {
    return size <= free_space(block, level);
}

static int fits_after_split(tree_block_t *block, int level, size_t size) {
    return size <= (tree_block_size(block, level) >> 1) - sizeof(tree_block_t);
}

static void split(tree_block_t *block, int level) {
    tree_block_t *r = RIGHT(block, level);
    tree_block_init(r, block, level, tree_block_size(block, level) >> 1);
    mark_split(block, level);
}

/* My intuition is that this algorithm makes for a good compromise
 * between first fit and best fit. Speed-wise, I think it should
 * even be better than first fit on average. It will be more wasteful
 * than a best fit into a linked list in terms of internal
 * fragmentation.
 */
/* TODO: potential optimization: favor left instead of right when the
 * requested size is larger than half of the next smallest block size?
 */
static tree_block_t *_alloc_internal(tree_block_t *block, size_t size, int level) {
    tree_block_t *ret = NULL;
    if (!block || !fits(block, level, size)) {
        return NULL;
    }
    if (!is_split_at(block, level)) {
        /* Note: There are two reasons why you might not be able to split.
         * Either we've reached a point where it is wasteful to split (metadata
         * dominates the actual data), or 
         * the requested # of bytes wouldn't fit in half the size of the current
         * block.
         */
        if (!can_split(block, level) || !fits_after_split(block, level, size)) {
            /* TODO: I haven't thought of all the tweaks that would be necessary
             * here but I don't think it would be too hard to fall back on a
             * different heap allocator at this point to avoid excessive
             * internal fragmentation.
             */
            block->allocated = 1;
            block->free_space = free_space(block, 0) - free_space(block, level);
            return block;
        }
        split(block, level);
    }
    ret = _alloc_internal(right(block, level), size, 0);
    if (!ret) {
        ret = _alloc_internal(left(block, level), size, level + 1);
    }
    if (ret) {
        block->free_space = free_space(block, level + 1) + free_space(right(block, level), 0);
        assert(block->free_space >= 0);
        return ret;
    }
    return NULL;
}

static int is_free(tree_block_t *block, int level) {
    return !block->allocated && !is_split_at(block, level);
}

static int can_reclaim(tree_block_t *block, int level) {
    return is_free(left(block, level), level + 1) && is_free(right(block, level), 0);
}

static void _update_free_space(tree_block_t *block, size_t reclaimed) {
    block->free_space = reclaimed + free_space(block, 0);
    if (block->parent) {
        _update_free_space(block->parent, reclaimed);
    }
}

static void _reclaim_and_update_free_space(tree_block_t *block, int level) {
    int i;
    size_t reclaimed;
    for (i = level; i > 0; i--) {
        if (can_reclaim(block, i - 1)) {
            unmark_split(block, i - 1);
        } else {
            break;
        }
    }
    reclaimed = tree_block_size(block, i) - sizeof(tree_block_t) - free_space(block, i);
    if (i != 0) {
        // Once we can't reclaim any more space than we already have, just
        // update the free space in all the parents.
        _update_free_space(block, reclaimed);
    } else {
        block->free_space = reclaimed + free_space(block, 0);
        if (block->parent) {
            _reclaim_and_update_free_space(block->parent, block->parent_index + 1);
        }
    }
}

static void _free_internal(tree_block_t *block) {
    int level = lowest_split(block);
    block->allocated = 0;
    _reclaim_and_update_free_space(block, level);
}

static void _tree_block_print_tree(tree_block_t *block, int level, int tree_level) {
    int i = 0;
    for (i = 0; i < tree_level; i++) {
        printf("  ");
    }
    if (is_split_at(block, level)) {
        printf("*\n");
        _tree_block_print_tree(left(block, level), level + 1, tree_level + 1);
        _tree_block_print_tree(right(block, level), 0, tree_level + 1);
    } else {
        printf("%ld\n", free_space(block, level));
    }
}

static void _tree_block_print_mem(tree_block_t *block, int level) {
    int i = 0;
    if (is_split_at(block, level)) {
        _tree_block_print_mem(left(block, level), level + 1);
        _tree_block_print_mem(right(block, level), 0);
    } else {
        printf("|");
        for (i = tree_block_size(block, level); i > 0; i -= MIN_BLOCK_SIZE) {
            if (block->allocated) {
                printf("#");
            } else {
                printf("_");
            }
        }
    }
}

int is_power_of_two(unsigned int x) {
      return ((x != 0) && !(x & (x - 1)));
}

void tree_init_heap(struct heap_info *info) {
    free_tree = (tree_block_t *) info->heap;
    /* less jarring way of handling this error would be
     * to choose the next lowest power of two below
     * info->size
     */
    assert(is_power_of_two(info->size));
    tree_block_init(free_tree, NULL, 0, info->size);
}

void *tree_alloc(size_t size) {
    return TREE_BUFFER(_alloc_internal(free_tree, size, 0));
}

void tree_free(void *ptr) {
    _free_internal(TREE_BLOCK(ptr));
}

void tree_heap_print() {
    _tree_block_print_tree(free_tree, 0, 0);
    _tree_block_print_mem(free_tree, 0);
    printf("|\n");
}

struct alloc_algo tree_algo = {
    .init = tree_init_heap,
    .malloc = tree_alloc,
    .free = tree_free,
    .print_free_list = tree_heap_print,
};

void tree_example() {
    char heap[1024 * 1024];
    free_tree = (tree_block_t *)heap;
    tree_block_t *allocated = NULL;
    tree_block_init(free_tree, NULL, 0, 1024 * 1024);
    tree_heap_print();
    printf("free: %ld\n", free_tree->free_space);
    int *ptr = tree_alloc(sizeof(int));
    tree_heap_print();
    printf("free: %ld\n", free_tree->free_space);
    printf("%p\n", ptr);
    *ptr = 32;
    printf("%d\n", *ptr);
    /* Increase by 1 to force allocator to go left instead of right */
    int *ptr2 = tree_alloc(262144);
    tree_heap_print();
    printf("free: %ld\n", free_tree->free_space);
    printf("%p\n", ptr);
    tree_free(ptr);
    tree_heap_print();
    printf("free: %ld\n", free_tree->free_space);
    tree_free(ptr2);
    printf("free: %ld\n", free_tree->free_space);
}
