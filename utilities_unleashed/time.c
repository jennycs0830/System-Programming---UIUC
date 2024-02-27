/**
 * utilities_unleashed
 * CS 341 - Fall 2023
 */
#include <aio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "format.h"
int main(int argc, char *argv[]) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    if( argc < 2 ){
        print_time_usage();
    }
    pid_t pid = fork();
    if( pid == -1 ){
        // forking error
        print_fork_failed();
    }
    else if( pid == 0 ){
        // child process
        if( execvp(argv[1], argv+1) == -1 )
            print_exec_failed();
    }
    else{
        // parent process
        int status = 0;
        waitpid( pid, &status, 0 );
        if( WIFEXITED(status) && WEXITSTATUS(status) == 0){
            clock_gettime(CLOCK_MONOTONIC, &end);
            // by chatgpt from here
            double walltime = ( end.tv_sec - start.tv_sec) + (double)( end.tv_nsec - start.tv_nsec) / 1e9;
            display_results( argv, walltime );
            // to here
        }
    }
    return 0;
}
