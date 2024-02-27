/**
 * finding_filesystems
 * CS 341 - Fall 2023
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>

// functions
const char* get_filename_from_path( const char* path );
int permission_set( inode* node );
int write_permission( int perm_set, inode* node );
// int assign_new_data_block( inode* node, file_system* fs, char* fs_entry );
char* get_parent_path( const char* path, const char* filename );
int max_filesize();

/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
char *minixfs_virtual_path_names[] = {"info", /* add your paths here*/};

/**
 * Forward declaring block_info_string so that we can attach unused on it
 * This prevents a compiler warning if you haven't used it yet.
 *
 * This function generates the info string that the virtual endpoint info should
 * emit when read
 */
static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
static char *block_info_string(ssize_t num_used_blocks) {
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string,
             "Free blocks: %zd\n"
             "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

int minixfs_chmod(file_system *fs, char *path, int new_permissions) {
    // Thar she blows!
    inode* node = get_inode( fs, path );
    if( node == NULL ){
        // fprintf( stderr, "chmod failed to get inode\n" );
        errno = ENOENT;
        return -1;
    }
    // fprintf( stderr, "old mode = %d\n", node->mode );
    
    node->mode = new_permissions;
    node->mode |= (node->mode >> RWX_BITS_NUMBER) << RWX_BITS_NUMBER;
    // fprintf( stderr, "new mode = %d\n", node->mode ); 

    clock_gettime(CLOCK_REALTIME, &node->ctim);
    // fprintf( stderr, "chmod successful return\n" );
    return 0;
}

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
    // Land ahoy!
    inode* node = get_inode( fs, path );
    if( node == NULL ){
        // fprintf( stderr, "chown failed to get inode\n" );
        errno = ENOENT;
        return -1;
    }

    if( owner != ((uid_t) -1) ){
        node->uid = owner;
    }
    if( group != ((gid_t) -1) ){
        node->gid = group;
    }
    clock_gettime(CLOCK_REALTIME, &node->ctim);
    // fprintf( stderr, "chown successful return\n" );
    return 0;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
    // fprintf( stderr, "\ncreate inode for path funciton\n" );
    // check if path already exist in parent inode
    inode* child_inode = get_inode( fs, path );
    if( child_inode != NULL ){ 
        // fprintf( stderr, "file inode already exist\n" );
        return NULL;
    }

    // check if node cannot be created
    // No avaible node
    inode_number new_inode = first_unused_inode( fs );
    if( new_inode == -1 ){
        // fprintf( stderr, "inode cannot be created because no space left\n" );
        return NULL;
    }
    
    // User has no write permission to parent directory
    // parent's inode 
    char* filename;
    inode* parent_inode = parent_directory( fs, path, (const char**) &filename );
    if( parent_inode == NULL ){
        // fprintf( stderr, "parent node do not exist\n" );
        return NULL;
    }
    if( !valid_filename( filename ) ){
        // fprintf( stderr, "invalid filename\n" );
        return NULL;
    }

    // check parent inode own which set of permission (UGO)
    int perm_set = permission_set( parent_inode ); // 1: user, 2: group, 3: others
    // // check if parent inode has write permission
    int write_perm = write_permission( perm_set, parent_inode );
    if( write_perm == 0 ){ 
        // fprintf( stderr, "user has no write permission\n" );
        return NULL; 
    }

    // create a new dirent
    minixfs_dirent new_dirent;  
    new_dirent.name = filename;
    new_dirent.inode_num = new_inode;
    
    char fs_entry[FILE_NAME_ENTRY];
    make_string_from_dirent( fs_entry, new_dirent );

    // check if the new inode needs a new data block & get data block number
    size_t file_size = parent_inode->size;
    size_t block_num = file_size / sizeof( data_block );
    size_t block_remain = file_size % sizeof( data_block );

    // check if there still enough space for new node
    if( file_size <= ( NUM_DIRECT_BLOCKS * sizeof( data_block ) ) ){
        if( block_remain == 0 ){
            // need new data block
            data_block_number new_block = first_unused_data( fs );
            if( new_block == -1 ){
                // fprintf( stderr, "failed to retreive first unused block\n" );
                return NULL;
            }
            char* write_place = (char*) ( fs->data_root + new_block );
            memcpy( write_place, fs_entry, FILE_NAME_ENTRY );

            parent_inode->direct[ block_num ] = new_block;
            set_data_used( fs, new_block, 1 );
        }
        else{
            char* write_place = (char*) (fs->data_root + parent_inode->direct[ block_num ] );
            memcpy( write_place + block_remain, fs_entry, FILE_NAME_ENTRY );
        }
        parent_inode->size += FILE_NAME_ENTRY;
        inode* new = fs->inode_root + new_inode;
        init_inode( parent_inode, new );
        return new;
    }
    // direct block full, check if indirect block exists
    else if( file_size <= ( (NUM_DIRECT_BLOCKS + NUM_INDIRECT_BLOCKS) * sizeof( data_block ) ) ){
        if( parent_inode->indirect == -1 ){
            // need to assign indirect block to node
            // one data block for storing an array of direct data block nuber
            data_block_number indirect_array = first_unused_data( fs );
            if( indirect_array == -1 ){
                // fprintf( stderr, "failed to allocate new data block" );
                return NULL;
            }
            parent_inode->indirect = indirect_array;
            set_data_used( fs, indirect_array, 1 );
            data_block_number* block_arr = (data_block_number*) ( fs->data_root + indirect_array );

            // another data block for storing current fs_entry
            data_block_number new_block = first_unused_data( fs );
            if( new_block == -1 ){
                // fprintf( stderr, "failed to allocate new data block" );
                return NULL;
            }
            block_arr[0] = new_block;
            set_data_used( fs, new_block, 1 );
            
            char* write_place = (char*) ( fs->data_root + new_block );
            memcpy( write_place, fs_entry, FILE_NAME_ENTRY );
        }
        else{
            size_t block_index = block_num - NUM_DIRECT_BLOCKS;
            
            if( block_remain != 0 ){
                data_block_number* block_arr = (data_block_number*) (fs->data_root + parent_inode->indirect);
                data_block_number new_block = block_arr[ block_index ];
                char* write_place = (char*) (fs->data_root + new_block );
                memcpy( write_place + block_remain, fs_entry, FILE_NAME_ENTRY );
            }
            else{
                data_block_number new_block = first_unused_data( fs );
                if( new_block == -1 ){
                    // fprintf( stderr, "failed to allocate new data block" );
                    return NULL;
                }
                data_block_number* block_arr = (data_block_number*) (fs->data_root + parent_inode->indirect) ;
                block_arr[ block_index ] = new_block;
                set_data_used( fs, new_block, 1 );

                char* write_place = (char*) ( fs->data_root + new_block );
                memcpy( write_place, fs_entry, FILE_NAME_ENTRY );
            }
        }
        parent_inode->size += FILE_NAME_ENTRY;
        inode* new = fs->inode_root + new_inode;
        init_inode( parent_inode, new );
        return new;
    }
    return NULL;
} 

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    // fprintf( stderr, "minixfs virtual read function\n" );
    if (!strcmp(path, "info")) {
        // TODO implement the "info" virtual file here
        size_t used_blocks = 0;
        for( int i = 0; i < DATA_NUMBER; i++ ){
            if( get_data_used( fs, i ) )
                used_blocks += 1;
        }
        char* info = NULL;
        asprintf( &info, "Free blocks: %zu\n"
                         "Used blocks: %zu\n", DATA_NUMBER - used_blocks, used_blocks );
        fprintf( stderr, "Free blocks: %zu\n, Used blocks: %zu\n", DATA_NUMBER - used_blocks, used_blocks );
        size_t read_byte = count < ( strlen( info ) - *off ) ? count : ( strlen( info ) - *off );
        memcpy( buf, info + *off, read_byte );
        *off += read_byte;
    }
    else{
        errno = ENOENT;
        return -1;
    }

    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    // X marks the spot
    // fprintf( stderr, "\nminixfs_write function\n" );
    int maxSize = max_filesize();
    if( (int) (*off + count) > maxSize ){
        // fprintf( stderr, "over max filesize\n" );
        errno = ENOSPC;
        return -1;
    }

    // size_t offset = (size_t) *off;
    inode* node = get_inode( fs, path );
    if( node == NULL ){
        // fprintf( stderr, "failed to get inode, try to create\n" );
        node = minixfs_create_inode_for_path( fs, path );
        if( node == NULL ){
            // fprintf( stderr, "failed to create node\n" );
            errno = ENOSPC;
            return -1;
        }
    }

    size_t block_count = node->size / sizeof( data_block );
    size_t block_remain = node->size % sizeof( data_block );
    if( block_remain > 0 )
        block_count += 1;

    // fprintf( stderr, "node->size = %zu\n", node->size );
    // fprintf( stderr, "block count = %zu\n", block_count );
    // fprintf( stderr, "block remain = %zu\n", block_remain );
    // fprintf( stderr, "sizeof( data block ) = %zu\n", sizeof( data_block ) );

    if( minixfs_min_blockcount( fs, path, block_count ) == -1 ){
        // fprintf( stderr, "inode do not contain enough data block and failed to add one\n" );
        return -1;
    }

    size_t written_byte = 0;
    char* write_place = NULL;
    if( block_count < NUM_DIRECT_BLOCKS ){
        // direct block not full
        write_place = (char*) ( fs->data_root + node->direct[ block_count ] );
        if( block_remain + count <= sizeof( data_block ) ){
            // this data block is big enough
            memcpy( write_place + block_remain, buf, count );
            written_byte += count;
        }
        else{
            // need additional block to store exceed data
            int write_size = sizeof( data_block ) - block_remain ;
            memcpy( write_place + block_remain, buf, write_size );
            *off += write_size;
            written_byte += write_size;
            buf += write_size;

            while( written_byte != count && ( block_count < NUM_DIRECT_BLOCKS ) ){
                block_count += 1;
                write_place = (char*) ( fs->data_root + node->direct[ block_count ] );
                
                write_size = sizeof( data_block ) < count - written_byte ? sizeof( data_block ) : count - written_byte;
                memcpy( write_place, buf, write_size );
                written_byte += write_size;
                buf += write_size;
            }
        }
        if( written_byte == count ){
            *off += count;
            node->size = *off;
            clock_gettime(CLOCK_REALTIME, &node->mtim);
            clock_gettime(CLOCK_REALTIME, &node->atim);
            // fprintf( stderr, "return in direct block case\n" );
            return count;
        }
    }
    // indirect block case ( block num exceed )
    write_place = (char*) ( fs->data_root + node->indirect + (block_count - NUM_DIRECT_BLOCKS) );

    if( block_remain + count <= sizeof( data_block ) ){
        // this data block is big enough
        memcpy( write_place + block_remain, buf, count );
        written_byte += count;
    }
    else{
        // need additional block to store exceed data
        int write_size = sizeof( data_block ) - block_remain ;
        memcpy( write_place + block_remain, buf, write_size );
        *off += write_size;
        written_byte += write_size;
        buf += write_size;

        while( written_byte != count && ( block_count < NUM_DIRECT_BLOCKS ) ){
            block_count += 1;
            write_place = (char*) ( fs->data_root + node->direct[ block_count ] );
                
            write_size = sizeof( data_block ) < count - written_byte ? sizeof( data_block ) : count - written_byte;
            memcpy( write_place, buf, write_size );
            written_byte += write_size;
            buf += write_size;
        }
    }
    node->size = *off;
    clock_gettime(CLOCK_REALTIME, &node->mtim);
    clock_gettime(CLOCK_REALTIME, &node->atim);
    // fprintf( stderr, "return in indirect block case\n" );
    return count;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    // fprintf( stderr, "\nminixfs read function\n" );
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);
    // 'ere be treasure!
    if( path == NULL ){
        errno = ENOENT;
        return -1;
    }

    inode* node = get_inode( fs, path );
    if( node == NULL ){
        // fprintf( stderr, "failed get file's inode\n" );
        errno = ENOENT;
        return -1;
    }

    if( *off >= (long) node->size ){
        // fprintf( stderr, "already reach the end of the file\n" );
        return 0;
    }

    // read where
    size_t block_count = *off / sizeof( data_block );
    if( block_count > NUM_DIRECT_BLOCKS + NUM_INDIRECT_BLOCKS ){
        // fprintf( stderr, "block count over max number of blocks\n" );
        return 0;
    }
    size_t block_remain = *off % sizeof( data_block );

    // fprintf( stderr, "*off = %d\n", (int)*off );
    // fprintf( stderr, "block_count = %zu\n", block_count );
    // fprintf( stderr, "block remain = %zu\n", block_remain );
    // fprintf( stderr, "node->size = %zu\n", node->size );
    if( *off + count >= node->size ){
        count = node->size - *off;
    }
    size_t read_byte = 0;
    while( read_byte != count ){
        char* read_place = NULL;
        // fprintf( stderr, "\ncur block count = %zu\n", block_count );
        if( block_count <= NUM_DIRECT_BLOCKS ){
            // fprintf( stderr, "read direct block\n" );
            read_place = (char*) ( fs->data_root + node->direct[ block_count ] );
        }
        else if( block_count <= NUM_DIRECT_BLOCKS + NUM_INDIRECT_BLOCKS ){
            // fprintf( stderr, "read indirect block\n" );
            // fprintf( stderr, "indirect bloc number = %d\n", (int) node->indirect );
            read_place = (char*) ( fs->data_root + node->indirect + ( block_count - NUM_DIRECT_BLOCKS ) );
        }
        // fprintf( stderr, "count = %zu\n", count );
        size_t read_size = ( count - read_byte ) < ( sizeof( data_block ) - block_remain ) ? ( count - read_byte ) : ( sizeof( data_block ) - block_remain );
        // fprintf( stderr, "read_size = %zu\n", read_size );
        // fprintf( stderr, "block remain = %zu\n", block_remain );
        // fprintf( stderr, "read_place = %s\n", read_place );
        memcpy( buf, read_place + block_remain, read_size );
        *off += read_size;
        buf += read_size;
        read_byte += read_size;
        block_remain = 0;
        block_count += 1;
    }
    clock_gettime(CLOCK_REALTIME, &node->atim);
    // fprintf( stderr, "final result read_byte = %zu\n", read_byte );
    return read_byte;
}

// functions
const char* get_filename_from_path( const char* path ){
    const char* last_slash = strrchr( path, '/' );
    if( last_slash ){
        return last_slash + 1; // point to the next index after last slash
    }
    else{
        return path; // if no slash, the entire path is filename
    }
}

int permission_set( inode* node ){
    // check parent inode own which set of permission (UGO)
    int perm_set = 0; // 1: user, 2: group, 3: others

    uid_t user_euid = geteuid();
    gid_t user_egid = getegid();
    uid_t parent_uid = node->uid;
    gid_t parent_gid = node->gid;

    if( user_euid == 0 || user_euid == parent_uid ){
        perm_set = 1; // user
    }
    else if( user_euid != parent_uid && user_egid == parent_gid ){
        perm_set = 2; // group
    }
    else{
        perm_set = 3; // others
    }
    return perm_set;
}

int write_permission( int perm_set, inode* node ){
    int write_perm = 0;
    switch( perm_set ){
        case 1: // user
            if( node->mode & S_IWUSR )
                write_perm = 1;
            break;
        case 2: // group
            if( node->mode & S_IWGRP )
                write_perm = 1;
            break;
        case 3:
            if( node->mode & S_IWOTH )
                write_perm = 1;
    }
    return write_perm;
}

char* get_parent_path( const char* path, const char* filename ){
    char* parent_path = malloc( ( strlen( path ) + 1 ) * sizeof( char ) );
    if( parent_path == NULL )
        return NULL;
    
    strcpy( parent_path, path );
    size_t filename_len = strlen( filename );
    size_t path_len = strlen( path );

    if( path_len >= filename_len && !strcmp( parent_path + path_len - filename_len, filename ) ){
        parent_path[ path_len - filename_len ] = '\0';
        if( parent_path[ path_len - filename_len - 1 ] == '/' ){
            parent_path[ path_len - filename_len - 1 ] = '\0';
        }
    }
    return parent_path;
}

int max_filesize(){
    size_t direct = NUM_DIRECT_BLOCKS * sizeof( data_block );
    size_t indirect = NUM_INDIRECT_BLOCKS * sizeof( data_block );

    int maxSize = direct + indirect;
    return maxSize;
}