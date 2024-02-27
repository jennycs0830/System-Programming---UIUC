/**
 * mapreduce
 * CS 341 - Fall 2023
 */
#include "utils.h"
#include "core/libds.h"
#include "core/mapper.h"
#include "core/reducer.h"
#include "core/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    // variables
    if( argc != 6 ){
        print_usage();
        return 1;
    }
    char* inputFile = argv[1];
    char* outputFile = argv[2];
    char* mapperExec = argv[3];
    char* reducerExec = argv[4];
    char* mapper_count_str = argv[5];
    int mapper_count = atoi(mapper_count_str);

    // Create an input pipe for each mapper.
    int mapperPipe[mapper_count][2];
    for( int i = 0; i < mapper_count; i++ ){
        if( pipe( mapperPipe[i] ) == -1 ){
            perror( "failed piping mapperPipe" );
            return 1;
        }
    }

    // Create one input pipe for the reducer.
    int reducerPipe[2];
    if( pipe( reducerPipe ) == -1 ){
        perror( "failed piping reducesPipe" );
        return 1;
    }

    // Open the output file.
    int output_fd = open( outputFile, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR );
    if( output_fd == -1 ){
        perror( "failed open output file" );
        return 1;
    }

    // Start a splitter process for each mapper.
    pid_t splitter_pids[ mapper_count ];
    const char* path = "./splitter";
    for( int i = 0; i < mapper_count; i++ ){
        pid_t pid = fork();
        splitter_pids[i] = pid;
        if( pid < 0 ){
            perror( "Failed to fork" );
            return 1;
        }
        else if( pid == 0 ){
            // child process 
            close( mapperPipe[i][0] ); // close mapper pipe read end (unused)
            if( dup2( mapperPipe[i][1], STDOUT_FILENO ) == -1 ){
                print_nonzero_exit_status( "splitter", 1 ); 
                exit(1);
            }// mapper pipe write end to stdout

            char index[10];
            sprintf( index, "%d", i );
            // fprintf( stderr,  "index = %s\n", index );
            
            execlp( path, path, inputFile, mapper_count_str, index, NULL ); // execute mapping
            perror( "spliiter execution failed" );
            return 1;
        }
    }

    // Start all the mapper processes.
    pid_t mapper_pid[ mapper_count ];
    for( int i = 0; i < mapper_count; i++ ){
        pid_t pid = fork();
        mapper_pid[i] = pid;
        close( mapperPipe[i][1] );

        if( pid < 0 ){
            perror( "Failed to fork" );
            return 1;
        }
        else if( pid == 0 ){
            if( dup2( mapperPipe[i][0], STDIN_FILENO ) == -1 ){
                exit(1);
            } // mapper pipe read end to stdin
            if( dup2( reducerPipe[1], STDOUT_FILENO ) == -1 ){
                exit(1);
            } // reducer pipe write end to stdout
            close( reducerPipe[0] );

            execlp( mapperExec, mapperExec, NULL );
            perror( "mapper execution failed" );
            print_nonzero_exit_status( mapperExec, 1 );
            return 1;
        }
    }

    // Start the reducer process.
    close( reducerPipe[1] );
    pid_t pid = fork();

    if( pid < 0 ){
        perror( "Failed to fork" );
        return 1;
    }
    else if( pid == 0 ){
        if( dup2( reducerPipe[0], STDIN_FILENO ) == -1 ){
            exit(1);
        }
        if( dup2( output_fd, STDOUT_FILENO ) == -1 ){
            exit(1);
        }

        execlp( reducerExec, reducerExec, NULL );
        perror( "reducer execution fialed" );
        return 1;
    }

    // Wait for the reducer to finish.
    for( int i = 0; i < mapper_count; i++ ){
        int status;
        waitpid( splitter_pids[i], &status, 0 );
    } 
    for( int i = 0; i < mapper_count; i++ ){
        int status;
        waitpid( mapper_pid[i], &status, 0 );
    }

    // Print nonzero subprocess exit codes.
    int status;
    waitpid( pid, &status, 0 );
    if( status != 0 )
        print_nonzero_exit_status( reducerExec, status );

    // Count the number of lines in the output file.
    print_num_lines( outputFile );

    close( reducerPipe[0] );
    close( output_fd ); 

    return 0;
}
