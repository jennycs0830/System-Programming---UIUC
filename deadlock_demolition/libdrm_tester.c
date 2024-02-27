/**
 * deadlock_demolition
 * CS 341 - Fall 2023
 */
#include "libdrm.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void* thread_func( void* arg ){
    drm_t *drm = (drm_t*) arg;

    pthread_t thread_id = pthread_self();
    int lock_acquired = drm_wait( drm,  &thread_id);
    assert( lock_acquired == 1 );

    printf( "Thread %lu is working ... \n", pthread_self() );
    sleep(1);

    int lock_released = drm_post( drm, &thread_id );
    assert( lock_released == 1 );

    return NULL;
}

int main() {
    drm_t *drm = drm_init();
    assert( drm != NULL );

    // TODO your tests here
    pthread_t thread1, thread2;
    pthread_create( &thread1, NULL, thread_func, drm );
    pthread_create( &thread2, NULL, thread_func, drm );

    pthread_join( thread1, NULL );
    pthread_join( thread2, NULL );

    drm_destroy(drm);

    return 0;
}
