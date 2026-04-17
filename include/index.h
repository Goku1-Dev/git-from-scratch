#ifndef INDEX_H
#define INDEX_H

#include <stdint.h>
#include <stddef.h>
#include <sys/stat.h>

/* A simplified index entry */
typedef struct {
    uint32_t ctime_sec;
    uint32_t mtime_sec;
    uint32_t dev;
    uint32_t ino;
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t file_size;
    unsigned char hash[20];
    uint16_t flags; // stores path length
    char *path;
} IndexEntry;

typedef struct {
    IndexEntry *entries;
    uint32_t entry_count;
    uint32_t entry_capacity;
} GitIndex;

/* Load the index from .mygit/index into memory */
GitIndex *read_index(void);

/* Write the index struct back to .mygit/index */
int write_index(GitIndex *idx);

/* Add or update a file in the index */
int add_to_index(GitIndex *idx, const char *path, const unsigned char *hash, struct stat *st);

/* Generate a tree object from the index recursively */
char *write_tree_from_index(GitIndex *idx);

/* Free the index from memory */
void free_index(GitIndex *idx);

#endif // INDEX_H
