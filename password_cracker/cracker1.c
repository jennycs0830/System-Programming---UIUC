/**
 * password_cracker
 * CS 341 - Fall 2023
 */

// initial code generate by chatgpt
#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include "includes/queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <crypt.h>
#include <string.h>
#include <math.h>

typedef struct thread_t {
    pthread_t thread;
    int thread_id;
} thread_t;

// global variable ( need mutex lock protext ! )
static queue* task_queue;
static int num_tasks = 0;
static pthread_mutex_t lock;
static int success = 0;

void* password_cracker( void* arg );

int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    
    task_queue = queue_create( 0 );
    pthread_mutex_init( &lock, NULL );
    //read task from stdin (redirect the file to stdin) 
    char buf[128];
    while( fgets( buf, sizeof( buf ), stdin ) ){
        num_tasks ++;
        queue_push( task_queue, strdup(buf) );
    }
    queue_push( task_queue, NULL );

    thread_t* thread_pool = (thread_t*) malloc ( thread_count * sizeof( thread_t ) );
    for( size_t i = 0; i < thread_count; i++ ){
        thread_pool[i].thread_id = i + 1;
        pthread_create( &thread_pool[i].thread, NULL, password_cracker, thread_pool + i);
    }

    for( size_t i = 0; i < thread_count; i++ ){
        pthread_join( thread_pool[i].thread, NULL );
    }
    v1_print_summary( success, num_tasks - success );
    free( thread_pool );
    queue_destroy( task_queue );

    // Remember to ONLY crack passwords in other threads
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}

void* password_cracker( void* arg ){
    thread_t* thread = (thread_t*) arg;
    char* task = NULL;
    while( (task = queue_pull( task_queue )) != NULL ){
        // parsing task
        char username[9];
        char hash[14];
        char prefix[9];
        sscanf( task, "%s %s %s", username, hash, prefix );
        // debug
        // printf( "username = %s\n", username );
        // printf( "hash = %s\n", hash );
        // printf( "prefix = %s\n", prefix );

        // doing crack!
        v1_print_thread_start( thread->thread_id, username );
        int prefixLen = getPrefixLength( prefix );
        int totalLen = strlen( prefix );
        int result = 1; // not found
        int hash_count = 0;
        double startTime = getThreadCPUTime();

        if( prefixLen == totalLen ){
            result = 0;
        }
        else{
            int suffixLen = totalLen - prefixLen;

            setStringPosition( prefix + prefixLen, 0 );
            long count = pow( 26, suffixLen );
            struct crypt_data cdata;
            cdata.initialized = 0;

            for( int i = 0; i < count; i++ ){
                hash_count ++;
                if( strcmp( crypt_r(prefix, "xx", &cdata), hash ) == 0 ){
                    result = 0; // found
                    pthread_mutex_lock( &lock );
                    success ++;
                    pthread_mutex_unlock( &lock );
                    break;
                }
                incrementString( prefix );
            } 
        }
        v1_print_thread_result( thread->thread_id, username, prefix, hash_count, getCPUTime() - startTime, result );  
    }
    queue_push( task_queue, NULL );
    free( task );
    task = NULL;
    return NULL;
}