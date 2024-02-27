/**
 * teaching_threads
 * CS 341 - Fall 2023
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reduce.h"
#include "reducers.h"

/* You might need a struct for each task ... */
typedef struct reduce_t{
    int* list;
    size_t len;
    reducer func;
    int base_case;
} reduce_t;

/* You should create a start routine for your threads. */
void* start_routine( void* data );
size_t* thread_size( size_t list_len, size_t num_threads );

int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    /* Your implementation goes here */

    // calculate the number of element in each thread
    size_t* threadSize = thread_size( list_len, num_threads );

    // construct reduce_t data
    reduce_t* data = malloc( num_threads * sizeof(reduce_t) );
    size_t start = 0;
    size_t num = 0;
    for( size_t i = 0; i < num_threads; i++ ){
        if( threadSize[i] > 0 ){
            data[i].list = list + start;
            data[i].len = threadSize[i];
            data[i].func = reduce_func;
            data[i].base_case = base_case;
            start += threadSize[i];
            num ++;
        }
    }

    // pthread_create
    pthread_t* threads = malloc( num_threads * sizeof(pthread_t) );
    for( size_t i = 0; i < num; i++ ){
        pthread_create( threads + i, NULL, start_routine, data + i );
    }
    
    int* retvals = malloc( num_threads * sizeof(int) );
    for( size_t i = 0; i < num; i++ ){
        void *retval = NULL;
        pthread_join( threads[i], &retval );
        retvals[i] = *(int*) retval;            
        free( retval );
        retval = NULL;
    }

    int result = reduce( retvals, num, reduce_func, base_case );
    free( threads );
    threads = NULL;
    free( data );
    data = NULL;
    free( retvals );
    retvals = NULL;
    free( threadSize );
    threadSize = NULL;
    return result;
}

size_t* thread_size( size_t list_len, size_t num_threads ){
    size_t* threadSize = calloc( num_threads,  sizeof(size_t) );
    if( num_threads > list_len ){
        for( size_t i = 0; i < list_len; i++ )
            threadSize[i] = 1;
    }
    else{
        size_t len_thread = list_len / num_threads;
        size_t remainder = list_len % num_threads;
        for( size_t i = 0; i < num_threads; i++ ){
            if( i < remainder ){
                threadSize[i] = len_thread + 1;
            }
            else{
                threadSize[i] = len_thread;
            }
        }
    }
    return threadSize;
}

void* start_routine( void* data ){
    reduce_t* reduce_data = (reduce_t*) data;
    int* result = (int*) malloc( sizeof(int) );
    *result = reduce( reduce_data->list, reduce_data->len, reduce_data->func, reduce_data->base_case );
    return (void*) result;
}
