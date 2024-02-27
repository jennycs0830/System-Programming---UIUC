/**
 * mad_mad_access_patterns
 * CS 341 - Fall 2023
 */
#include "tree.h"
#include "utils.h"

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/
#include "tree.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

// functions
void solve( char* word, char* map );
int check_valid_file( char* map );

int main(int argc, char **argv) {
    if( argc < 3 ){
        printArgumentUsage();
        exit(1);
    }
    char* filename = argv[1];
    FILE* file = fopen( filename, "r" );
    if( file == NULL ){
        openFail( filename );
        exit(2);
    }

    struct stat info;
    if( fstat( fileno(file), &info ) == -1 ){
        formatFail( filename );
        exit(2);
    }

    char* map = mmap( NULL, info.st_size, PROT_READ, MAP_PRIVATE, fileno(file), 0 );
    if( map == MAP_FAILED ){
        mmapFail(filename );
        exit(3);
    }

    if( !check_valid_file( map ) ){
        formatFail( filename );
        exit(2);
    }

    for( int i = 2; i < argc; i++ ){
        solve( argv[i], map );
    }

    if( munmap( map, info.st_size) == -1 ){
      perror( "Error un-mapping the file" );
    }
    fclose(file);
    return 0;
}

int check_valid_file( char* map ){
    if( !strncmp( map, "BTRE", 4 ) )
        return 1;
    else
        return 0;
}

void solve( char* word, char* map ){
    long offset = BINTREE_ROOT_NODE_OFFSET;
    while( offset != 0 ){
        BinaryTreeNode* node = (BinaryTreeNode*)(map + offset);

        char* node_word = (char*)(node + 1);

        // strcmp word
        if( strcmp( word, node_word ) == 0 ){
            printFound( word, node->count, node->price );
            return ;
        }
        else if( strcmp( word, node_word ) < 0 ){
            offset = node->left_child;
        }
        else{
            offset = node->right_child;
        }
    }
    printNotFound( word );
    return ;
}