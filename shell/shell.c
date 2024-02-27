/**
 * shell
 * CS 341 - Fall 2023
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include "sstring.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

typedef struct process {
    char *command;
    char* status;
    pid_t pid;
} process;

typedef struct redirection_info {
    char* filename;
    char* type;  // '>', '<', or '>>'
    char* cmd;
} redirection_info;

// functions
void signal_handler( int signal );
void handle_args( int argc, char** argv );
char* args_preprocessor( char** argv );
void add_process( char* cmd, char* status, pid_t pid );
process* create_process( char* cmd, char* status, pid_t pid );
void script_file( char* filename );
void command( char* cmd );
int classify( char* cmd );
vector* cmd_preprocessor( char* cmd );
char* logical( char* cmd );
int external( char* cmd );
void shell_exit( int status );
void write_history( char* path );
size_t set_process_status( pid_t pid, char* status );
void free_args(char **args);
process_info* create_info( char* cmd, pid_t pid );
redirection_info* redirect( char* cmd );
char* redirector( char* cmd );
void restore_redirection(FILE *file);
FILE* setup_redirection(redirection_info *redir);

// variables
static vector* cmd_his = NULL; // record history command
static vector* processes = NULL; // record exist command
static char* cmd_source = "stdin";
static FILE* script = NULL;
static FILE* history = NULL;
static char* script_filename = NULL;
static vector* script_cmd = NULL;
static char* h_filename = NULL;
static int bg = 0;
static int original_stdout = -1;
static int original_stdin = -1;

int shell(int argc, char *argv[]) {
    // TODO: This is the entry point for your shell.

    // handle signal: SIGINT
    signal( SIGINT, signal_handler );
    signal( SIGCHLD, signal_handler );

    // handle optional arguments
    script_cmd = string_vector_create();
    if( argc > 1 )
        handle_args( argc, argv );

    // Initialize variables
    cmd_his = string_vector_create();
    processes = shallow_vector_create();

    // add process ( this is the first process, and the other process will be added when executing external command )
    char* args = args_preprocessor( argv );
    add_process( args, "RUNNING", getpid() );
    free( args );
    print_prompt( get_current_dir_name(), getpid() );

    if( h_filename != NULL ){
        history = fopen( h_filename, "r" );
        if( history == NULL ){
            print_history_file_error();
        }
        else{
            char* cmd = (char*) malloc ( 256 * sizeof(char) );
            if( cmd == NULL )
                perror( "memory allocation error" );
            while( fgets( cmd, 256, history ) != NULL ){
                size_t len = strlen( cmd );
                if( len > 0 && cmd[ len - 1 ] == '\n' )
                    cmd[ len - 1 ] = '\0';
                vector_push_back( cmd_his, cmd );
            }
            vector_push_back( script_cmd, (char*)"exit" );
            free( cmd );
            cmd = NULL;
        }
    }
    
    // script file case
    char* cmd = (char*) malloc ( 256 * sizeof(char) );
    if( cmd == NULL )
        perror( "memory allocation error" );
    if( !strcmp( cmd_source, "script" ) ){
        // printf( "script file case\n" );
        for( size_t i = 0; i < vector_size( script_cmd ); i++ ){
            // printf( "cmd = %s\n", (char*) vector_get( script_cmd, i ) );
            command( (char*) vector_get( script_cmd, i ) );
            print_prompt( get_current_dir_name(), getpid() );
            fflush( stdout );
        }
        // free script command
        vector_destroy( script_cmd );
        free( cmd );
        cmd = NULL;
        return 0;
    }
    // stdin case
    else if( !strcmp( cmd_source, "stdin" ) ){
        // printf("stdin:\n");
        size_t len = 0;
        while( getline( &cmd, &len, stdin ) != -1 ){
            while (len > 0 && isspace((unsigned char) cmd[len - 1])) {
                cmd[len - 1] = '\0';
                len--;
            }
            command( cmd );
            print_prompt( get_current_dir_name(), getpid() );
            fflush( stdout );
            // printf( "next round!\n" );
        }
        // printf( "get EOF (ctrl-D)\n" );
        shell_exit(1);
        free( cmd );
        cmd = NULL; 
    }
    // here
    // shell_exit(0);
    return 0;
}

// SIGCHID要處理嗎（當child process結束要讓parent process更新他的status
void signal_handler( int signal ){
    // catch SIGINT and do nothing
    if( signal == SIGINT ){}
    else if( signal == SIGCHLD ){
        pid_t pid = 0;
        int status = 0;
        if( (pid = waitpid( -1, &status, WNOHANG) ) != -1 ){
            set_process_status(pid, "KILLED");
        }
    }
    return ;
}

void handle_args( int argc, char** argv ){
    int opt;
    cmd_source = "stdin";
    while( ( opt = getopt( argc, argv, "h:f:" ) ) != -1 ){
        // printf( "opt = %d\n", opt);
        if( opt == 'h' ){
            // printf( "set h_filename = %s\n", get_full_path( optarg ) );
            // history file
            h_filename = get_full_path( optarg );
        }
        else if( opt == 'f' ){
            // script file
            cmd_source = "script";
            script_filename = optarg;
            script_file( script_filename );
        }
        else{
            // invalid command line
            print_usage();
        }
    }
    return ;
}

char* args_preprocessor( char** argv ){
    size_t len = 0;
    char** ptr = argv;
    while( *ptr != NULL ){
        len += strlen( *ptr ) + 1;
        ptr ++;
    }
    char* cmd;
    if( len == 0 ){
        cmd = (char*) malloc ( sizeof(char) );
        if( cmd == NULL )
            perror( "memory allocation error" );
        cmd[0] = '\0';
    }
    else{
        cmd = (char*) malloc ( len * sizeof(char) );
        if( cmd == NULL )
            perror( "memory allocation error" );
        cmd[0] = '\0'; // for strcat
        ptr = argv;
        while( *ptr != NULL ){
            strcat( cmd, *ptr );
            if( *( ptr + 1 ) != NULL )
                strcat( cmd, " " );
            ptr ++;
        }
    }
    return cmd;
}

void add_process( char* cmd, char* status, pid_t pid ){
    for( size_t i = 0; i < vector_size( processes ); i++ ){
        process* cur = vector_get( processes, i );
        if( cur->pid == pid ){
            cur->command = strdup(cmd);
            cur->status = strdup(status);
            return ;
        }
    }
    process* new = create_process( cmd, status, pid );
    
    vector_push_back( processes, new );
    return ;
}

process* create_process( char* cmd, char* status, pid_t pid ){
    process* new = (process*) malloc ( sizeof(process) );
    if( new == NULL )
        perror( "memory allocaiton error" );
    new->command = strdup( cmd );
    new->status = status;
    new->pid = pid;
    return new;
}

void script_file( char* filename ){
    // printf("script file\n");
    script = fopen( filename, "r" );
    if( script == NULL ){
        print_script_file_error();
        return ;
    }
    else{
        // printf( "script file open\n" );
        char* cmd = (char*) malloc ( 256 * sizeof(char) );
        if( cmd == NULL )
            perror( "memory allocation error" );
        while( fgets( cmd, 256, script ) != NULL ){
            size_t len = strlen( cmd );
            while (len > 0 && isspace((unsigned char) cmd[len - 1])) {
                // printf("set len-1 to 0");
                cmd[len] = '\0';
                len--;
            }
            vector_push_back( script_cmd, cmd );
        }
        vector_push_back( script_cmd, (char*)"exit" );
        // printf( "check script cmd content\n" );
        // for( size_t i = 0; i < vector_size( script_cmd) ; i++ ){
        //     printf( "cmd = %s\n", (char*) vector_get( script_cmd, i ) );
        // }
        free( cmd );
        cmd = NULL;
        return ;
    }
}

void command( char* cmd ){
    // printf("cmd = %s, strlen = %lu\n\n", cmd, strlen(cmd));
    bg = 0;
    if( cmd[ strlen(cmd) - 2 ] == '&' ){
        cmd[ strlen(cmd) - 2 ] = '\0';
        bg = 1;
    }
    // printf("cmd = %s, strlen = %lu\n\n", cmd, strlen(cmd));
    if( !strcmp( cmd, "\n" ) )
        return ;
    char* operator = logical( cmd );
    if( operator ){
        char* cmd1 = NULL;
        char* cmd2 = NULL;
        char* pos = strstr(cmd, operator);
        if (pos != NULL) {
            *pos = '\0'; 
            cmd1 = cmd;
            cmd2 = pos + strlen(operator);
        }

        int result1;
        int class1 = classify( cmd1 );
        switch( class1 ){
            // printf( "class1 = %d\n", class1 );
            result1 = 1;
            case -1:
                print_invalid_command( cmd1 );
                result1 = 0;
                // here
                // result1 = 1;
                break;
            case 0:
                result1 = 0;
                // result1 = 1;
                break;
            case 6:
                fflush( stdout );
                result1 = external( cmd1 );
                break;
            default:
                result1 = 1;
                // result1 = 0;
                break;
        }
        // printf( "result1 = %d\n", result1 );
        if( !strcmp( operator, "&&" ) ){
            if( result1 ){
                switch( classify( cmd2 ) ){
                    case -1:
                        print_invalid_command( cmd2 );
                        break;
                    case 6:
                        fflush( stdout );
                        external( cmd2 );
                        break;
                    default:
                        return ;
                }
                return ;
            }
            return ;
        }
        if( !strcmp( operator, "||" ) ){
            if( result1 )
                return ;
        }
        switch( classify( cmd2 ) ){ 
            case -1:
                print_invalid_command( cmd2 );
                break;
            case 6:
                fflush( stdout );
                result1 = external( cmd2 );
                break;
            default:
                return;
            return ;
        }
        return ;
    }
    else{
        int result = classify( cmd );
        switch( result ){
            case -1:
                print_invalid_command( cmd );
                break;
            case 6:
                fflush( stdout );
                external( cmd );
                // printf( "finish external cmd\n" );
                break;
            default:
                break;
        }
        // printf( "return from command function\n");
        return ;
    }
}

int classify( char* cmd ){
    /*
    -1: invalid
    0: valid but error
    1: cd
    2: !history
    3: #<n>
    4: !<prefix>
    5: exit
    6: external
    7: ps
    8: kill
    9: stop
    10: cont
    */
    // printf( "classify cmd = %c\n", *cmd );
    redirection_info* re_info = redirect( cmd );
    if( re_info->type != NULL ){
        cmd = re_info->cmd;
    }
    vector* cmd_split = string_vector_create();
    cmd_split = cmd_preprocessor( cmd );

    // built-in
    if( !strcmp( (char*) vector_get ( cmd_split, 0 ), "cd" ) ){
        if( vector_size( cmd_split ) != 2 ){
            vector_destroy( cmd_split );
            cmd_split = NULL;
            return -1;
        }
        else{
            if( chdir( vector_get( cmd_split, 1 ) ) == -1 ){
                print_no_directory( cmd );
                vector_destroy( cmd_split );
                cmd_split = NULL;
                return 0;
            }
        }
        vector_push_back( cmd_his, cmd );
        vector_destroy( cmd_split );
        cmd_split = NULL;
        return 1;
    }
    else if( !strcmp( (char*) vector_get ( cmd_split, 0 ), "!history" ) ){
        if( vector_size( cmd_split ) != 1 ){
            vector_destroy( cmd_split );
            cmd_split = NULL;
            return -1;
        }
        for( size_t i = 0; i < vector_size( cmd_his ); i++ )
            print_history_line( i, (char*) vector_get ( cmd_his, i ) );
        vector_destroy( cmd_split );
        cmd_split = NULL;
        return 2;
    }
    else if( *( (char*) vector_get ( cmd_split, 0 ) ) == '#' ){
        size_t len = strlen( (char*) vector_get ( cmd_split, 0 ) );
        if( vector_size( cmd_split ) != 1 || len <= 1 ){
            vector_destroy( cmd_split );
            cmd_split = NULL;
            return -1;
        }
        int num = 0;
        for( size_t i = 1; i < len; i++ ){
            if( isdigit( *( (char*) vector_get ( cmd_split, 0 ) + i) ) == 0 ){
                vector_destroy( cmd_split );
                cmd_split = NULL;
                return -1;
            }
            else
                num = num * 10 + ( *( (char*) vector_get ( cmd_split, 0 ) + i ) - '0' );
        }
        if( num < (int) vector_size ( cmd_his ) ){
            char* get_cmd = (char*) vector_get ( cmd_his, num );
            print_command( get_cmd );
            command( get_cmd );
            vector_destroy( cmd_split );
            cmd_split = NULL;
            return 3;
        }   
        else{
            print_invalid_index();
            vector_destroy( cmd_split );
            cmd_split = NULL;
            return 0;
        }
        return 3;
    }
    else if( *( (char*) vector_get ( cmd_split, 0 ) ) == '!' ){
        if( vector_size( cmd_split ) < 1 ){
            vector_destroy( cmd_split );
            cmd_split = NULL;
            return -1;
        }
        // printf( "%s\n", (char*)vector_get( cmd_his, (int)vector_size(cmd_his)-1 ) );
        if( strlen( vector_get( cmd_split, 0 ) ) == 1 ){
            print_command( (char*)vector_get( cmd_his, (int)vector_size(cmd_his) - 1) );
            command( (char*)vector_get( cmd_his, (int)vector_size(cmd_his) - 1) );
            return 4;
        }
        char* prefix = (char*) cmd + 1;
        // printf( "prefix = %s\n", prefix );
        for( size_t i = vector_size( cmd_his ); i > 0; i-- ){
            char* get_cmd = (char*)vector_get( cmd_his, i - 1 );
            if( !strncmp( get_cmd, prefix, strlen(prefix) ) ){
                print_command( get_cmd );
                //handle and exec command
                command( get_cmd );
                vector_destroy( cmd_split );
                cmd_split = NULL;
                return 4;
            }
        }
        print_no_history_match();
        vector_destroy( cmd_split );
        cmd_split = NULL;
        return 0;
    }
    else if( !strcmp( (char*) vector_get ( cmd_split, 0 ), "exit" )){
        // printf( "built-in exit case\n" );
        if( vector_size( cmd_split ) != 1 ){
            vector_destroy( cmd_split );
            cmd_split = NULL;
            return -1;
        }
        vector_destroy( cmd_split );
        cmd_split = NULL;
        shell_exit(1);
        // shell_exit( 0 );
        return 5;
    }
    else if( !strcmp( (char*) vector_get ( cmd_split, 0 ), "kill" ) ){
        // printf("kill case\n");
        FILE* file;
        if( re_info->type != NULL ){
            file = setup_redirection( re_info );
        }
        pid_t pid = atoi( vector_get ( cmd_split, 1 ) );
        if( vector_size( cmd_split ) != 2 ){
            print_invalid_command( cmd );
            if( re_info->type != NULL )
                restore_redirection( file );
            return -1;
        }
        else{
            // printf("start kill\n");
            // printf("vector_size(processes) = %zu\n", vector_size(processes));
            for( size_t i = 0; i < vector_size( processes ); i++ ){
                process* cur = vector_get( processes, i );
                // printf("cur->pid = %d\n", cur->pid);
                if( cur->pid ==  pid ){
                    kill( cur->pid, SIGKILL );
                    print_killed_process( cur->pid, cur->command );
                    // process_destroy( cur->pid );
                    set_process_status( pid, "KILLED" );
                    vector_push_back( cmd_his, cmd );
                    // printf("kill success!\n");
                    if( re_info->type != NULL )
                        restore_redirection( file );
                    // printf("restore redirection\n");
                    return 7;
                }
            }
            if( re_info->type != NULL )
                restore_redirection( file );
            print_no_process_found( pid );
            return 0;
        }
    }
    else if( !strcmp( (char*) vector_get ( cmd_split, 0 ), "stop" ) ){
        // printf("stop case\n");
        FILE* file;
        if( re_info->type != NULL ){
            file = setup_redirection( re_info );
        }
        pid_t pid = atoi( vector_get ( cmd_split, 1 ) );
        if( vector_size( cmd_split ) != 2 ){
            print_invalid_command( cmd );
            if( re_info->type != NULL )
                restore_redirection( file );
            return -1;
        }
        else{
            for( size_t i = 0; i < vector_size( processes ); i++ ){
                process* cur = vector_get( processes, i );
                if( cur->pid == pid ){
                    kill( cur->pid, SIGSTOP );
                    print_stopped_process( cur->pid, cur->command );
                    vector_push_back( cmd_his, cmd );
                    // printf("stop success!\n");
                    if( re_info->type != NULL )
                        restore_redirection( file );
                    return 8;
                }
            }
            print_no_process_found( pid );
            if( re_info->type != NULL )
                restore_redirection( file );
            return 0;
        }
    }
    else if( !strcmp( (char*) vector_get ( cmd_split, 0 ), "cont" ) ){
        // printf("cont case\n");
        FILE* file;
        if( re_info->type != NULL ){
            file = setup_redirection( re_info );
        }
        pid_t pid = atoi( vector_get ( cmd_split, 1 ) );
        if( vector_size( cmd_split ) != 2 ){
            print_invalid_command( cmd );
            if( re_info->type != NULL )
                restore_redirection( file );
            return -1;
        }
        else{
            for( size_t i = 0; i < vector_size( processes ); i++ ){
                process* cur = vector_get( processes, i );
                if( cur->pid ==  pid ){
                    kill( cur->pid, SIGCONT );
                    print_continued_process( cur->pid, cur->command );
                    vector_push_back( cmd_his, cmd );
                    // printf("cont success!\n");
                    if( re_info->type != NULL )
                        restore_redirection( file );
                    return 9;
                }
            }
            print_no_process_found( pid );
            if( re_info->type != NULL )
                restore_redirection( file );
            return 0;
        }
    }
    else if( !strcmp( (char*) vector_get ( cmd_split, 0 ), "ps") ){
        // printf("ps case\n");
        if( vector_size( cmd_split ) != 1 ){
            print_invalid_command( cmd );
            return -1;
        }
        print_process_info_header();
        for (size_t i = 0; i < vector_size(processes); ++i) {
            // printf( "processes size = %zu\n", vector_size( processes ));
            process *p = vector_get(processes, i);
            if (strcmp(p->status, "KILLED")) {
                // create process info and print
                process_info* info = create_info( p->command, p->pid );
                print_process_info( info );
                free( info->command );
                free( info->start_str );
                free( info->time_str );
                free( info );
            }
        }
        vector_push_back( cmd_his, cmd );
        // printf("ps success!\n");
        return 10;
    }
    else{
        vector_destroy( cmd_split );
        cmd_split = NULL;
        return 6;
    }
}

vector* cmd_preprocessor( char* cmd ){
    // remove space
    char* start = cmd;
    char* end = cmd + strlen( cmd ) - 1;
    while( *start && isspace( (unsigned char) *start ) )
        start ++;
    while( end > start && isspace( (unsigned char) *end ) )
        end --;
    int len = end - start + 1;
    memmove( cmd, start, len );
    cmd[ len ] = '\0';
    
    // split cmd
    vector* cmd_vec = string_vector_create();
    sstring* str_cmd = cstr_to_sstring( cmd );
    cmd_vec = sstring_split( str_cmd, ' ' );
    sstring_destroy( str_cmd );
    return cmd_vec;
}

char* logical( char* cmd ){
    if( strstr( cmd, "&&" ) ) return "&&";
    if( strstr( cmd, "||" ) ) return "||";
    if( strstr( cmd, ";" ) ) return ";";
    return NULL;
}

char* redirector( char* cmd ){
    if( strstr( cmd, ">" ) ) return ">";
    if( strstr( cmd, ">>" ) ) return ">>";
    if( strstr( cmd, "<" ) ) return "<";
    return NULL;
}

int external( char* cmd ){
    // printf("external command\n");
    vector* cmd_split = string_vector_create();
    cmd_split = cmd_preprocessor( cmd );
    char **args = malloc( ( vector_size(cmd_split) + 1 ) * sizeof(char*) );  // +1 for the NULL terminator
    if( args == NULL ){
        perror( "memory allocation error" );
        shell_exit( 1 );
        return 0;
    }

    fflush( stdout );
    pid_t pid = fork();
    add_process( cmd, "RUNNING", pid );

    if( pid < 0 ){
        print_fork_failed();
        shell_exit( 1 );
        return 0;
    }
    else if( pid == 0 ){
        // printf( "child process\n" );
        // redirection_info* re_info = redirect( cmd );
        // if( re_info != NULL )
        //     handle_redirection( re_info );
        if( bg == 1 ){
            if( setpgid( getpid(), getpid() ) == -1 ){
                print_setpgid_failed();
                fflush( stdout );
                shell_exit(1);
            }
        }

        fflush( stdout );
        
        for(size_t i = 0; i < vector_size(cmd_split); i++) {
            args[i] = (char *)vector_get(cmd_split, i);
        }
        args[ vector_size(cmd_split) ] = (char*) NULL;
        // printf("execute command");
        execvp( args[0], args );

        print_exec_failed( cmd );
        fflush( stdout );
        exit(0);
        return 0;
    }
    else{
        // printf( "parent process\n" );
        // free_args( args );
        print_command_executed( pid );
        int status = 0;
        if( bg == 1 ){
            waitpid( pid, &status, WNOHANG );
        }
        else{
            if( waitpid( pid, &status, 0 ) == -1 ){
                print_wait_failed();
                shell_exit( -1 );
            }
            else if ( WIFEXITED(status) ) {
                if ( WEXITSTATUS(status) != 0 ){
                    shell_exit( 1 );
                }
                fflush(stdout);
                set_process_status( pid, "KILLED" );
            } 
            else if ( WIFSIGNALED(status) ) {
                set_process_status( pid, "KILLED" );
            }
        }
        vector_push_back( cmd_his, cmd );
        return 1;
    }
}

void shell_exit( int status ){
    for( size_t i = 0; i < vector_size( processes ); i++ ){
        process *p = (process*)vector_get( processes, i );
        pid_t pgid = getpgid( p->pid );
        if( strcmp( p->status, "KILLED" ) && pgid != p->pid )
            kill( p->pid, SIGKILL );
    }
    if( h_filename != NULL ){
        // printf( "write history file\n" );
        write_history( h_filename ); 
    }
    if( script != NULL ){
        fclose( script );
        script = NULL;
    }
    // printf( "shell exit function: free meemory!\n" );
    vector_destroy( cmd_his );
    cmd_his = NULL;
    vector_destroy( processes );
    processes = NULL;
    // free( cmd_source );
    // cmd_source = NULL;
    // free( script_filename );
    // script_filename = NULL;
    vector_destroy( script_cmd );
    script_cmd = NULL;
    free( h_filename );
    h_filename = NULL;
    
    exit( status );
    return ;
}

void write_history( char* path ){
    // printf( "path = %s\n", path );
    history = fopen( path, "w" );
    if( history == NULL ){
        print_history_file_error();
        return ;
    }
    for( size_t i = 0; i < vector_size( cmd_his ); i++ ){
        // printf( "write cmd = %s\n", (char*)vector_get( cmd_his, i ));
        fprintf( history, "%s\n", (char*)vector_get( cmd_his, i ) );
    }
    fclose( history );
    history = NULL;
}

size_t set_process_status( pid_t pid, char* status ){
    for( size_t i = 0; i < vector_size( processes ); i++ ){
        process* p = vector_get( processes, i );
        if( p->pid == pid ){
            p->status = status;
            return i;
        }
    }
    return -1;
}

void free_args(char **args) {
    while (*args) {
        free(*args);
        args++;
    }
    free(args);
}

process_info* create_info( char* cmd, pid_t pid ){
    process_info* info = (process_info*) malloc ( sizeof(process_info) );
    info->command = malloc ( ( strlen( cmd ) + 1 ) * sizeof( char ) );
    strcpy( info->command, cmd );
    info->pid = pid;

    char path[100];
    snprintf( path, 40, "/proc/%d/status", pid );
    FILE* status = fopen( path, "r" );
    if( status == NULL ){
        print_script_file_error();
        exit(1);
    }
    char line[1000];
    while( fgets( line, 100, status ) ){
        if( !strncmp( line, "State:", 6) ){
            char* p = line + 7;
            while( isspace( *p ) )
                p ++;
            info->state = *p;
        }
        else if( !strncmp( line, "Threads:", 8 ) ){
            char* p = line + 9;
            char* ptr_thread;
            while( isspace( *p ) )
                p ++;
            info->nthreads = strtol( p, &ptr_thread, 10 );
        }
        else if( !strncmp( line, "VmSize:", 7 ) ){
            char* p = line + 8;
            char* ptr_vsize;
            while( isspace( *p ) )
                p ++;
            info->vsize = strtol( p, &ptr_vsize, 10 );
        }
    }
    fclose( status );

    FILE* stat_sys = fopen( "/proc/stat", "r" );
    if( stat_sys == NULL ){
        print_script_file_error();
        exit(1);
    }
    unsigned long btime;
    while( fgets( line, 40, stat_sys ) ){
        if( !strncmp( line, "btime", 5 ) ){
            char* p = line + 6;
            char* ptr_btime;
            while( isspace( *p ) )
                p ++;
            btime = strtol( p, &ptr_btime, 10 );
        }
    }
    fclose( stat_sys );
    
    snprintf( path, 100, "/proc/%d/stat", pid );
    FILE* stat = fopen( path, "r" );
    if( stat == NULL ){
        print_script_file_error();
        exit(1);
    }
    fgets( line, 100, stat );
    fclose( stat );
    char* p = strtok( line, " " );
    char* cpu;
    unsigned long utime, stime;
    unsigned long starttime;
    int i = 1;
    while( p != NULL ){
        if( i == 14 )
            utime = strtol( p, &cpu, 10 );
        else if( i == 15 )
            stime = strtol( p, &cpu, 10 );
        else if( i == 22 )
            starttime = strtol( p, &cpu, 10 );
        p = strtok( NULL, " " );
        i++;
    }

    char cpu_buf[100];
    unsigned long cpu_time = ( utime + stime ) / sysconf( _SC_CLK_TCK );
    if( !execution_time_to_string( cpu_buf, 100, cpu_time / 60, cpu_time % 60 ) )
        exit(1);
    info->time_str = malloc ( ( strlen(cpu_buf) + 1 ) * sizeof( char ) );
    strcpy( info->time_str, cpu_buf );

    time_t start = starttime / sysconf(_SC_CLK_TCK) + btime;
    struct tm* time_info = localtime( &start );
    char ptr_start[100];
    if( !time_struct_to_string( ptr_start, 100, time_info ) )
        exit(1);
    info->start_str = calloc ( (strlen(ptr_start) + 1), sizeof(char) );
    strcpy( info->start_str, ptr_start );
    return info;
}

redirection_info* redirect( char* cmd ){
    char* operator = redirector( cmd );
    redirection_info* info = (redirection_info*) malloc ( sizeof(redirection_info) );
    if( operator ){
        char* pos = strstr(cmd, operator);
        if (pos != NULL) {
            *pos = '\0'; 
            info->cmd = cmd;
            // printf("info->cmd = %s\n", info->cmd);
            char* filename_start = pos + strlen( operator );
            while(*filename_start && isspace(*filename_start)){
                filename_start++;
            }
            info->filename = filename_start;
            // printf("info->filename = %s\n", info->filename);
        }
        info->type = operator;
    }
    return info;
}

FILE* setup_redirection(redirection_info *redir) {
    if (!redir) return NULL;

    original_stdout = dup(STDOUT_FILENO);  // Save original stdout
    original_stdin = dup(STDIN_FILENO);    // Save original stdin

    FILE *file = NULL;

    if (!strcmp(redir->type, ">")) {
        file = fopen(redir->filename, "w");
    } 
    else if (!strcmp(redir->type, "<")) {
        file = fopen(redir->filename, "r");
    } 
    else if (!strcmp(redir->type, ">>")) {
        file = fopen(redir->filename, "a");
    }

    if (!file) {
        perror("Failed to open redirection file");
        exit(1);
    }

    int fd = fileno(file);

    if (!strcmp(redir->type, ">") || !strcmp(redir->type, ">>")) {
        dup2(fd, STDOUT_FILENO);
    } 
    else if (!strcmp(redir->type, "<")) {
        dup2(fd, STDIN_FILENO);
    }

    return file;
}

void restore_redirection(FILE *file) {
    if (file) {
        // Restore stdout and stdin to their original state
        dup2(original_stdout, STDOUT_FILENO);
        dup2(original_stdin, STDIN_FILENO);

        // Close the saved file descriptors
        close(original_stdout);
        close(original_stdin);

        fclose(file);
    }
}


