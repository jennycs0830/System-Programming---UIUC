/**
 * utilities_unleashed
 * CS 341 - Fall 2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>
#include "format.h"

int parse_input( int argc, char** argv, char*** cmd, char*** key, char*** value );
void destroy ( char** arr );

int main(int argc, char *argv[]) {
    // check argc, argv
    // printf("argc = %d\n", argc);
    // for( int i=0; i<argc; i++){
    //     printf("%s\n", argv[i]);
    // }
    // printf("\n");
    // shortest: ./env -- cmd 
    if( argc < 3 )
        print_time_usage(); 
    
    char** cmd = NULL;
    char** key = NULL;
    char** value = NULL;
    int index = parse_input( argc, argv, &cmd, &key, &value );
        
    pid_t pid = fork();
    if( pid == -1 ){
        // forking error
        print_fork_failed();
    }
    else if( pid == 0 ){
        // child process
        // printf( "start running child process!!!\n");
        
        // setting environment variables
        for( int i = 0; i < index - 1; i++ ){
            if( value[i][0] == '%' ){
                char* env_value = getenv( value[i] + 1 );
                if( env_value == NULL )
                    value[i] = "";
                else
                    value[i] = strdup( env_value );
            }
            if( setenv( key[i], value[i], 1 ) == -1 )
                print_environment_change_failed();
        }

        // execute command
        // printf( "cmd[0] = %s\n", cmd[0] );
        // while( *cmd != NULL ){
        //     printf( "cmd = %s\n", *cmd);
        //     cmd ++;
        // }
        // printf( "child process start execute command\n");
        execvp( cmd[0], cmd );
        print_exec_failed();
        destroy( cmd );
        destroy( key );
        destroy( value );
        exit( 1 );
    }
    else{
        // parent process
        // printf( "start running parent process!!!\n");
        int status;
        waitpid( pid, &status, 0 );
        if( !WIFEXITED( status ) || WEXITSTATUS( status ) ){
            // printf("parent status failed\n");
            destroy( cmd );
            destroy( key );
            destroy( value );
            print_exec_failed();
        }
    }
    destroy( cmd );
    destroy( key );
    destroy( value );
    return 0;
}

int parse_input( int argc, char** argv, char*** cmd, char*** key, char*** value ){
    // parse environment variables and command
    // printf( "parse env and cmd !!!\n" );
    int index; // record the index of "--"
    int i;
    for( i = 1; i < argc; i++ ){
        // printf( "argv[i] = %s\n", argv[i] );
        if( !strcmp( argv[i], "--" ) ){
            index = i;
            break;
        }
    }
    if( i == argc-1 ){
        // cannot find -- in arguments
        assert( 0 );
    }
    // printf( "index = %d\n\n", index );

    // parse environment variables
    // printf( "parse env key and value !!!\n" );
    *key = (char**) malloc ( index * sizeof(char*) );
    if( *key == NULL ){
        // printf("failed malloc to key\n");
        exit(1);
    }

    *value = (char**) malloc ( index * sizeof(char*) );
    if( *value == NULL ){
        // printf("failed malloc to value\n");
        exit(1);
    }

    for( int i = 0; i < index - 1; i++ ){
        // printf( "argv[] = %s\n", argv[i+1] );
        char* equal = strchr( argv[i+1], '=' );
        // printf( "equal = %s\n", equal );
        if( equal != NULL ){
            (*key)[i] = (char*) malloc ( (equal - argv[i+1] + 1) * sizeof(char) );
            if( (*key)[i] == NULL ){
                // printf( "failed malloc to key[%d]\n", i );
                exit(1);
            }
            strncpy( (*key)[i], argv[i+1], equal - argv[i+1] );
            (*key)[i][equal - argv[i+1]] = '\0';
            // printf( "*key[i] = %s\n", (*key)[i] );

            (*value)[i] = (char*) malloc ( (strlen(argv[i+1])+1) * sizeof(char) );
            if( (*value)[i] == NULL ){
                // printf( "failed malloc to value[%d]\n", i );
                exit(1);
            }
            strcpy( (*value)[i], equal + 1 );
            (*value)[i][strlen(argv[i+1])]='\0';
        }
        (*key)[index-1] = NULL;
        (*value)[index-1] = NULL;
    }

    // parse command
    // printf( "parse cmd !!!\n");
    *cmd = (char**) malloc ( ((argc - index - 1)+1) * sizeof(char*) );
    int j = 0;
    for( int i = index+1; i < argc ; i++, j++ ){
       (*cmd)[j] = malloc( (strlen(argv[i])+1) * sizeof(char) );
       strcpy((*cmd)[j], argv[i]);
    }
    (*cmd)[j] = NULL;
    return index;
}

void destroy ( char** arr ){
    if( arr == NULL )
        return ;
    for (int i = 0; arr[i] != NULL; i++) {
        // printf( "i = %d\n", i);
        // printf( "arr[i] = %s\n", arr[i]);
        free(arr[i]);
        // printf("sucessful free!!!\n");
        arr[i] = NULL;
    }
    free(arr);
    arr = NULL; 
}