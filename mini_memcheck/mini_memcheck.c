/**
 * mini_memcheck
 * CS 341 - Fall 2023
 */
#include "mini_memcheck.h"
#include <stdio.h>
#include <string.h>

//variables
meta_data *head;
size_t total_memory_requested;
size_t total_memory_freed;
size_t invalid_addresses;


void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
    // your code here
    // fprintf( stderr, "run mini malloc function\n" );
    if( request_size == 0 ) 
        return NULL;

    // malloc memory for new entry
    void* mem = malloc( request_size + sizeof( meta_data ) );
    if( mem == NULL )
        return NULL;
    meta_data* new = (meta_data*) mem;

    // set variable
    new->request_size = request_size;
    new->filename = filename;
    new->instruction = instruction;
    new->next = head;

    // record total mem request
    total_memory_requested += new->request_size;
    head = new;
    
    return mem + sizeof( meta_data );
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction) {
    // your code here
    // fprintf( stderr, "run mini calloc function\n" ); 
    if( num_elements == 0 || element_size == 0 )
        return NULL;
    // void* mem = mini_malloc( num_elements * element_size, filename, instruction );
    void* mem = malloc( num_elements * element_size + sizeof( meta_data ) );
    if( mem == NULL )
        return NULL;

    meta_data* entry = (meta_data*) mem;
    entry->filename = filename;
    entry->instruction = instruction;
    entry->request_size = num_elements * element_size;
    entry->next = head;    

    head = entry;
    total_memory_requested += entry->request_size;

    memset( mem + sizeof(meta_data), 0, entry->request_size );
    return mem + sizeof(meta_data);
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {
    // your code here
    // fprintf( stderr, "run mini realloc function\n" );
    if( payload == 0 && request_size == 0 )
        return NULL;
    if( request_size == 0 ){
        mini_free( payload );
        return NULL;
    }
    // if( check_invalid( payload ) ){
    //     invalid_addresses ++;
    //     return NULL;
    // }
    if(  payload == NULL )
        return mini_malloc( request_size, filename, instruction );

    meta_data* previous = NULL;
    meta_data* cur = head;

    void* entry = payload - sizeof( meta_data );
    while( cur ){
        if( (void*)cur == entry )
            break;
        previous = cur;
        cur = cur->next;
    }
    if( cur == NULL ){
        invalid_addresses ++;
        return NULL;
    }
    
    meta_data* new = (meta_data*) realloc (cur, request_size + sizeof(meta_data) );
    if( new == NULL )
        return NULL;
    
    size_t pre_size = cur->request_size;
    if( pre_size <= request_size )
        total_memory_requested += (request_size - pre_size);
    else
        total_memory_freed += (pre_size - request_size);

    new->filename = filename;
    new->instruction = instruction;
    new->request_size = request_size;

    if( new != cur ){
        previous->next = new;
        new->next = cur->next;
        free(cur);
    }
    return new+1;
}

void mini_free(void *payload) {
    // your code here
    // fprintf( stderr,  "run mini_free function\n" );
    if( payload == NULL )
        return ;
    
    meta_data* cur = head;
    meta_data* previous = NULL;

    void* mem = payload - sizeof(meta_data);
    while( cur != NULL ){
        if( (void*) cur == mem)
            break;
        previous = cur;
        cur = cur->next;
    }
    if( cur == NULL ){
        invalid_addresses++;
        return ;
    }

    if( cur == head )
        head = cur->next;
    else
        previous->next = cur->next;
    total_memory_freed += cur->request_size;
}
