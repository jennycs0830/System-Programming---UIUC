/**
 * finding_filesystems
 * CS 341 - Fall 2023
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // Write tests here!
    file_system* fs = open_fs("test.fs");
    char *str = calloc(1, 102500);
    off_t off = 16384;
    char *test = "test write works well";
    minixfs_write(fs, "/goodies/triplanetary.txt", test, strlen(test), &off);
    off = 16041;
    minixfs_read(fs, "/goodies/triplanetary.txt", str, 102499, &off);
    puts(str);
    printf("off:%ld\n", off);
    free(str);
}