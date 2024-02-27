/**
 * deadlock_demolition
 * CS 341 - Fall 2023
 */
#include "graph.h"
#include "libdrm.h"
#include "dictionary.h"
#include "set.h"
#include <pthread.h>

struct drm_t {
    pthread_mutex_t mtx;
};
graph* g = NULL;
pthread_mutex_t global = PTHREAD_MUTEX_INITIALIZER;

int DFS( graph* g, void* node, dictionary* visited, dictionary* stacked ){
    dictionary_set( visited, node, NULL );
    dictionary_set( stacked, node, NULL );

    vector* neighbors = graph_neighbors( g, node );
    for( size_t i = 0; i < vector_size( neighbors ); i++ ){
        void* neighbor = vector_get( neighbors, i );
        if( !dictionary_contains( visited, neighbor ) ){
            if( DFS( g, neighbor, visited, stacked ) )
                return 1;
        }
        else if( dictionary_contains( stacked, neighbor ) )
            return 1;
    }
    dictionary_remove( stacked, node );
    free( neighbors );
    return 0;
}

int isCycle( graph* g ){
    dictionary* visited = shallow_to_shallow_dictionary_create();
    dictionary* stacked = shallow_to_shallow_dictionary_create();

    vector* vertices = graph_vertices( g );
    for( size_t i = 0; i < vector_size( vertices ); i++ ){
        void* vertex = vector_get( vertices, i );
        if( !dictionary_contains( visited, vertex ) ){
            if( DFS( g, vertex, visited, stacked ) ){
                dictionary_destroy( visited );
                dictionary_destroy( stacked );
                vector_destroy( vertices );
                return 1;
            }
        }
    }
    dictionary_destroy( visited );
    dictionary_destroy( stacked );
    vector_destroy( vertices );
    return 0;
}

drm_t *drm_init() {
    /* Your code here */
    if( g == NULL )
        g = shallow_graph_create();
    drm_t* drm = malloc( sizeof( drm_t ) );
    if( drm == NULL ) return NULL;
    pthread_mutex_init( &drm->mtx, NULL );

    pthread_mutex_lock( &global );
    graph_add_vertex( g, drm );
    pthread_mutex_unlock( &global );
    
    return drm;
}

int drm_post(drm_t *drm, pthread_t *thread_id) {
    /* Your code here */
    pthread_mutex_lock( &global );
    if( drm == NULL ){
        pthread_mutex_unlock( &global );
        return 0;
    }
    if( !graph_adjacent( g, drm, thread_id ) ){
        pthread_mutex_unlock( &global );
        return 0;
    }
    graph_remove_edge( g, drm, thread_id );
    pthread_mutex_unlock( &drm->mtx );
    pthread_mutex_unlock( &global );
    return 1;
}

int drm_wait(drm_t *drm, pthread_t *thread_id) {
    /* Your code here */
    pthread_mutex_lock( &global );
    if( drm == NULL ) return 0;
    if( !graph_contains_vertex( g, drm ) ) return 0; // drm haven't be initialize 
    if( !graph_contains_vertex( g, thread_id ) ) 
        graph_add_vertex( g, thread_id );
    if( graph_adjacent( g, drm, thread_id ) ){
        pthread_mutex_unlock( &global );
        return 0; // thread already own the drm
    }

    graph_add_edge( g, thread_id, drm ); // thread request drm
    if( isCycle( g ) ){
        graph_remove_edge( g, thread_id, drm );
        pthread_mutex_unlock( &global );
        return 0;
    }
    else{
        pthread_mutex_unlock( &global );
        pthread_mutex_lock( &drm->mtx );
        pthread_mutex_lock( &global );
        graph_remove_edge( g, thread_id, drm );
        graph_add_edge( g, drm, thread_id );
        pthread_mutex_unlock( &global );
        return 1;
    }
    return 0;
}

void drm_destroy(drm_t *drm) {
    /* Your code here */
    pthread_mutex_destroy( &global );
    pthread_mutex_destroy( &drm->mtx );
    graph_remove_vertex( g, drm );
    free( drm );
    return;
}