/**
 * extreme_edge_cases
 * CS 341 - Fall 2023
 */
#include "camelCaser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

char **camel_caser(const char *input_str) {
    // TODO: Implement me!
    if( input_str==NULL )
        return NULL;
    char** output_str = NULL;
    int word_cnt = 0;
    int newSent = 1; //check if character is the beginning of sentence
    int newWord = 1; //check if character is the beginning of word
    char *word = NULL; //record the word
    int wordSize = 0;
    while( *input_str!='\0' ){
        if( ispunct( *input_str ) ){
            word_cnt ++;
            output_str = (char**) realloc (output_str, word_cnt*sizeof(char*));
            wordSize ++;
            word = (char*) realloc (word, wordSize*sizeof(char));
            word[wordSize-1] = '\0';
            output_str[word_cnt-1] = word;
            word = NULL;
            wordSize = 0;
            newSent = 1;
            newWord = 1;
        }
        else{
            if( isspace(*input_str) ){
                newWord = 1;
            }
            else{
                wordSize ++;
                word = (char*) realloc (word, wordSize * sizeof(char));
                if( isalpha(*input_str) ){
                    if( newSent )
                        word[wordSize-1] = tolower(*input_str);
                    else{
                        if( newWord ){
                            word[wordSize-1] = toupper(*input_str);
                        }
                        else{
                            word[wordSize-1] = tolower(*input_str);
                        }
                    }
                    newWord = 0;
                    newSent = 0;
                }
                else{
                    word[wordSize-1] = *input_str;
                }
            }
        }
        input_str ++;
    }
    word_cnt ++;
    output_str = (char**) realloc (output_str, word_cnt*sizeof(char*));
    output_str[word_cnt-1] = NULL;
    return output_str;
}

void destroy(char **result) {
    // TODO: Implement me!
    if(result == NULL)
        return;
    int i;
    for(i=0;result[i]!=NULL;i++){
        free(result[i]);
    }
    free(result);
    return;
}