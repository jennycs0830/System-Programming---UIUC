/**
 * parallel_make
 * CS 341 - Fall 2023
 */

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "dictionary.h"
#include "set.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

/*
rule->status:
-1 => dropped
*/

static graph* dependency_graph = NULL;
static set* order = NULL;
static vector* topo_order = NULL;
static pthread_mutex_t mtx_0 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv_1 = PTHREAD_COND_INITIALIZER;
static dictionary* valid_target;

int DFS_cycle( char* target, set* visited, set* visiting );
void DFS_topological_order( char* target );
void* worker_function();
int get_target_status( char* target );

int parmake(char *makefile, size_t num_threads, char **targets) {
    // dependency graph
    dependency_graph = shallow_graph_create();
    dependency_graph = parser_parse_makefile( makefile, targets );

    // targets (vector)
    vector* targets_vec = graph_vertices( dependency_graph );
    size_t num_targets = vector_size( targets_vec );

    int cycle = 0;
    valid_target = string_to_int_dictionary_create();
    for( size_t i = 0; i < num_targets; i++ ){
        char* target = vector_get( targets_vec, i );
        int value = 0;
        dictionary_set( valid_target, target, &value );
    }
    for( size_t i = 0; i < num_targets; i++ ){
        char* target = vector_get( targets_vec, i );
        // cycle check
        set* visited = shallow_set_create();
        set* visiting = shallow_set_create();
        if( DFS_cycle( target, visited, visiting ) ){
            print_cycle_failure( target );
            cycle = 1;
        }
        set_destroy( visited );
        set_destroy( visiting );
    }

    if( cycle == 0 ){
        // get topological order
        order = shallow_set_create();
        for( size_t i = 0; i < num_targets; i++ ){
            char* target = vector_get( targets_vec, i );
            DFS_topological_order( target );
        }
        topo_order = string_vector_create();
        topo_order = set_elements( order );

        // multi-threads
        pthread_t threads[ num_threads ];
        for( size_t i = 0; i < num_threads; i++ ){
            pthread_create( &threads[i], NULL, worker_function, NULL );
        }
        for( size_t i = 0; i < num_threads; i++ ){
            if( pthread_join( threads[i], NULL ) != 0 )
                exit( 1 );
        }
        vector_destroy( topo_order );
        set_destroy( order );
    }
    graph_destroy( dependency_graph );
    vector_destroy( targets_vec );

    return 0;
}

int DFS_cycle( char* target, set* visited, set* visiting ){
    if( *((int*)dictionary_get( valid_target, target )) ){
        return 0;
    }
    if( !strcmp( target, "" ) )
        return 0;
    // check if target in visiting list -> cycle
    if( set_contains( visiting, (char*) target ) ){
        // set dropped target status
        int value = 1;
        dictionary_set( valid_target, target, &value );
        return 1;
    }
    // check if target in visited list -> return (no cycle)
    if( set_contains( visited, (char*) target ) ){
        return 0;
    }
    // DFS
    set_add( visiting , (char*) target );
    vector* dependencies = graph_neighbors( dependency_graph, target );

    for( size_t i = 0; i < vector_size( dependencies ); i++ ){
        char* neighbor_target = vector_get( dependencies, i );
        // printf( "adding neighbor \"%s\" to visting set\n", neighbor_target );
        if( DFS_cycle( neighbor_target, visited, visiting ) ){
            int value = 1;
            dictionary_set( valid_target, target, &value );
            return 1;
        }
    }
    set_remove( visiting, target );
    set_add( visited, target );
    return 0;
}

void DFS_topological_order( char* target ){
    if( !strcmp( target, "" ) )
        return ;
    if( set_contains( order, target ) )
        return ;
    vector* neighbors = graph_neighbors( dependency_graph, target );
    for( size_t i = 0; i < vector_size( neighbors ); i++ ){
        char* neighbor = vector_get( neighbors, i );
        DFS_topological_order( neighbor );
    }
    set_add( order, target );
    return ;
}

void* worker_function(){
    // printf( "worker function\n" );
    // printf( "print topo order: " );
    // for( size_t i = 0; i < vector_size( topo_order ); i++ ){
    //     printf( "%s(%zu) ", (char*) vector_get( topo_order, i ), i );
    // }
    // printf( "\n" );
    while( true ){
        pthread_mutex_lock( &mtx_1 );
        size_t num = vector_size( topo_order );
        if( num > 0 ){
            for( size_t i = 0; i < num; i++ ){
                char* target = vector_get( topo_order, i );
                // printf( "target = %s\n", target );
                int status = get_target_status( target );
                // printf( "status = %d\n", status );
                rule_t* rule = (rule_t*) graph_get_vertex_value( dependency_graph, target );
                if( status == 1 ){
                    // printf( "status = 1\n" );
                    vector_erase( topo_order, i );
                    pthread_mutex_unlock( &mtx_1 );
                    vector* commands = rule->commands;
                    // printf( "get command success! \n" );
                    int error = 0;
                    for( size_t i = 0; i < vector_size( commands ); i++ ){
                        // printf( "execute system code\n" );
                        if( system( (char*) vector_get ( commands, i ) ) != 0 ){
                            error = 1;
                            break;
                        }
                    }
                    pthread_mutex_lock( &mtx_0 );
                    if( error )
                        rule->state = -1;
                    else
                        rule->state = 1;
                    pthread_cond_broadcast( &cv_1 );
                    pthread_mutex_unlock( &mtx_0 );
                    break;
                }
                else if( status == 3 ){
                    vector_erase( topo_order, i );
                    pthread_mutex_unlock( &mtx_1 );
                    break;
                }
                else if( status == -1 || status == 2 ){
                    vector_erase( topo_order, i );
                    pthread_mutex_unlock( &mtx_1 );
                    pthread_mutex_lock( &mtx_0 );
                    if( rule->state != -1 )
                        rule->state = 1;
                    pthread_cond_broadcast( &cv_1 );
                    pthread_mutex_unlock( &mtx_0 );
                    break;
                }
                else if( i == num - 1 ){
                    pthread_cond_wait( &cv_1, &mtx_1 );
                    pthread_mutex_unlock( &mtx_1 );
                    break;
                }
            }
        }
        else{
            pthread_mutex_unlock( &mtx_1 );
            return NULL;
        }
    }
}

int get_target_status( char* target ){
    // 5 status
    // -1 => Error: Command do not execute correctly
    // 0 => In progress: Dependencies not yet satified
    // 1 => Passed: All dependencies are satisfied
    // 2 => Halted: File already up-to-date
    // 3 => Doned: Rule is already done
    rule_t* rule = (rule_t*) graph_get_vertex_value( dependency_graph, target );
    if( rule->state != 0 )
        return 3;
    
    vector* dependencies = graph_neighbors( dependency_graph, target );
    if( vector_size( dependencies ) > 0 ){
        if( access( target, F_OK ) != -1 ){
            // target is file
            // check if dependencies has file
            for( size_t i = 0; i < vector_size( dependencies ); i++ ){
                char* sub_target = vector_get( dependencies, i );
                if( access( sub_target, F_OK ) != -1 ){
                    // sub target is file
                    struct stat stat_target, stat_sub;
                    if( stat( target, &stat_target ) == -1 || stat( sub_target, & stat_sub ) == -1 ){
                        vector_destroy( dependencies );
                        return -1;
                    }
                    // compare target and sub target which is newer
                    if( difftime( stat_target.st_mtime, stat_sub.st_mtime ) < 0 ){
                        // dependencies is newer -> rebuild
                        vector_destroy( dependencies );
                        return 1;
                    }
                }
                else{
                    vector_destroy( dependencies );
                    return 1;
                }
            }
            vector_destroy( dependencies );
            return 2;
        }
        else{
            pthread_mutex_lock( &mtx_0 );
            for( size_t i = 0; i < vector_size( dependencies ); i++ ){
                rule_t* sub_rule = (rule_t*) graph_get_vertex_value( dependency_graph, vector_get( dependencies, i ) );
                if( sub_rule->state != 1 ){
                    pthread_mutex_unlock( &mtx_0 );
                    vector_destroy( dependencies );
                    return sub_rule->state;
                }
            }
            pthread_mutex_unlock( &mtx_0 );
            vector_destroy( dependencies );
            return 1;
        }
    }
    else{
        vector_destroy( dependencies );
        if( access( target, F_OK ) != -1 )
            return 2;
        else
            return 1;
    }
}