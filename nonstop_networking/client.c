/**
 * nonstop_networking
 * CS 341 - Fall 2023
 */
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>

#include "common.h"

#define BUFFER_SIZE 1024
#define ERR_MSG_SIZE 256

//functions
char **parse_args(int argc, char **argv);
verb check_args(char **args);
void get_request( int fd, char* local );
void put_request( int fd, char* local );
void list_request( int fd );
int check_response( int fd );


int main(int argc, char **argv) {
    // Good luck!
    char** args = parse_args( argc, argv );
    verb method = check_args( args );

    // parsing args
    char* host = args[0];
    char* port = args[1];
    char* remote = NULL;
    if( argc > 3 )
        remote = args[3];
    char* local = NULL;
    if( argc > 4 )
        local = args[4];

    // getaddrinfo
    struct addrinfo hints, *result;
    memset( &hints, 0, sizeof(hints) );

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo( host, port, &hints, &result );
    if( status != 0 ){
        fprintf( stderr, "getaddrinfo error: %s\n", gai_strerror( status ) );
    }
    
    // connect to socket
    struct addrinfo* p;
    int sockfd;
    for( p = result; p != NULL; p = p->ai_next ){
        sockfd = socket( p->ai_family, p->ai_socktype, p->ai_protocol );
        if( sockfd == -1 )
            continue;
        if( connect( sockfd, p->ai_addr, p->ai_addrlen ) == -1 ){
            close( sockfd );
            continue;
        }
        break;
    }
    if( p == NULL ){
        fprintf( stderr, "Failed to connect\n" );
        exit(1);
    }


    char* buf = NULL;
    if( method == GET ){
        buf = malloc( strlen("GET") + 1 + strlen(remote) + 2 );
        if( buf == NULL ){
            perror( "Error allocating memory" );
            return -1;
        }
        sprintf( buf, "%s %s\n", "GET", remote );
        write_all_to_socket( sockfd, buf, (ssize_t) strlen( buf ) );
        if( check_response( sockfd ) )
            get_request( sockfd, local );
    }
    else if( method == PUT ){
        buf = malloc( strlen("PUT") + 1 + strlen(remote) + 2 );
        if( buf == NULL ){
            perror( "Error allocating memory" );
            return -1;
        }
        sprintf( buf, "%s %s\n", "PUT", remote );
        write_all_to_socket( sockfd, buf, (ssize_t) strlen( buf ) );
        put_request( sockfd, local );
        if( check_response( sockfd ) )
            print_success();
    }
    else if( method == LIST ){
        buf = malloc( strlen("LIST") + 2 );
        if( buf == NULL ){
            perror( "Error allocating memory" );
            return -1;
        }
        sprintf( buf, "%s\n", "LIST" );
        write_all_to_socket( sockfd, buf, (ssize_t) strlen( buf ) );
        if( check_response( sockfd ) )
            list_request( sockfd );
    }
    else if( method == DELETE ){
        buf = malloc( strlen("DELETE") + 1 + strlen(remote) + 2 );
        if( buf == NULL ){
            perror( "Error allocating memory" );
            return -1;
        }
        sprintf( buf, "%s %s\n", "DELETE", remote );
        write_all_to_socket( sockfd, buf, (ssize_t) strlen( buf ) );
        if( check_response( sockfd ) )
            print_success();
    }

    free( buf );
    free( args );
    close( sockfd );
    freeaddrinfo( result );
    return 0;
}

void get_request( int fd, char* local ){
    ssize_t result;
    char buf[ BUFFER_SIZE ] = {0};
    FILE* local_file = fopen( local, "w" );
    if( local_file == NULL ){
        perror( "Error opening local file" );
        return ; 
    }

    // read file size
    size_t file_size;
    read_all_from_socket( fd, (char*)&file_size, sizeof(size_t) );
    // printf( "remote file size = %zu\n", file_size );

    size_t total = 0;
    while( (result = read_all_from_socket( fd, buf, BUFFER_SIZE ) ) > 0 ){
        fwrite( buf, 1, result, local_file );
        total += result;
    }
    // printf( "total write bytes = %zu\n", total  );
    
    if( check_read_byte( total, file_size ) )
        exit(1);
    fclose( local_file );
    return ;
}

void put_request( int fd, char* local ){
    FILE* local_file = fopen( local, "r" );

    // write file size 
    struct stat file_info;
    if( stat( local, &file_info ) == -1 )
        return;
    size_t file_size = file_info.st_size;
    write_all_to_socket( fd, (char*)&file_size, sizeof(size_t) );
    
    size_t result;
    char buf[ BUFFER_SIZE ];
    while( (result = fread( buf, 1, BUFFER_SIZE, local_file ) ) > 0 ){
        size_t sent = write_all_to_socket( fd, buf, result );
        if( sent < 0 ){
            perror( "Error sending to socket" );
            fclose( local_file );
            break;
        }
        // printf( "send %zu bytes to remote\n", sent );
    }

    // shutdown write half
    if( shutdown( fd, SHUT_WR ) != 0 ){
        perror( "shutdown error" );
        return ;
    }
    fclose( local_file );
    return ;
}

void list_request( int fd ){
    size_t file_size;
    read_all_from_socket( fd, (char*)&file_size, sizeof(size_t) );
    // printf( "remote file size = %zu\n", file_size );

    ssize_t result;
    size_t total = 0;
    char buf[ BUFFER_SIZE ];
    while( (result = read_all_from_socket( fd, buf, BUFFER_SIZE ) ) > 0 ){
        printf( "%s", buf );
        total += result;
    }
    buf[ total ] = '\0';
    if( check_read_byte( total, file_size ) )
        exit(1);
    return ;
}

int check_response( int fd ){
    // OK
    char* buf = calloc( 1, strlen("OK\n") + 1 );
    size_t bytes = read_all_from_socket( fd, buf, strlen( "OK\n" ) );
    if( !strcmp( buf, "OK\n" ) ){
        printf( "%s", buf );
        free( buf );
        return 1;
    }
    else{
        buf = realloc( buf, strlen( "ERROR\n" ) + 1 );
        read_all_from_socket( fd, buf + bytes, strlen( "ERROR\n" ) - bytes );
        if( !strcmp( buf, "ERROR\n" ) ){
            printf( "%s", buf );
            char err_msg[20] = {0};
            if( ! read_all_from_socket( fd, err_msg, 20 ) ){
                print_connection_closed();
            }
            // printf( "read %zu bytes\n", bytes );
            print_error_message( err_msg );
        }
        free( buf );
        return 0;
    }
    print_invalid_response();
    return 0;
}


/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL) {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp) {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3) {
        args[3] = argv[3];
    }
    if (argc > 4) {
        args[4] = argv[4];
    }

    return args;
}

/**
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args) {
    if (args == NULL) {
        print_client_usage();
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "LIST") == 0) {
        return LIST;
    }

    if (strcmp(command, "GET") == 0) {
        if (args[3] != NULL && args[4] != NULL) {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0) {
        if (args[3] != NULL) {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0) {
        if (args[3] == NULL || args[4] == NULL) {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}