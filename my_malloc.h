#ifndef MY_MALLOC
#define MY_MALLOC

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint32_t uint;

#define LOCAL_ALIGNMENT (16)                    // Alignment for local variables (64-bit)
#define EXPANSION_SIZE (128 * 1024)             // Initial expansion size for the heap
#define MAX_CHUNK_SIZE ((16 * 1024 * 1024) + 1) // Maximum chunk size (16MB)

typedef struct
{
    uint : 5;           // padding
    uint used : 1;      // is the chunk used
    uint last : 1;      // is the chunk last in the heap
    uint prev_used : 1; // is the previous used (helpful when merging)
    uint size : 24;     // MAX is 16MB
    uint : 32;          // padding
    uint : 32;          // padding
    uint : 32;          // padding
} chunk_t;

typedef struct
{
    chunk_t *classes[10]; /* This array (free list) is an array of doubly linked chunks, the nodes
                           * are the chunks, and the links stored in the footer of the chunk.
                           * The footer is set if a chunk is not used:
                           * Chunk: [header(chunk_t), size:16][size:chunk.size-16][footer:[prev_link][next_link|1], size:16]
                           *    - if the size of the chunk (chunk.size) is 16 the LSB of next_link is set.

                           * chunks are stored in this way:
                           * Index 0-7 are mapped to bytes from 16 to 128 bytes
                           * Index 8 is mapped to bytes between 144 and 256 bytes
                           * Index 9 is mapped to bytes > 256 bytes
                           */
    void *heap;
    size_t size;
    size_t free_space;
} heap_t;

void *my_malloc(size_t size);

void my_free(void *ptr);

#endif
