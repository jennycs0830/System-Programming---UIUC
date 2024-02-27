/**
 * password_cracker
 * CS 341 - Fall 2023
 */

// initial code generate by chatgpt
#include "cracker2.h"
#include "libs/format.h"
#include "libs/utils.h"
#include "includes/queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <crypt.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

typedef struct thread_t {
    pthread_t thread;
    int thread_id;
    int hash_count;
} thread_t;

// global variable ( need mutex lock protext ! )
static pthread_mutex_t lock;
static pthread_barrier_t barrier_start, barrier_end;
static int num_threads;

// parsing input (global)
static char username[9];
static char hash[14];
static char prefix[9];

// outputs
static char* answer;
static int working;

void* password_cracker( void* arg );

int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    pthread_barrier_init( &barrier_start, NULL, thread_count + 1 );
    pthread_barrier_init( &barrier_end, NULL, thread_count + 1 );

    pthread_mutex_init( &lock, NULL );

    thread_t* thread_pool = (thread_t*) malloc ( thread_count * sizeof( thread_t ) );
    for( size_t i = 0; i < thread_count; i++ ){
        thread_pool[i].thread_id = i + 1;
        thread_pool[i].hash_count = 0;
        pthread_create( &thread_pool[i].thread, NULL, password_cracker, thread_pool + i);
    }

    num_threads = thread_count;
    //read task from stdin (redirect the file to stdin) 
    char buf[128];
    while( fgets( buf, sizeof( buf ), stdin ) ){
        sscanf( buf, "%s %s %s", username, hash, prefix );
        
        pthread_mutex_lock( &lock );
        v2_print_start_user( username );
        pthread_mutex_unlock( &lock );

        working = 1;
        answer = NULL;
        pthread_barrier_wait( &barrier_start );
        double startTime = getTime();
        double startCPUTime = getCPUTime();

        pthread_barrier_wait( &barrier_end );
        int hash_count = 0;
        for( size_t i = 0; i < thread_count; i++ ){
            hash_count += thread_pool[i].hash_count;
        }
        
        double endTime = getTime();
        double endCPUTime = getCPUTime();
        // check if find answer
        pthread_mutex_lock( &lock );
        if( answer == NULL )
            v2_print_summary( username, answer, hash_count, endTime - startTime, endCPUTime - startCPUTime, 1 );
        else
            v2_print_summary( username, answer, hash_count, endTime - startTime, endCPUTime - startCPUTime, 0 );
        
        pthread_mutex_unlock( &lock );
        free( answer );
    }

    working = 0;
    pthread_barrier_wait( &barrier_start );

    for( size_t i = 0; i < thread_count; i++ ){
        pthread_join( thread_pool[i].thread, NULL );
    }

    free( thread_pool );
    pthread_barrier_destroy( &barrier_start );
    pthread_barrier_destroy( &barrier_end );

    // Remember to ONLY crack passwords in other threads
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}

void* password_cracker( void* arg ){
    thread_t* thread = (thread_t*) arg;
    while( 1 ){
        pthread_barrier_wait( &barrier_start );
        if( working == 0 )
            break;
        
        char* cur_prefix = strdup( prefix );
        int prefixLen = getPrefixLength( prefix );
        int totalLen = strlen( prefix );
        int suffixLen = totalLen - prefixLen;

        long start_index, count = 0;
        getSubrange( suffixLen, num_threads, thread->thread_id, &start_index, &count );
        setStringPosition( cur_prefix + prefixLen, start_index );

        pthread_mutex_lock( &lock );
        v2_print_thread_start( thread->thread_id, username, start_index, cur_prefix );
        pthread_mutex_unlock( &lock );

        struct crypt_data cdata;
        cdata.initialized = 0;

        int result = 2;
        for( long i = 0; i < count; i++ ){
            if( answer != NULL ){
                result = 1;
                break;
            }

            thread->hash_count += 1;
            if( strcmp( crypt_r( cur_prefix, "xx", &cdata ), hash ) == 0 ){
                result = 0;
                answer = cur_prefix;
                break;
            }
            incrementString( cur_prefix );
        }

        if( result != 0 )
            free( cur_prefix );
        
        pthread_mutex_lock( &lock );
        v2_print_thread_result( thread->thread_id, thread->hash_count, result );
        pthread_mutex_unlock( &lock );

        pthread_barrier_wait( &barrier_end );
    }
    return NULL;
}