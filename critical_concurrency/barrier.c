/**
 * critical_concurrency
 * CS 341 - Fall 2023
 */
#include "barrier.h"

// init code done by chatgpt
// The returns are just for errors if you want to check for them.
int barrier_destroy(barrier_t *barrier) {
    int error = 0;
    if( (error = pthread_mutex_destroy( &barrier->mtx )) != 0 )
        return error;
    if( (error = pthread_cond_destroy( &barrier->cv )) != 0 )
        return error;

    return 0;
}

int barrier_init(barrier_t *barrier, unsigned int num_threads) {
    int error = 0;
    if( (error = pthread_mutex_init( &barrier->mtx, NULL )) != 0 )
        return error;
    if( (error = pthread_cond_init( &barrier->cv, NULL )) != 0 )
        return error;
    barrier->count = 0;
    barrier->n_threads = num_threads;
    barrier->times_used = 0;

    return 0;
}

int barrier_wait(barrier_t *barrier) {
    pthread_mutex_lock( &barrier->mtx );
    
    barrier->count ++;
    if( barrier->count < barrier->n_threads ){
        unsigned int stage = barrier->times_used;
        while( stage == barrier->times_used )
            pthread_cond_wait( &barrier->cv, &barrier->mtx );
    }
    else{
        barrier->count = 0;
        barrier->times_used ++;
        pthread_cond_broadcast( &barrier->cv );
    }
    pthread_mutex_unlock( &barrier->mtx );
    return barrier->times_used;
}
