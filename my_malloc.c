#include "my_malloc.h"

static heap_t heap = {
    .classes = {0},
    .heap = NULL,
    .size = 0,
    .free_space = 0,
};

/* Helpers */

/* Returns the size's class:
 * 0-7 are for bytes from 16 to 128 bytes
 * 8 is for bytes between 144 and 256 bytes
 * 9 is for bytes > 256 bytes
 */
static int get_size_class(size_t size)
{
    if (size <= 128)
        return (size - 16) >> 4;
    if (size <= 256)
        return 8;
    return 9;
}

/*
    Round the given size to the LOCAL_ALIGNMENT
*/
static size_t round_to_align(size_t size)
{
    return (size < 16) ? 16 : ((size + 15) & ~15);
}

/*
    Footer: [prev_link][next_link|1]
    - if the size of the chunk is 16 (LOCAL_ALIGNMENT) the LSB is set.
*/
static void set_footer(chunk_t *chunk, chunk_t *prev_link, chunk_t *next_link)
{
    // uint64_t *footer = (void *)((char *)chunk + chunk->size + sizeof(chunk_t));
    uint64_t *footer = (void *)chunk + chunk->size + sizeof(chunk_t);
    if (chunk->size == 16)
    {
        footer[-2] = (uint64_t)prev_link;
        footer[-1] = (uint64_t)next_link | 1;
    }
    else
    {
        footer[-3] = chunk->size;
        footer[-2] = (uint64_t)prev_link;
        footer[-1] = (uint64_t)next_link;
    }
}

/* ===== Chunks Navigation ===== */

/*
    Returns the chunk after a given chunk in the the heap.
*/
static chunk_t *next_chunk(chunk_t *chunk)
{
    if (chunk->last)
        return NULL;
    return (void *)chunk + sizeof(chunk_t) + chunk->size;
    // return (void *)((char *)chunk + sizeof(chunk_t) + chunk->size);
}

/*
    Returns the the chunk before a given chunk in the heap.
*/
static chunk_t *previous_chunk_if_free(chunk_t *chunk)
{
    if (chunk->prev_used)
        return NULL;
    uint64_t *footer = (void *)chunk;
    uint64_t previous_size = (footer[-1] & 1) ? 16 : footer[-3];
    return (void *)chunk - previous_size - sizeof(chunk_t);
    // return (void *)((char *)chunk - previous_size - sizeof(chunk_t));
}

/* ===== Free List Navigation ===== */

/*
    Returns the chunk after a given chunk in the free list (heap.classes).
*/
static chunk_t *next_link(chunk_t *chunk)
{
    // uint64_t *footer = (void *)((char *)chunk + sizeof(chunk_t) + chunk->size);
    uint64_t *footer = (void *)chunk + sizeof(chunk_t) + chunk->size;
    return (chunk_t *)(footer[-1] & ~3);
}

/*
    Returns the chunk before a given chunk in the free list (heap.classes).
*/
static chunk_t *prev_link(chunk_t *chunk)
{
    // uint64_t *footer = (void *)((char *)chunk + sizeof(chunk_t) + chunk->size);
    uint64_t *footer = (void *)chunk + sizeof(chunk_t) + chunk->size;
    return (chunk_t *)footer[-2];
}

/* ===== Free List Operations ===== */

/*
    Removes a given chunk from the free list (heap.classes).
*/
static void remove_link(chunk_t *chunk)
{
    chunk_t *prev = prev_link(chunk);
    chunk_t *next = next_link(chunk);

    if (prev)
        set_footer(prev, prev_link(prev), next);
    if (next)
        set_footer(next, prev, next_link(next));

    int size_class = get_size_class(chunk->size);
    if (heap.classes[size_class] == chunk)
        heap.classes[size_class] = next;

    heap.free_space -= chunk->size;
}

/*
    Adds a given chunk to the correct linked list in the free list (heap.classes).
*/
static void prepend_link(chunk_t *chunk)
{
    int c = get_size_class(chunk->size);

    chunk_t *first = heap.classes[c];
    set_footer(chunk, NULL, first);
    if (first)
        set_footer(first, chunk, next_link(first));

    heap.classes[c] = chunk;

    heap.free_space += chunk->size;
}

/* ===== Heap Expansion =====*/

/*
    Return the number of bytes that can hold a given size with:
    - minimum: EXPANSION_SIZE
    - maximum: MAX_CHUNK_SIZE
*/
static size_t expand_size(size_t size)
{
    for (size_t i = EXPANSION_SIZE; i < MAX_CHUNK_SIZE; i += EXPANSION_SIZE)
        if (size < i)
            return i;

    return 0;
}

/*
    Expand the heap by creating a new free chunk that can hold a given size.
*/
static void expand_heap(size_t size)
{
    size_t sz = expand_size(size);
    if (sz == 0)
    {
        fprintf(stderr, "Size requested is too large.\n");
        exit(EXIT_FAILURE);
    }

    void *ptr = sbrk(sz + sizeof(chunk_t));
    if (ptr == (void *)-1)
    {
        perror("sbrk() failed.");
        exit(EXIT_FAILURE);
    }

    /* Set the chunk's metadata */
    chunk_t *chunk = ptr;
    chunk->size = sz;
    chunk->used = false;
    chunk->last = true;
    chunk->prev_used = true;

    if (heap.size == 0)
        heap.heap = ptr;

    heap.size += sz;

    prepend_link(chunk);
}

/*
    Initialize the heap by creating a chunk of size EXPANSION_SIZE
*/
// static void init_my_heap(size_t size)
// {
//     size_t sz = expand_size(size);

//     void *ptr = sbrk(sz + sizeof(chunk_t));
//     if (ptr == (void *)-1)
//     {
//         perror("sbrk() failed.");
//         exit(EXIT_FAILURE);
//     }

//     /* Set the chunk's metadata */
//     chunk_t *chunk = ptr;

//     // header
//     chunk->size = sz;
//     chunk->used = false;
//     chunk->last = true;
//     chunk->prev_used = true;

//     // footer (prepend() call this)
//     // set_footer(chunk, NULL, NULL);

//     /* Set the heap metadata */
//     heap.heap = ptr;

//     heap.size += sz;

//     prepend_link(chunk);
// }

/* ===== Chunks Operations ===== */

/*
    Returns the best fit chunk for a given size.
*/
static chunk_t *best_fit(chunk_t *chunk, size_t size)
{
    chunk_t *best_match = NULL;
    size_t best_size = MAX_CHUNK_SIZE;

    while (chunk && best_size != size)
    {
        if (chunk->size >= size && chunk->size < best_size)
        {
            best_match = chunk;
            best_size = chunk->size;
        }

        chunk = next_link(chunk);
    }

    return best_match;
}

/*
    Split a given chunk to a chunk of a given size and a second chunk with the rest,
    and returns the second chunk.
    If a given chunk is too small to split NULL is returned.
*/
static chunk_t *split(chunk_t *chunk, size_t size)
{
    size_t extra_size = chunk->size - size;
    if (extra_size < sizeof(chunk_t) + LOCAL_ALIGNMENT)
        return NULL;

    // chunk_t *new = (void *)((char *)chunk + sizeof(chunk_t) + size);
    chunk_t *new = (void *)chunk + sizeof(chunk_t) + size;
    new->last = chunk->last;
    new->prev_used = chunk->used;
    new->size = extra_size - sizeof(chunk_t);
    new->used = false;

    chunk_t *next_of_new = next_chunk(new);
    if (next_of_new)
        next_of_new->prev_used = new->used;

    chunk->last = false;
    chunk->size = size;

    return new;
}

/*
    Merge two adjacents chunks.
*/
static void merge(chunk_t *left, chunk_t *right)
{
    size_t extra_size = sizeof(chunk_t) + right->size;
    left->size += extra_size;
    left->last = right->last;

    chunk_t *next = next_chunk(left);
    if (next)
        next->prev_used = left->used;
}

/*
    Returns the best fit chunk for a given size.
*/
static chunk_t *find_free_chunk(size_t size)
{
    chunk_t *chunk = NULL;
    for (int size_class = get_size_class(size);
         size_class < 10;
         ++size_class)
    {
        chunk_t *tmp = heap.classes[size_class];
        chunk = (size_class < 8) ? tmp : best_fit(tmp, size);
        if (chunk)
            break;
    }

    return chunk;
}

/* Malloc */
void *my_malloc(size_t size)
{
    if (size == 0)
        return NULL;

    size = round_to_align(size);

    if (heap.size == 0)
        expand_heap(size);

    chunk_t *c = find_free_chunk(size);

    // If failed it retry
    if (!c)
    {
        expand_heap(size);
        if (!(c = find_free_chunk(size)))
        {
            fprintf(stderr, "Memory allocation failed.\n");
            return NULL;
        }
    }

    remove_link(c);

    chunk_t *rest = split(c, size);
    if (rest)
        prepend_link(rest);

    c->used = true;
    chunk_t *next = next_chunk(c);
    if (next)
        next->prev_used = true;

    return (void *)c + sizeof(chunk_t);
}

/* Free */
void my_free(void *ptr)
{
    if (!ptr)
        return;

    // chunk_t *chunk = (void *)((char *)ptr - sizeof(chunk_t));
    chunk_t *chunk = ptr - sizeof(chunk_t);
    if (!chunk->used)
    {
        fprintf(stderr, "chunk already freed.\n");
        return;
    }
    chunk_t *prev = previous_chunk_if_free(chunk);
    chunk_t *next = next_chunk(chunk);

    chunk->used = false;
    if (next)
        next->prev_used = chunk->used;

    if (next && !next->used)
    {
        remove_link(next);
        merge(chunk, next);
    }

    if (prev)
    {
        remove_link(prev);
        merge(prev, chunk);
        chunk = prev;
    }

    prepend_link(chunk);
}
