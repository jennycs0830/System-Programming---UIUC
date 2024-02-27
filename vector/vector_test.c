/**
 * vector
 * CS 341 - Fall 2023
 */
#include <stdlib.h>
#include <stdio.h>
#include <aio.h>

#include "vector.h"

void print( vector* input );

int main(int argc, char *argv[]) {
    // Write your test cases here
    // string test case
    vector* str_test = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
    vector_push_back( str_test, "index0" );
    vector_push_back( str_test, "index1" );
    vector_push_back( str_test, "index2" );
    vector_push_back( str_test, "index3" );
    vector_push_back( str_test, "index4" );
    vector_push_back( str_test, "index5" );
    printf( "Create and push element\n" );
    print( str_test );
    printf( "\n" );
    // pop back
    vector_pop_back( str_test );
    vector_pop_back( str_test );
    printf( "after pop back *2\n" );
    print( str_test );
    printf( "\n" );
    // resize
    vector_resize( str_test, 10 );
    printf( "after resize\n" );
    print( str_test );
    printf( "\n" );
    // reserve
    vector_reserve( str_test, 20 );
    printf( "after reserve\n" );
    print( str_test );
    printf( "\n" );
    //insert
    vector_insert( str_test, 2, "Insert element" );
    printf( "after insert\n" );
    print( str_test );
    printf( "\n" );
    //set 
    vector_set( str_test, 2, "set" );
    printf( "after set\n" );
    print( str_test );
    printf( "\n" ); 
    //at
    void** at = vector_at( str_test, 2 );
    printf( "vector element at position 2 = %s\n", (char*) *at );
    printf( "\n" );
    //get 
    for (size_t i = 0; i < vector_size( str_test ); i++) {
      char* str = (char *) vector_get(str_test, i);
      printf("%zu:%c, ", i, *str);
    }
    printf("\n");
    //erase
    vector_erase( str_test, 2 );
    printf( "after erase\n" );
    print( str_test );
    printf( "\n" );
    // clear
    vector_clear( str_test );
    printf( "after clear\n" );
    print( str_test );
    printf( "\n" );
    //destroy
    vector_destroy( str_test );
    return 0;
}

void print( vector* input ){
    //start
    void** begin = vector_begin( input );
    printf( "The begin address of vector = %p\n", begin );
    //end
    void** end = vector_end( input );
    printf( "The end address of vector = %p\n", end );
    //size
    printf( "The size of vector = %zu\n", vector_size( input ) );
    //capacity
    printf( "The capacity of vector = %zu\n", vector_capacity( input ) );
    //empty
    printf( "If vector is empty = %d\n", vector_empty( input ) );
    //front
    void** front = vector_front( input );
    printf( "The front address of vector = %p\n", front );
    //back
    void** back = vector_back( input );
    printf( "The back address of vector = %p\n", back );
    //print whole vector
    printf("vector = ");
    for( size_t i=0; i<vector_size( input ); i++){
        void* get = vector_get( input, i );
        printf("%s, ", (char*) get);
    }
    printf("\n");
}
