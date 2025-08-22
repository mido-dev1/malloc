#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "my_malloc.h"

void test_basic()
{
    void *p = my_malloc(16);
    assert(p != NULL);
    memset(p, 0xAB, 16); // write to the block
    my_free(p);
}

void test_alignment()
{
    for (size_t i = 1; i < 1024; i++)
    {
        void *p = my_malloc(i);
        assert(((uintptr_t)p % 16) == 0); // e.g., 16-byte alignment
        my_free(p);
    }
}

void test_various_sizes()
{
    void *ptrs[1000];
    for (int i = 0; i < 1000; i++)
    {
        size_t sz = (i % 128) + 1;
        ptrs[i] = my_malloc(sz);
        memset(ptrs[i], 0xCD, sz);
    }
    for (int i = 0; i < 1000; i++)
    {
        my_free(ptrs[i]);
    }
}

void test_reuse()
{
    void *a = my_malloc(64);
    my_free(a);
    void *b = my_malloc(64);
    assert(a == b); // Ideally, should reuse same block
    my_free(b);
}

void test_random()
{
    srand(time(NULL));
    void *ptrs[1000] = {0};
    for (int i = 0; i < 100000; i++)
    {
        int idx = rand() % 1000;
        if (ptrs[idx])
        {
            my_free(ptrs[idx]);
            ptrs[idx] = NULL;
        }
        else
        {
            size_t sz = (rand() % 511) + 1;
            ptrs[idx] = my_malloc(sz);
            if (ptrs[idx])
                memset(ptrs[idx], 0xEF, sz);
        }
    }
    for (int i = 0; i < 1000; i++)
    {
        my_free(ptrs[i]);
    }
}

void test_my_realloc()
{
    // Case 1: realloc(NULL, size) -> malloc
    void *p = my_realloc(NULL, 32);
    assert(p != NULL);
    memset(p, 0xAA, 32);

    // Case 2: Shrink
    p = my_realloc(p, 16);
    assert(p != NULL);
    for (int i = 0; i < 16; i++)
    {
        assert(((unsigned char *)p)[i] == 0xAA); // old data preserved
    }

    // Case 3: Grow (may or may not move)
    p = my_realloc(p, 64);
    assert(p != NULL);
    for (int i = 0; i < 16; i++)
    {
        assert(((unsigned char *)p)[i] == 0xAA); // old data still preserved
    }
    memset(p, 0xBB, 64);

    // Case 4: realloc to 0 -> free
    void *q = my_realloc(p, 0);
    assert(q == NULL);

    // Case 5: Grow beyond likely inplace (force move test)
    p = my_malloc(32);
    assert(p != NULL);
    memset(p, 0xCC, 32);
    p = my_realloc(p, 4096); // large grow, probably moves
    assert(p != NULL);
    for (int i = 0; i < 32; i++)
    {
        assert(((unsigned char *)p)[i] == 0xCC); // data preserved after move
    }
    my_free(p);
}

int main()
{
    test_basic();
    test_alignment();
    test_various_sizes();
    test_reuse();
    test_random();
    test_my_realloc();
    printf("All tests passed!\n");

    return 0;
}
