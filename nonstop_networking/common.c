/**
 * nonstop_networking
 * CS 341 - Fall 2023
 */
#include "common.h"
#include "format.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

ssize_t read_request_header( int fd, char* buf, size_t bytes ){
    size_t total = 0;
    while( total < bytes ){
        ssize_t result = read( fd, (void*)(buf + total), 1);
        if( result == 0 || buf[strlen(buf)-1] == '\n' )
            break;
        if( result == -1 && errno == EINTR )
            continue;
        if( result == -1 )
            break;
        total += result;
    }
    return total;
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count)
{
    ssize_t curr_size  = 0;
    ssize_t total_size = 0;

    // Keep reading until we write all, or the write results in an error
    while (1) {
        curr_size = read(socket, (void*)buffer, count);
        if (curr_size == 0) {
            // nothing to read anymore
            break;
        } else if (curr_size == -1 && errno == EINTR) {
            // retry
            continue;
        } else if (curr_size == -1 && errno != EINTR) {
            // read failed
            return -1;
        }

        // Add the to the total number of bytes read 
        total_size += curr_size;
        count      -= curr_size;

        // exit loop if we wrote everything
        if (count <= 0) {
            break;
        } else {
            // move the buffer pointer forward
            buffer = buffer + curr_size;
        }
    }
    
    return total_size;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count) 
{
    ssize_t curr_size  = 0;
    ssize_t total_size = 0;

    // Keep writing until we write all, or the write results in an error
    while (1) {
        curr_size = write(socket, (void*)buffer, count);
        if (curr_size == 0) {
            // nothing to write anymore
            break;
        } else if (curr_size == -1 && errno == EINTR) {
            // retry
            continue;
        } else if (curr_size == -1 && errno != EINTR) {
            // write failed
            return -1;
        }

        // Add the to the total number of bytes written
        total_size += curr_size;
        count      -= curr_size;

        // exit loop if we wrote everything
        if (count <= 0) {
            break;
        } else {
            // move the buffer pointer forward
            buffer = buffer + curr_size;
        }
    }
    
    return total_size;
}

int check_read_byte( size_t total, size_t file_size ){
    if( total == 0 && total != file_size ){
        printf( "connection close\n" );
        print_connection_closed();
        return 1;
    }
    else if( total < file_size ){
        printf( "too little data\n" );
        print_too_little_data();
        return 1;
    }
    else if( total > file_size ){
        printf( "too much data\n" );
        print_received_too_much_data();
        return 1;
    }
    return 0;
}