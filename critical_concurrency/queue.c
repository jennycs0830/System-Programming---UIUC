/**
 * critical_concurrency
 * CS 341 - Fall 2023
 */
#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * This queue is implemented with a linked list of queue_nodes.
 */
typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node;

struct queue {
    /* queue_node pointers to the head and tail of the queue */
    queue_node *head, *tail;

    /* The number of elements in the queue */
    ssize_t size;

    /**
     * The maximum number of elements the queue can hold.
     * max_size is non-positive if the queue does not have a max size.
     */
    ssize_t max_size;

    /* Mutex and Condition Variable for thread-safety */
    pthread_cond_t cv;
    pthread_mutex_t m;
};

queue *queue_create(ssize_t max_size) {
    /* Your code here */
    queue* cur = malloc( sizeof(queue) );
    if( pthread_mutex_init( &cur->m, NULL ) != 0 ){
        free( cur );
        return NULL;
    }
    if( pthread_cond_init( &cur->cv, NULL ) != 0 ){
        free( cur );
        return NULL;
    }
    cur->max_size = max_size;
    cur->head = NULL;
    cur->tail = NULL;
    cur->size = 0;

    return cur;
}

void queue_destroy(queue *this) {
    /* Your code here */
    pthread_mutex_lock( &this->m );

    queue_node* cur ;
    while( this->head ){
        cur = this->head;
        this->head = this->head->next;
        free( cur );
    }

    this->head = NULL;
    this->tail = NULL;
    this->size = 0;

    pthread_mutex_unlock( &this->m );

    pthread_mutex_destroy( &this->m );
    pthread_cond_destroy( &this->cv );
    free( this );
}

void queue_push(queue *this, void *data) {
    /* Your code here */
    pthread_mutex_lock( &this->m );
    if( this->max_size > 0 ){
        while( this->size == this->max_size )
            pthread_cond_wait( &this->cv, &this->m );
    }
    
    // assign data to empty queue node
    queue_node* insert = malloc( sizeof(queue_node) );
    if( insert == NULL ){
        pthread_mutex_unlock( &this->m );
        return ;
    }
    insert->data = data;
    insert->next = NULL;

    queue_node* head = this->head;
    if( head == NULL ){
        this->head = insert;
        this->tail = insert;
    }
    else{
        this->tail->next = insert;
        this->tail = insert;
    }

    this->size ++;
    pthread_cond_broadcast( &this->cv );
    pthread_mutex_unlock( &this->m );
}

void *queue_pull(queue *this) {
    /* Your code here */
    pthread_mutex_lock( &this->m );
    while( this->size == 0 )
        pthread_cond_wait( &this->cv, &this->m );

    void* data = NULL;
    queue_node* cur;
    if( this->head != NULL ){
        cur = this->head;
        data = this->head->data;
        this->head = this->head->next;
        this->size --;
        free( cur );
    }

    pthread_cond_broadcast( &this->cv );
    pthread_mutex_unlock( &this->m );
    return (void*) data;
}
