/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

team_t team = {
    /* Team name */
    "ateam", // teamname
    /* First member's full name */
    "Harry Bovik", // name1
    /* First member's email address */
    "bovik@cs.cmu.edu", // id1
    /* Second member's full name (leave blank if none) */
    "", // name2
    /* Second member's email address (leave blank if none) */
    ""}; // id2

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8 // 메모리 정렬 기준 (8바이트)

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7) // 입력받은 size를 가장 가까운 8의 배수로 올림함

#define SIZE_T_SIZE (ALIGN(sizeof(size_t))) // size_t 크기(8바이트)를 8의 배수로 정렬한 크기

#define WSIZE 4                                                         // Word Size 정의
#define DSIZE 8                                                         // Double Word Size 정의
#define CHUNKSIZE (1 << 12)                                             // 2^12 = 4096 = 4KB
#define MAX(x, y) ((x) > (y) ? (x) : (y))                               // x, y중 큰 값을 반환.
#define PACK(size, alloc) ((size) | (alloc))                            // size와 alloc을 or연산. size는 Block의 총 크기에서 할당 여부 플래그를 합치는것이다.
#define GET(p) (*(unsigned int *)(p))                                   // unsigned int * 크기만큼 p의 값을 역참조. (unsigned int * 크기만큼 p에서 값을 읽어오기.)
#define PUT(p, val) (*(unsigned int *)(p) = (val))                      // val값을 p에서 unsigned int * 크기 만큼 삽입.
#define GET_SIZE(p) (GET(p) & ~0x7)                                     // 헤더 값에서 하위 3비트를 0으로 지워 Block의 '순수 크기'만 추출함
#define GET_ALLOC(p) (GET(p) & 0x1)                                     // 헤더 값에서 맨 뒷자리만 남기고 앞을 다 지워 '할당 여부(0 또는 1)'만 추출함
#define HDRP(bp) ((char *)(bp) - WSIZE)                                 // Block의 시작점인 pointer를 기준으로 - WSIZE를 통해 Header의 시작점을 얻는다.
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)            // 현재 Block 포인터(bp)를 기준으로 Block의 맨 끝에 붙어있는 푸터(Footer)의 시작 주소를 얻는다.
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) // 현재 Block의 헤더(bp - WSIZE)에서 읽은 현재 Block 크기를 bp에 더해, 다음 Block의 bp(Payload 시작 주소)를 얻는다.
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) // 이전 Block의 푸터(bp - DSIZE)에서 읽은 이전 Block 크기를 bp에서 빼서, 이전 Block의 bp(Payload 시작 주소)를 얻는다.

/*
 * mm_init - initialize the malloc package.
 */
static char *heap_listp; // 16B

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) // 이전 블록, 다음 블록 둘다 할당되어있는가?
    {
        return bp; // return Block Pointer
    }

    else if (prev_alloc && !next_alloc) // 이전 Block은 사용 중이지만, 다음 Block은 비어 있는 상태
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); // 다음 블록의 크기를 현재 크기에 더한다.
        PUT(HDRP(bp), PACK(size, 0));          // 현재 블록 헤더에 새로운 크기와 가용 상태를 기록한다.
        PUT(FTRP(bp), PACK(size, 0));          // 합쳐진 블록의 새 풋터에 새로운 크기와 가용 상태를 기록한다.
    }

    else if (!prev_alloc && next_alloc) // 이전 Block은 비어 있고, 다음 Block은 사용 중인 상태
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));   // 이전 블록의 크기를 현재 크기에 더한다.
        PUT(FTRP(bp), PACK(size, 0));            // 현재 블록 풋터에 새로운 크기와 가용 상태를 기록한다.
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // 이전 블록 헤더에 새로운 크기와 가용 상태를 기록한다.
        bp = PREV_BLKP(bp);                      // 블록의 시작 주소가 되었으므로 포인터를 이전 블록으로 이동한다.
    }
    else
    { // (!prev_alloc && !next_alloc)
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void *find_fit(size_t size)
{
    char *bp = NEXT_BLKP(heap_listp);

    while (GET_SIZE(HDRP(bp)) != 0)
    {
        if (size <= GET_SIZE(HDRP(bp)) && GET_ALLOC(HDRP(bp)) == 0)
        {
            return bp;
        }
        bp = NEXT_BLKP(bp);
    }

    return NULL;
}

static void place(void *bp, size_t asize)
{

    if (GET_SIZE(HDRP(bp)) - asize >= 2 * DSIZE)
    {
        char *next_bp = (char *)bp + asize;

        // 남은 공간을 독립된 free 블록으로 만든다.
        PUT(HDRP(next_bp), PACK(GET_SIZE(HDRP(bp)) - asize, 0));
        PUT(FTRP(next_bp), PACK(GET_SIZE(HDRP(bp)) - asize, 0));

        // 앞 블록을 요청한 크기로 할당한다.
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
    }
    else
    {
        // 분할할 공간이 부족하면 기존 블록 전체를 할당한다.
        PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
        PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
    }
}

int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0); // Alignment padding 정렬 패딩
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));

    heap_listp += (2 * WSIZE);

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
// void *mm_malloc(size_t size)
// {
//     int newsize = ALIGN(size + SIZE_T_SIZE);
//     void *p = mem_sbrk(newsize);
//     if (p == (void *)-1)
//         return NULL;
//     else
//     {
//         *(size_t *)p = size;
//         return (void *)((char *)p + SIZE_T_SIZE);
//     }
// }

void *mm_malloc(size_t size)
{
    size_t asize; // block size
    size_t extendsize;
    char *bp;

    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    // free list for a fit
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    // No fit found. find_fit return is NULL
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}
/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
// void *mm_realloc(void *ptr, size_t size)
// {
//     void *oldptr = ptr;
//     void *newptr;
//     size_t copySize;

//     newptr = mm_malloc(size);// 새로운 영역을 할당한다.(size의 크기만큼)
//     if (newptr == NULL)// mm_malloc(size)가 정상적으로 실행됐는지를 검증한다.
//         return NULL;
//     copySize = GET_SIZE(HDRP(oldptr)) - 2 * WSIZE;// payload 크기 계산.

//     if (size < copySize)
//         copySize = size;

//     memcpy(newptr, oldptr, copySize);
//     mm_free(oldptr);
//     return newptr;
// }

void *mm_realloc(void *bp, size_t size)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    size_t bp_size = GET_SIZE(HDRP(bp)) - 2 * WSIZE;
    size_t prev_size = GET_SIZE(FTRP(PREV_BLKP(bp))) - 2 * WSIZE;
    size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp))) - 2 * WSIZE;

    size_t req_size = size;

    if (bp_size < req_size) // 크게 요구하는 크기보다 실제 블록 크기가 작아서 추가적으로 할당하는 경우다.
    {
        // 추가적으로 할당하는 경우에 prev_alloc과 next_alloc을 확인하여 합칠수있는지 확인한다.
        // 하지만 여기서 할당이 가능한 경우엔 현재 bp_size + 사용가능한 블록 size >= req_size 여야한다.

        if (!next_alloc && (bp_size + next_size + 2 * WSIZE >= req_size)) // 다음 블록이 할당 가능하고, 합치면 충분해야한다.
        {
            // 해야할것
            // 1. payload size 변경
            // 2. Header (size, 1)로 변경
            // 3. Footer (size, 1)로 변경

            bp_size += next_size + 4 * WSIZE; // 다음 블록의 크기를 현재 크기에 더한다.
            PUT(HDRP(bp), PACK(bp_size, 1));
            PUT(FTRP(bp), PACK(bp_size, 1));

            return bp;
        }
        else if (!prev_alloc && (prev_size + bp_size + 2 * WSIZE >= req_size))
        {
            // 이전 + 현재 블록 병합
            void *new_bp = PREV_BLKP(bp);
            size_t merged_size = prev_size + bp_size + 4 * WSIZE;

            // 기존 데이터 보존 후 헤더·푸터 갱신
            memmove(new_bp, bp, bp_size);
            PUT(HDRP(new_bp), PACK(merged_size, 1));
            PUT(FTRP(new_bp), PACK(merged_size, 1));

            return new_bp;
        }
        else if (!prev_alloc && !next_alloc && (prev_size + bp_size + next_size + 4 * WSIZE >= req_size))
        {
            // 이전 + 현재 + 다음 블록 병합
            void *new_bp = PREV_BLKP(bp);
            size_t merged_size = prev_size + bp_size + next_size + 6 * WSIZE;

            memmove(new_bp, bp, bp_size);
            PUT(HDRP(new_bp), PACK(merged_size, 1));
            PUT(FTRP(new_bp), PACK(merged_size, 1));

            return new_bp;
        }
        else
        {
            // 병합으로 확보할 수 없으면 새 공간에 복사
            void *new_bp = mm_malloc(size);
            if (new_bp == NULL)
                return NULL;

            // 바깥 조건이 bp_size < req_size이므로 기존 공간 크기만큼 복사
            memcpy(new_bp, bp, bp_size);
            mm_free(bp);

            return new_bp;
        }
    }
    else
    {
        return bp;
    }
}
