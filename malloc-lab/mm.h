#include <stdio.h>

extern int mm_init (void); // 외부 참조 가능
extern void *mm_malloc (size_t size); // 외부 참조 가능
extern void mm_free (void *ptr); // 외부 참조 가능
extern void *mm_realloc(void *ptr, size_t size); // 외부 참조 가능


/* 
 * Students work in teams of one or two.  Teams enter their team name, 
 * personal names and login IDs in a struct of this
 * type in their bits.c file.
 */
typedef struct {
    char *teamname; /* ID1+ID2 or ID1 */
    char *name1;    /* full name of first member */
    char *id1;      /* login ID of first member */
    char *name2;    /* full name of second member (if any) */
    char *id2;      /* login ID of second member */
} team_t; // team_t Type. all Pointer

extern team_t team; // 외부 참조 가능

