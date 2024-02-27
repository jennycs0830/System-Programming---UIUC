/**
 * critical_concurrency
 * CS 341 - Fall 2023
 */
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "queue.h"

// generate by chatgpt
void test_push_and_pull(){
    queue* q = queue_create(10);
    assert( q != NULL );

    int data = 42;
    queue_push( q, (void*) &data );

    int *pulled_data = (int*) queue_pull(q);
    assert( *pulled_data == 42 );
}

void* thread_func( void* arg ){
    queue* q = (queue*) arg;
    for( int i = 0; i < 1000; i++ ){
        int* num = malloc( sizeof(int) );
        *num = i;
        queue_push( q, (void*) num );

        int* pulled_num = (int*) queue_pull(q);
        free( pulled_num );
    }
    return NULL;
}

void test_multithreading(){
    queue* q = queue_create( 100 );
    assert( q != NULL );

    pthread_t threads[4];

    for( int i = 0; i < 4; i++ )
        assert( pthread_create( &threads[i], NULL, thread_func, q ) == 0 );
    
    for( int i = 0; i < 4; i++ )
        pthread_join( threads[i], NULL );
    queue_destroy( q );
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("usage: %s test_number return_code\n", argv[0]);
        exit(1);
    }
    
    int test_number = atoi( argv[1] );
    int return_code = atoi( argv[2] );

    switch( test_number ){
        case 1:
            printf("Running test_push_and_pull...\n");
            test_push_and_pull();
            break;
        case 2:
            printf("Running test_multithreading...\n");
            test_multithreading();
            break;
        default:
            printf("Invalid test_number %d. Exiting with code %d.\n", test_number, return_code);
            exit(return_code);
    }

    printf("Test %d passed. Exiting with code %d.\n", test_number, return_code);
    exit(return_code);

    return 0;
}
