/**
 * nonstop_networking
 * CS 341 - Fall 2023
 */
#pragma once
#include <stddef.h>
#include <sys/types.h>

#define LOG(...)                      \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");        \
    } while (0);

typedef enum { GET, PUT, DELETE, LIST, V_UNKNOWN } verb;

ssize_t read_request_header( int fd, char* buf, size_t bytes );
ssize_t get_message_size(int socket);
ssize_t write_all_to_socket(int socket, const char *buffer, size_t count);
ssize_t read_all_from_socket(int socket, char *buffer, size_t count);
int check_read_byte( size_t total, size_t file_size );