/**
 * mad_mad_access_patterns
 * CS 341 - Fall 2023
 */
#include "tree.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
*/

// functions
int check_valid_file( FILE* file );
void solve( char* word, FILE* file );

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

    if( !check_valid_file( file ) ){
        formatFail( filename );
        exit(2);
    }

    for( int i = 2; i < argc; i++ ){
        solve( argv[i], file );
    }

    fclose( file );
    return 0;
}

int check_valid_file( FILE* file ){
    char* root = malloc( 4 * sizeof(char) );
    fread( root, 4, 1, file );
    if( !strcmp( root, "BTRE" ) )
        return 1;
    else
        return 0;
    return 1;
}

void solve( char* word, FILE* file ){
    long offset = BINTREE_ROOT_NODE_OFFSET;
    while( offset != 0 ){
        BinaryTreeNode node;
        if( fseek( file, offset, SEEK_SET ) != 0 ){
            return;
        }
        
        if( fread( &node, sizeof( BinaryTreeNode ), 1, file ) != 1 ){
            return;
        }

        fseek( file, sizeof(BinaryTreeNode) + offset, SEEK_SET );
        
        char node_word[16];
        fread( node_word, sizeof(BinaryTreeNode), 1, file );

        // strcmp word
        if( strcmp( word, node_word ) == 0 ){
            printFound( word, node.count, node.price );
            return ;
        }
        else if( strcmp( word, node_word ) < 0 ){
            offset = node.left_child;
        }
        else{
            offset = node.right_child;
        }
    }
    printNotFound( word );
    return ;
}
