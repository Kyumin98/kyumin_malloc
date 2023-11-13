#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"

team_t team = {
    /* Team name */
    "Kyumin",
    /* First member's full name */
    "KYUMINKIM",
    /* First member's email address */
    "kyumin98",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

#define WSIZE       4
#define DSIZE       8
#define CHUNKSIZE   (1 << 12)
#define MAX(x, y)   ((x) > (y) ? (x) : (y))
#define PACK(size, alloc)   ((size) | (alloc))
#define GET(p)  (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))
#define GET_SIZE(p)    (GET(p) & ~0x7)
#define GET_ALLOC(p)   (GET(p) & 0x1)
#define HDRP(bp)    ((char *)(bp) - WSIZE)
#define FTRP(bp)    ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp)   (((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)   (((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE)))

static void *heap_listp;
static char *last_bp;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *next_fit(size_t asize);
static void place(void *bp, size_t asize);

int mm_init(void){
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) {
        return -1;
    }

    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));
    heap_listp += (2*WSIZE);

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }
    last_bp = (char *)heap_listp;
    return 0;
}

void *mm_malloc(size_t size){
    size_t asize;
    size_t extend_size;
    char *bp;

    if (size == 0) {
        return NULL;
    }

    if (size <= DSIZE) {
        asize = 2*DSIZE;
    }
    else {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    }

    if ((bp = next_fit(asize)) != NULL) {
        place(bp, asize);
        last_bp = bp;
        return bp;
    }

    extend_size = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extend_size/WSIZE)) == NULL) {
        return NULL;
    }
    place(bp, asize);
    last_bp = bp;
    return bp;
}

static void *next_fit(size_t asize){
    char *bp = last_bp;

    for (bp = NEXT_BLKP(bp); GET_SIZE(HDRP(bp)) != 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize) {
            last_bp = bp;
            return bp;
        }
    }

    bp = heap_listp;
    while (bp < last_bp) {
        bp = NEXT_BLKP(bp);
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize) {
            last_bp = bp;
            return bp;
        }
    }

    return NULL;
}

void mm_free(void *bp){
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) {
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        last_bp = bp;
        return bp;
    }
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    last_bp = bp;
    return bp;
}

static void place(void *bp, size_t asize){
    size_t current_size = GET_SIZE(HDRP(bp));

    if ((current_size - asize) >= (2 * (DSIZE))) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(current_size - asize, 0));
        PUT(FTRP(bp), PACK(current_size - asize, 0));
    }
    else {
        PUT(HDRP(bp), PACK(current_size, 1));
        PUT(FTRP(bp), PACK(current_size, 1));
    }
}

void *mm_realloc(void *ptr, size_t size){
    if (ptr == NULL) {
        return mm_malloc(size);
    }

    size_t oldSize = GET_SIZE(HDRP(ptr));

    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    void *newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;

    size_t copySize = (size < oldSize) ? size : oldSize;
    memcpy(newptr, ptr, copySize);
    mm_free(ptr);

    return newptr;
}