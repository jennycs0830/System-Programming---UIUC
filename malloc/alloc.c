/**
 * malloc
 * CS 341 - Fall 2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFRAG 4
#define SEARCH 5
#define SPLIT 4
#define FREE 5

typedef struct metadata_t meta;
static size_t block_size[16] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144};
static meta* freeList_head[17];
static int defrag = 0;

struct metadata_t{
    size_t size;
    int free;
    meta* next;
    meta* prev;
};

int get_index( size_t size );
void block_coalescing( int index );
void removeblock( meta* cur );
void addblock( meta* cur );
meta* first_fit( size_t size );
meta* best_fit( size_t size );
void block_splitting( meta* cur, size_t size );
void combine_next( meta* cur );
void print_free_list( int index );

int get_index( size_t size ){
    for( int i = 0; i < 16; i++ ){
        if( size <= block_size[i] )
            return i;
    }
    return 16;
}

void block_coalescing( int index ){
    // fprintf( stderr, "block coalescing\n");
    if( freeList_head[ index ] == NULL ) return ;
    else{
        meta* cur = freeList_head[ index ];
        while( cur != NULL  ){
            if( cur->free == 1 )
                combine_next( cur );
            if( ((void*)((char*) cur + sizeof(meta) + cur->size)) >= sbrk(0) ){
                // fprintf( stderr, "before remove\n" );
                // for( int i = 0; i < 17; i++ ){
                //     print_free_list( i );
                // }
                removeblock( cur );
                cur->free = 0;
                // fprintf( stderr, "\nafter remove\n" );
                // for( int i = 0; i < 17; i++ ){
                //     print_free_list( i );
                // }
                sbrk( 0 - (cur->size + sizeof(meta)));
                return ;
            }
            cur = cur->next;
        }
    }
}

void removeblock( meta* cur ){
    if( cur == NULL ) return;
    int index = get_index( cur->size );
    if( cur->prev != NULL && cur->next != NULL ){
        cur->prev->next = cur->next;
        cur->next->prev = cur->prev;
    }
    else if( cur->prev != NULL && cur->next == NULL ){
        cur->prev->next = NULL;
    }
    else if( cur->prev == NULL && cur->next != NULL ){
        freeList_head[ index ] = cur->next;
        cur->next->prev = NULL;
    }
    else{
        freeList_head[ index ] = NULL;
    }
    cur->next = NULL;
    cur->prev = NULL;
    return ;
}

void addblock( meta* cur ){
    if( cur == NULL ) return ;
    int index = get_index( cur->size );
    // add in the front of the block list
    if( freeList_head[ index ] == NULL ){
        freeList_head[ index ] = cur;
        cur->prev = NULL;
        cur->next = NULL;
    }
    else{
        freeList_head[ index ]->prev = cur;
        cur->next = freeList_head[ index ];
        cur->prev = NULL;
        freeList_head[ index ] = cur;
    }
    return ;
}

meta* first_fit( size_t size ){
    int index = get_index( size );
    if( freeList_head[ index ] == NULL ) return NULL;

    meta* cur = freeList_head[ index ];
    while( cur != NULL ){
        if( cur->free == 1 && cur->size >= size ){
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

meta* best_fit( size_t size ){
    int index = get_index( size );
    if( freeList_head[ index ] == NULL ) return NULL;
    meta* cur ;
    meta* chosen = NULL;
    for( cur = freeList_head[ index ]; cur != NULL ; cur = cur->next ){
        if( cur->free == 1 && cur->size >= size ){
            if( !chosen || chosen->size > cur->size )
                chosen = cur;
        }
    }
    return chosen;
}

void block_splitting( meta* cur, size_t size ){
    // fprintf( stderr, "block splitting, requested size = %zu\n", size );
    // fprintf( stderr, "start splitting and sbrk(0) = %p\n", sbrk(0) );
    // fprintf( stderr, "cur block addr = %p, sizeof(meta) = %zu, block size = %zu\n", (void*) cur, sizeof(meta), cur->size );
    if( cur == NULL ) return ;
    if( cur->free == 0 ) return ;
    size_t overhead = sizeof( meta ) + SPLIT;
    if( cur->size - size > overhead ){
        // new block
        meta* new = (void*) cur + sizeof(meta) + size;
        // if( (void*) new >= sbrk(0) ) return;
        // fprintf( stderr, "new addr = %p\n", (void*) new );
        new->size = cur->size - size - sizeof(meta);
        new->free = 1;
        // fprintf( stderr, "before add\n");
        // for( int i = 0; i < 17; i++ ){
        //     print_free_list( i );
        // }
        addblock( new );
        // fprintf( stderr, "add block to list\n");
        // old block
        cur->size = size;
        // fprintf( stderr, "\nafter add block\n");
        // for( int i = 0; i < 17; i++ ){
        //     print_free_list( i );
        // }
    }
    // fprintf( stderr, "splitting exit\n");
}

void combine_next( meta* cur ){
    meta* next = (void*) ( cur + 1 ) + cur->size;
    if( (void*) next < sbrk(0) && next->free == 1 ){
        removeblock( next );
        next->free = 0;
        cur->size += sizeof(meta) + next->size;
    }
}

void print_free_list( int index ){
    fprintf( stderr, "list %d:\n", index );
    for( meta* cur = freeList_head[ index ]; cur; cur = cur->next ){
        fprintf( stderr, "( %p, size = %zu )\n", (void*)cur, cur->size );
    }
    return ;
}
/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
    // implement calloc!
    void* new = malloc( num * size );
    if( new != NULL ) 
        memset( new, 0, num * size );
    return new;
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
    // implement malloc!
    // fprintf( stderr, "check all free list!\n");
    // for( int i = 0; i < 17; i++ ){
    //     print_free_list( i );
    // }
    if( size == 0 ) return NULL;
    int index = get_index( size );
    if( index == 16 )
        block_coalescing( 16 );
    else if( index == 15 ){
        defrag ++;
        if( defrag > 6 ){
            block_coalescing( 15 );
            defrag = 0;
        }
    }

    meta* new = NULL;
    if( freeList_head[ index ] != NULL ){
        // if( index < SEARCH ){
        //     new = first_fit( size );
        // }
        // else{
        //     new = best_fit( size );
        // }
        new = (get_index( size ) >= SEARCH)? best_fit( size ): first_fit( size );
    }

    if( new != NULL ){
        removeblock( new );
        new->free = 0;
        block_splitting( new, size );
        return (void*) new + sizeof(meta);
    }
    else{
        new = sbrk( sizeof(meta) + size );
        if( new == (void*) -1 ) return NULL;
        new->free = 0;
        new->size = size;
        new->next = NULL;
        new->prev = NULL;
        // addblock( new );
        // fprintf( stderr, "sbrk(0) = %p\n", (void*)sbrk(0));
        return (void*) new + sizeof(meta);
    }
        
    return NULL;
}

/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
    // implement free!
    // fprintf( stderr, "free begin\n");
    // fprintf( stderr, "free list before:\n");
    // for( int i = 0; i < 17; i++ ){
    //     print_free_list( i );
    // }
    // fprintf( stderr, "ptr = %p\n", (void*) ptr );
    // fprintf( stderr, "sbrk(0) = %p\n", sbrk(0) );
    if( ptr == NULL || ptr >= sbrk(0) ) return;
    // fprintf( stderr, "start free normally\n");
    meta* cur = (meta*)ptr - 1;
    // fprintf( stderr, "cur addr = %p\n", (void*) cur );
    if( cur->free == 1 )  return ;
    combine_next( cur );
    int index = get_index( cur->size );
    // fprintf( stderr, "free list index = %d\n", index );
    if( index > FREE && ( ptr + cur->size >= sbrk(0) )){
        sbrk( 0 - ( cur->size + sizeof(meta) ) );
        // fprintf( stderr, "free by sbrk\n");
        // fprintf( stderr, "free list after:\n");
        // for( int i = 0; i < 17; i++ ){
        //     print_free_list( i );
        // }
        return ;
    }
    // fprintf( stderr, "free set to 1 and add back to list\n");
    cur->free = 1;
    addblock( cur );
    // fprintf( stderr, "free list after:\n");
    // for( int i = 0; i < 17; i++ ){
    //     print_free_list( i );
    // }
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    // implement realloc!
    // fprintf( stderr, "realloc ptr = %p, size = %zu\n", (void*) ptr, size );
    if( ptr == NULL ){
        return malloc( size );
    }
    if( size == 0 ){
        free( ptr );
        return NULL;
    }
    meta* cur = (void*) ptr - sizeof(meta);
    // fprintf( stderr, "before realloc current block size = %zu\n", cur->size );
    // int index = get_index( cur->size );
    // fprintf( stderr, "realloc list %d:\n", index );
    // print_free_list( index );

    if( cur->size >= size ){
        block_splitting( cur, size );
        // removeblock( cur );
        cur->free = 0;
        // fprintf( stderr, "return addr = %p\n", (void*) cur + sizeof(meta));
        // fprintf( stderr, "requested size = %zu\n", size );
        // fprintf( stderr, "cur->size = %zu\n", cur->size );
        // fprintf( stderr, "splitting current block case\n");
        // print_free_list( get_index( cur->size ) );
        return ptr;
    }
    else{
        combine_next( cur );
        if( cur->size >= size ){
            // block_splitting( cur, size );
            // if( cur->free == 1 )
            //     removeblock( cur );
            cur->free = 0;
            // fprintf( stderr, "combine block case\n");
            // print_free_list( get_index( cur->size ) );
            return ptr;
        }

        void* new_ptr = malloc( size );
        if( new_ptr == NULL ) return NULL;
        memcpy( new_ptr, ptr, cur->size );
        // meta* new = (void*) new_ptr - sizeof(meta);
        // fprintf( stderr, "new_ptr addr = %p\n", (void*) new_ptr );
        free( ptr );
        // fprintf( stderr, "malloc new memory case\n");
        // print_free_list( get_index( new->size ) );
        return new_ptr;
    }
}