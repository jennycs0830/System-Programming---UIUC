/**
 * deepfried_dd
 * CS 341 - Fall 2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <aio.h>
#include <signal.h>
#include <time.h>

#include "format.h"

// functions
// long valid_file( char* file, size_t block_size );
void parsing_input( int argc, char** argv );
// void fill_output_with_zero( int fd, size_t block_size, size_t fill_size );
void print_report();
void signal_handler( int signal );

// variables
static char* input_filename = NULL;
static char* output_filename = NULL;
static FILE* input_file;
static FILE* output_file;

static int copy_block = -1;
static size_t block_size = 512;
static size_t input_offset = 0;
static size_t output_offset = 0;

static struct timespec start;
static int flag = 0;
static size_t full_blocks_in;
static size_t full_blocks_out;
static size_t partial_blocks_in;
static size_t partial_blocks_out;
static size_t total_bytes_copied;

int main(int argc, char **argv) {
    signal( SIGUSR1, signal_handler );
    clock_gettime( CLOCK_REALTIME, &start );
    parsing_input( argc, argv );

    if( input_filename != NULL ){
        input_file = fopen( input_filename, "r" );
        if( input_file == NULL ){
        print_invalid_input( input_filename );
        exit(1);
    }
    }
    else{
        // fprintf( stderr, "stdin case\n" );
        input_file = stdin;
    }

    if( output_filename != NULL ){
        output_file = fopen( output_filename, "w+" );
        if( output_file == NULL ){
            print_invalid_output( output_filename );
            fclose( input_file );
            exit(1);
        }
    }
    else{
        // fprintf( stderr, "stdout case\n" );
        output_file = stdout;
    }

    off_t input_offset_byte = input_offset * block_size;
    off_t output_offset_byte = output_offset * block_size;

    if( input_offset ){
        if( fseek( input_file, input_offset_byte, SEEK_SET ) ){
            fclose( input_file );
            fclose( output_file );
            exit(1);
        }
    }

    if( output_offset ){
        if( fseek( output_file, output_offset_byte, SEEK_SET ) ){
            fclose( input_file );
            fclose( output_file );
            exit(1);
        }
    }

    char* buf = malloc( block_size );
    if( buf == NULL ){
        // fprintf( stderr, "error allocating memory\n" );
        fclose( input_file );
        fclose( output_file );
        exit(1);
    }

    ssize_t byte_read, byte_written;
    int blocks_copied = 0;
    
    while( ( byte_read = fread( buf, 1, block_size, input_file ) ) > 0 ){
        if( flag )
            print_report();
        byte_written = fwrite( buf, 1, byte_read, output_file );
        // fprintf( stderr, "byte_read = %zu\n", byte_read );
        // fprintf( stderr, "byte_written = %zu\n\n", byte_written );
        if( byte_read != byte_written ){
            // fprintf( stderr, "error writing to output file\n" );
            exit(1);
            break;
        }

        blocks_copied ++;
        total_bytes_copied += byte_written;

        if( copy_block == blocks_copied ){
            full_blocks_in ++;
            full_blocks_out ++;
            break;
        }
        else if( (int) block_size != byte_read) {
            partial_blocks_in ++;
            partial_blocks_out ++;
            break;
        }
        full_blocks_in ++;
        full_blocks_out ++;
    }
    if( byte_read == -1 ){
        // fprintf( stderr, "error reading input file\n" );
        exit(1);
    }

    print_report();

    free( buf );
    fclose( input_file );
    fclose( output_file );

    return 0;
}

void parsing_input( int argc, char** argv ){
    int opt;
    while( (opt = getopt( argc, argv, "i:o:b:c:p:k:" )) != -1 ){
        switch( opt ){
            case 'i':
                input_filename = optarg;
                break;
            case 'o':
                output_filename = optarg;
                break;
            case 'b':
                block_size = (size_t) atoi ( optarg );
                break;
            case 'c':
                copy_block = (size_t) atoi ( optarg );
                break;
            case 'p':
                input_offset = (size_t) atoi ( optarg );
                break;
            case 'k':
                output_offset = (size_t) atoi ( optarg );
                break;
            default:
                // fprintf( stderr, "invalid arguments\n" );
                exit(1);
        }
    }
}

void signal_handler( int signal ){
    if( SIGUSR1 == signal )
        flag = 1;
}

void print_report(){
    struct timespec end;
    clock_gettime( CLOCK_REALTIME, &end );
    double diff_sec = difftime( end.tv_sec, start.tv_sec );
    double diff_nsec = ((double)( end.tv_nsec - start.tv_nsec )) / 1000000000;
    print_status_report( full_blocks_in, partial_blocks_in, full_blocks_out, partial_blocks_out, total_bytes_copied, diff_sec + diff_nsec );
    flag = 0;
    return ;
}