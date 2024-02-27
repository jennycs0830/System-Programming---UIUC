/**
 * perilous_pointers
 * CS 341 - Fall 2023
 */
#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {
    // your code here
    first_step( 81 );
    
    int value2=132;
    second_step( &value2 );

    int* value3 = malloc( sizeof( int ) );
    *value3 = 8942;
    double_step( &value3 );

    char value4[] = {0, 0, 0, 0, 0, 15, 0, 0, 0};
    strange_step(value4);

    char* value5 = "lol";
    empty_step( (void*)value5 );

    char value6_s2[] = "uuuuuu";
    char* value6_s = value6_s2;
    two_step( (void*)value6_s, value6_s2 );

    char* value7 = "abdefg";
    three_step( value7, value7+2, value7+4 );


    char* value8 = "aaiq";
    // printf( "%c\n", value8[1]+8 );
    // printf( "%c\n", value8[2]);
    // printf( "%c\n", value8[2]+8);
    // printf( "%c\n", value8[3]);
    step_step_step( value8, value8, value8);

    char* value9_a = "A";
    it_may_be_odd(value9_a, 'A');

    char value10[] = "CS241,CS241,CS241";
    // printf("%s\n", strtok(value10, ","));
    tok_step( value10 );    

    char value11[5] = {1, 1, 1, 3, '\0'};
    the_end((void*)value11, (void*)value11);
    return 0;
}
