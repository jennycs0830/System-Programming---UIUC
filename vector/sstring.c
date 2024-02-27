/**
 * vector
 * CS 341 - Fall 2023
 */
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>

struct sstring {
    char* str;
    size_t size;
    // Anything you want
};

sstring *cstr_to_sstring(const char *input) {
    // your code goes here
    assert( input != NULL );
    sstring* output = ( sstring* ) malloc ( sizeof( sstring ) );
    if( output == NULL )
        assert( 0 );
    output->size = strlen( input );
    output->str =  malloc( output->size + 1 );
    strcpy( output->str, input );
    if( output->str ==  NULL ){
        free( output );
        return NULL;
    }
    return output;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
    assert( input != NULL );
    char* output = malloc( input->size * sizeof( char ) );
    if( output == NULL )
        return NULL;
    for(size_t i=0; i<input->size; i++){
        *( output + i ) = *(input->str + i);
    }
    return output;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
    assert( addition->str != NULL );
    size_t new_size = this->size + addition->size;
    char* new_str = (char*) realloc( this->str, new_size + 1 );
    if( this->str == NULL )
        return -1;
    strcpy( new_str + this->size, addition->str);
    this->str = new_str;
    this->size = new_size;
    return (int) new_size;
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
    vector* output = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
    char* start = this->str;
    while( start < this->str + this->size ){
        // by chatgpt
        char* token = strchr( start, delimiter );
        if( token != NULL){
            size_t len = token - start;
            char* substr = (char*) malloc ( (len+1) * sizeof(char) );
            if( substr == NULL ){
                vector_destroy( output );
                return NULL;
            }
            strncpy( substr, start, len );
            substr[ len ] = '\0';
            vector_push_back( output, substr );
            start = token + 1;
        }
        else{
            if( start < this->str + this->size )
                vector_push_back( output, start );
            break;
        }
        // to here
    }
    return output;
}

// substring
char *substring(sstring *this, int offset, int len){
    int str_len = strlen( sstring_to_cstr( this ) );
    if( offset < 0 || offset >= str_len || len < 0 )
        return NULL;
    char* output = (char*) malloc ( len* sizeof(char) );
    if( output == NULL )
        return NULL;
    strncpy( output, sstring_to_cstr(this+offset), len);
    output[len] = '\0';
    return output;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
    // revise by chatgpt
    size_t target_len = strlen( target );
    size_t substitution_len = strlen( substitution );
    if( offset >= this->size || target_len == 0 )
        return -1;
    char* locate = strstr( this->str + offset, target );
    if( locate != NULL ){
        size_t index = locate - this->str;
        char* buf = (char*) malloc ( this->size - target_len + substitution_len +1 );
        if( buf == NULL )
            return -1;
        strncpy( buf, this->str, index );
        buf[ index ] = '\0';
        strcat( buf, substitution );
        strcat( buf, locate + target_len );
        free( this->str );
        this->str = buf;
        this->size = strlen( buf );
        return 0;
    }
    else{
        return -1;
    }
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    size_t len = end - start + 1;
    char* output = (char*) malloc( len );
    if( output ==  NULL )
        return NULL;
    strncpy( output , this->str + start, len - 1);
    output[len] = '\0';
    return output;
}

void sstring_destroy(sstring *this) {
    free( this->str );
    this->str = NULL;
    free( this );
    this = NULL;
    // your code goes here
}
