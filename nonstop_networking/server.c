/**
 * nonstop_networking
 * CS 341 - Fall 2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>

#include "common.h"
#include "format.h"
#include "includes/dictionary.h"

#define MAX_EVENTS 10000
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 200

// variables
static char* port;
static int sock_fd;
static char* directory;
static dictionary* connections_info;
static int epoll_fd;
static vector* fileList;

// typedef
typedef struct {
    verb command;
    char filename[256];
} Header;

typedef struct {
    int status; 
    // 0: not yet parsing header
    // 1: already parsing header but not yet execute
    Header* header;
} info;

// functions
void signal_handler( int signal );
void sigint_handler();
void server_connection();
void initial_temp_directory();
void accept_epoll();
int parsing_header( int fd, info* cur );
int execute( int fd, info* cur );
int execute_get( int fd, info* cur );
void epoll_monitor( int fd );
int execute_put( int fd, info* cur );
int execute_delete( int fd, info* cur );
int execute_list( int fd );
void remove_dirt( const char* path );
void remove_dirt_content( const char* path );
void disconnect();
int file_not_exist( char* filename );

int main( int argc, char** argv ){
    if( argc != 2 ){
        print_server_usage();
        exit(1);
    }
    
    port = argv[1];
    // signal handler
    signal_handler( SIGINT );
    signal_handler( SIGPIPE );

    server_connection();
    initial_temp_directory();

    // dictionary
    connections_info = int_to_shallow_dictionary_create();

    // list file vector
    fileList = string_vector_create();

    accept_epoll();
    disconnect();
}

void signal_handler( int signal ){
    // LOG( "signal handler\n" );
    struct sigaction sa;
    
    if( signal == SIGPIPE ){
        sa.sa_handler = SIG_IGN;
    }
    else if( signal == SIGINT ){
        sa.sa_handler = sigint_handler;
        sa.sa_flags = SA_RESTART;
    }
    
    if( sigaction( signal, &sa, NULL ) == -1 ){
        perror( "sigaction error" );
        exit(1);
    }
}

void sigint_handler(){
    // LOG( "SIGINT case\n" );
    disconnect();
    return ;
}

void server_connection(){
    // LOG( "server connection function\n" );
    // get list of socket
    struct addrinfo hints, *result;
    memset( &hints, 0, sizeof(hints) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // getaddrinfo
    // LOG( "getaddrinfo\n" );
    int status = getaddrinfo( NULL, port, &hints, &result );
    if( status != 0 ){
        fprintf( stderr, "getaddrinfo error: %s\n", gai_strerror( status ) ); 
    }

    // setsockopt
    // LOG( "setsockopt\n" );
    sock_fd = socket( AF_INET, SOCK_STREAM, 0 );
    int value = 1;
    if( setsockopt( sock_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value) ) ){
        perror( "setsockopt error" );
        exit(1);
    }

    // bind
    // LOG( "bind\n" );
    if( bind( sock_fd, result->ai_addr, result->ai_addrlen) != 0 ){
        perror( "bind error" );
        exit(1);
    }

    // listen
    // LOG( "listen\n" );
    if( listen( sock_fd, MAX_CLIENTS ) ){
        perror( "listen error" );
        exit(1);
    }

    // non-blocking setting -> if no new connection, "accept" won't blocking
    // LOG( "fcntl set nonblocking" );
    int flags = fcntl( sock_fd, F_GETFL, 0 );
    fcntl( sock_fd, F_SETFL, flags | O_NONBLOCK );

    return ;
}

void initial_temp_directory(){
    // LOG( "initial temp directory function\n" );
    char template[] = "XXXXXX";
    directory = mkdtemp( template );
    if( directory == NULL ){
        perror( "mkdtemp error" );
        exit(1);
    }
    print_temp_directory( directory );
    return ;
}

void accept_epoll(){
    // fprintf( stderr, "server fd = %d\n", sock_fd );
    epoll_fd = epoll_create1(0);
    if( epoll_fd == -1 ){
        perror( "epoll create error\n" );
        exit(1);
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = sock_fd;
    epoll_ctl( epoll_fd, EPOLL_CTL_ADD, sock_fd, &event );

    struct epoll_event events[MAX_EVENTS];
    while(1){
        int num_fd = epoll_wait( epoll_fd, events, MAX_EVENTS, -1 );
        if( num_fd == -1 ){
            if( errno != EINTR ){
                perror( "error epoll wait" );
                exit(1);
            }
        }
        
        for( int i = 0; i < num_fd; i++ ){
            int event_fd = events[i].data.fd;
            if( event_fd == sock_fd ){
                // new connection
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof( client_addr );

                int client_fd = accept( sock_fd, (struct sockaddr*) &client_addr, &client_len );
                if( client_fd == -1 ){
                    continue;
                }
                
                info* new = (info*) malloc( sizeof(info) );
                if( new == NULL ){
                    perror( "malloc error" );
                    exit(1);
                }
                new->status = 0;
                new->header = malloc( sizeof(Header) );
                dictionary_set( connections_info, &client_fd, new );

                struct epoll_event new_event;
                new_event.events = EPOLLIN | EPOLLET;
                new_event.data.fd = client_fd;
                epoll_ctl( epoll_fd, EPOLL_CTL_ADD, client_fd, &new_event );
            }
            else{
                // existing connection
                info* cur = (info*) dictionary_get( connections_info, &event_fd );

                if( cur->status == 0 ){
                    if( parsing_header( event_fd, cur ) ){
                        perror( "error parsing header" );
                        exit(1);
                    }
                }
                else if( cur->status == 1 ){
                    if( execute( event_fd, cur ) ){
                        perror( "error execute command" );
                        exit(1);
                    }
                }
            }
        }
    }

}

int parsing_header( int fd, info* cur ){
    char* buf = calloc(1, sizeof(char));
    ssize_t bytes_read = read_request_header( fd, buf, 1024);

    if( bytes_read == -1 ){
        perror( "error reading from socket" );
        exit(1);
    }
    if( bytes_read == BUFFER_SIZE ){
        // malformed request
        write_all_to_socket( fd, err_bad_request, strlen(err_bad_request) );
        epoll_monitor( fd );
    }

    if( !strncmp( buf, "GET", 3) ){
        LOG( "get request\n");
        cur->header->command = GET;
        strcpy( cur->header->filename, buf + 4 );
        cur->header->filename[ strlen( cur->header->filename ) - 1 ] = '\0';
    }
    else if( !strncmp( buf, "PUT", 3 ) ){
        LOG( "put request\n" );
        cur->header->command = PUT;
        strcpy( cur->header->filename, buf + 4 );
        cur->header->filename[ strlen( cur->header->filename ) - 1 ] = '\0';
    }
    else if( !strncmp( buf, "LIST", 4 ) && !strncmp( buf, "LIST\n", 5 ) ){
        LOG( "list request\n" );
        cur->header->command = LIST;
    }
    else if( !strncmp( buf, "DELETE", 6 ) ){
        LOG( "delete request\n" );
        cur->header->command = DELETE;
        strcpy( cur->header->filename, buf + 7 );
        cur->header->filename[ strlen( cur->header->filename ) - 1 ] = '\0';
    }
    else{
        print_invalid_response();
        epoll_monitor( fd );
        return 1;
    }
    epoll_monitor( fd );
    cur->status = 1;
    return 0;
}

int execute( int fd, info* cur ){
    int result = 0;
    if( cur->header->command == GET ){
        result = execute_get( fd, cur );
    }
    else if( cur->header->command == PUT ){
        result = execute_put( fd, cur );
    }
    else if( cur->header->command == DELETE ){
        result = execute_delete( fd, cur );
    }
    else if( cur->header->command == LIST ){
        result = execute_list( fd );
    }

    if( !result )
        return 1;
    epoll_ctl( epoll_fd, EPOLL_CTL_DEL, fd, NULL );
    shutdown( fd, SHUT_RDWR );
    close( fd );
    return 0;
}

int execute_get( int fd, info* cur ){
    char* filename = cur->header->filename;

    int len = strlen( filename ) + strlen( directory ) + 2;
    char* fullPath = calloc( 1, len * sizeof(char) );
    sprintf( fullPath, "%s/%s", directory, filename );

    FILE* remote_file = fopen( fullPath, "r" );
    if( remote_file == NULL ){
        perror( "failed to open file" );
        return 0;
    }
    write_all_to_socket( fd, "OK\n", 3 );

    struct stat file_info;
    if( stat( fullPath, &file_info ) == -1 )
        return 0;
    size_t file_size = file_info.st_size;
    write_all_to_socket( fd, (char*)&file_size, sizeof(size_t) );

    // read file content 
    size_t total = 0;
    char buf[ BUFFER_SIZE ];
    while( total < file_size ){
        size_t result = fread( buf, 1, BUFFER_SIZE, remote_file );
        size_t sent = write_all_to_socket( fd, buf, result );
        if( sent < 0 ){
            perror( "Error sending to socket" );
            write_all_to_socket( fd, err_no_such_file, strlen(err_no_such_file) );
            fclose( remote_file );
            exit(1);
        }
        total += result;
    }
    
    fclose( remote_file );
    return 1;
}                      

int execute_put( int fd, info* cur ){
    char* filename = cur->header->filename;

    int len = strlen( filename ) + strlen( directory ) + 2;
    char fullPath[ len ];
    sprintf( fullPath, "%s/%s", directory, filename );

    ssize_t result;
    char buf[ BUFFER_SIZE ] = {0};
    FILE* remote_file = fopen( fullPath, "w" );
    if( remote_file == NULL ){
        perror( "Error opening remote file" );
        return 0;
    }

    // read file size
    size_t file_size;
    read_all_from_socket( fd, (char*)&file_size, sizeof(size_t) );

    while( (result = read_all_from_socket( fd, buf, BUFFER_SIZE ) ) > 0 ){
        fwrite( buf, 1, result, remote_file );
    }
    
    write_all_to_socket( fd, "OK\n", 3 );

    if( file_not_exist( filename ) )
        vector_push_back( fileList, filename );
    
    fclose( remote_file );
    return 1;
}

int file_not_exist( char* filename ){
    for( size_t i = 0; i < vector_size( fileList ); i++ ){
        if( !strcmp( (char*)vector_get( fileList, i ), filename ) )
            return 0;
    }
    return 1;
}


int execute_delete( int fd, info* cur ){
    LOG( "execute delete" );
    char* filename = cur->header->filename;

    int len = strlen( directory ) + strlen( filename ) + 2;
    char fullPath[ len ];
    sprintf( fullPath, "%s/%s", directory, filename );

    if( remove( fullPath ) == -1 ){
        perror( "error delete file" );
        write_all_to_socket( fd, err_no_such_file, strlen(err_no_such_file) );
        exit(1);
    }

    for( size_t i = 0 ; i < vector_size( fileList ); i++ ){
        char* name = (char*) vector_get( fileList, i );
        if( !strcmp( name, filename ) ){
            vector_erase( fileList, i );
            write_all_to_socket( fd, "OK\n", 3 );
            return 1;
        }
    }

    write_all_to_socket( fd, err_no_such_file, strlen(err_no_such_file) );
    return 0;
}

int execute_list( int fd ){
    LOG( "execute list" );
    write_all_to_socket( fd, "OK\n", 3 );

    size_t size = 0;
    for( size_t i = 0; i < vector_size( fileList ); i++ ){
        char* name = (char*) vector_get( fileList, i );
        size += strlen(name) + 1;
    }
    if( size > 0 )
        size -= 1; // last line no \name
    write_all_to_socket( fd, (char*)&size, sizeof(size_t) );

    for( size_t i = 0; i < vector_size( fileList ); i++ ){
        char* name = (char*) vector_get( fileList, i );
        fprintf( stderr, "name = %s", name );
        write_all_to_socket( fd, name, strlen(name) );
        if( i != vector_size( fileList ) - 1 )
            write_all_to_socket( fd, "\n", 1 );
    }
    return 1;
}

void disconnect(){
    // close client fd
    vector* clients_fd = dictionary_values( connections_info );
    for( size_t i = 0; i < vector_size(clients_fd); i++ ){
        free( vector_get( clients_fd, i ) );
    }
    dictionary_destroy( connections_info );

    // close epoll
    close( epoll_fd );

    // remove dirt
    remove_dirt( directory );
    exit(1);
}

void remove_dirt_content( const char* path ){
    DIR* dir = opendir( path );
    if( dir == NULL ){
        perror( "error opendir" );
        return;
    }

    struct dirent* entry;
    while( ( entry = readdir(dir) ) != NULL ){
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
        }

        char file_path[1024];
        snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);

        if (unlink(file_path) == -1) {
            perror("unlink");
        }
    }
    closedir( dir );
}

void remove_dirt( const char* path ){
    remove_dirt_content( path );
    if( rmdir( path ) == -1 ){
        perror( "error rmdir" );
    }
    return ;
}

void epoll_monitor( int fd ){
    struct epoll_event event;
    event.events = EPOLLOUT; 
    event.data.fd = fd;
    epoll_ctl( epoll_fd, EPOLL_CTL_MOD, fd, &event);
}