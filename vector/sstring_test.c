/**
 * vector
 * CS 341 - Fall 2023
 */
#include "sstring.h"
#include "string.h"
#include "vector.h"

void print_vector( vector* input );

int main(int argc, char *argv[]) {
    // TODO create some tests
    // sstring_append
    printf("Test sstring_append function\n");
    sstring *str1 = cstr_to_sstring("abc");
    sstring *str2 = cstr_to_sstring("def");
    int len = sstring_append(str1, str2); // len == 6
    printf("len = %d\n", len);
    printf("%s\n\n", sstring_to_cstr(str1)); // == "abcdef"

    // sstring_split
    printf("Test sstring_split function\n");
    vector* output1 = sstring_split(cstr_to_sstring("abcdeefg"), 'e'); // == [ "abcd", "", "fg" ]);
    print_vector( output1 );
    vector* output2 = sstring_split(cstr_to_sstring("This is a sentence."), ' ');
    print_vector( output2 );
    printf("\n");

    // sstring_substitute
    printf("Test sstring_substitute function\n");
    sstring *replace_me = cstr_to_sstring("This is a {} day, {}!");
    // printf("%s\n", replace_me);
    sstring_substitute(replace_me, 18, "{}", "friend");
    char* cstr_replace = sstring_to_cstr(replace_me); // == "This is a {} day, friend!"
    printf("%s\n", cstr_replace);
    sstring_substitute(replace_me, 0, "{}", "good");
    cstr_replace = sstring_to_cstr(replace_me);
    printf("%s\n\n", cstr_replace);

    // Test sstring_slice function
    printf("Test sstring_slice function\n");
    sstring *slice_me = cstr_to_sstring("1234567890");
    printf("%s\n\n", sstring_slice(slice_me, 2, 5));

    // Test sstring_destroy function
    sstring_destroy(str1);
    sstring_destroy(str2);
    sstring_destroy(replace_me);
    sstring_destroy(slice_me);
    return 0;
}

void print_vector( vector* input ){
    printf("vector = ");
    for( size_t i=0; i<vector_size( input ); i++){
        void* get = vector_get( input, i );
        printf("%s, ", (char*) get);
    }
    printf("\n");
}

