#include "../include/index.h"
#include "../include/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h> // For htonl, ntohl
#include "../include/tree.h"
#include "../include/hash.h"

#define INDEX_PATH ".mygit/index"
#define INDEX_SIGNATURE 0x44495243 // "DIRC"
#define INDEX_VERSION 2

static uint32_t read_uint32(const unsigned char **p) {
    uint32_t val;
    memcpy(&val, *p, 4);
    *p += 4;
    return ntohl(val);
}

static void write_uint32(FILE *f, uint32_t val) {
    val = htonl(val);
    fwrite(&val, 1, 4, f);
}

GitIndex *read_index(void) {
    GitIndex *idx = calloc(1, sizeof(GitIndex));
    size_t size;
    unsigned char *data = (unsigned char*)read_file(INDEX_PATH, &size);
    if (!data) return idx; // Empty or missing index

    const unsigned char *p = data;
    
    uint32_t sig = read_uint32(&p);
    uint32_t ver = read_uint32(&p);
    uint32_t count = read_uint32(&p);
    
    if (sig != INDEX_SIGNATURE || ver != INDEX_VERSION) {
        free(data);
        return idx; // Invalid index
    }

    idx->entry_count = count;
    idx->entry_capacity = count + 10;
    idx->entries = calloc(idx->entry_capacity, sizeof(IndexEntry));

    for (uint32_t i = 0; i < count; i++) {
        IndexEntry *e = &idx->entries[i];
        e->ctime_sec = read_uint32(&p);
        read_uint32(&p); // ctime_nsec
        e->mtime_sec = read_uint32(&p);
        read_uint32(&p); // mtime_nsec
        e->dev = read_uint32(&p);
        e->ino = read_uint32(&p);
        e->mode = read_uint32(&p);
        e->uid = read_uint32(&p);
        e->gid = read_uint32(&p);
        e->file_size = read_uint32(&p);
        
        memcpy(e->hash, p, 20);
        p += 20;

        uint16_t flags;
        memcpy(&flags, p, 2);
        flags = ntohs(flags);
        p += 2;
        e->flags = flags;

        uint16_t path_len = flags & 0x0FFF;
        e->path = strdup((const char*)p);
        
        // Pad to multiple of 8 (entry size without path is 62)
        int entry_len = 62 + path_len;
        int pad = 8 - (entry_len % 8);
        if (pad == 8) pad = 0;
        p += path_len + pad;
    }

    free(data);
    return idx;
}

int write_index(GitIndex *idx) {
    FILE *f = fopen(INDEX_PATH, "wb");
    if (!f) return -1;

    write_uint32(f, INDEX_SIGNATURE);
    write_uint32(f, INDEX_VERSION);
    write_uint32(f, idx->entry_count);

    for (uint32_t i = 0; i < idx->entry_count; i++) {
        IndexEntry *e = &idx->entries[i];
        
        write_uint32(f, e->ctime_sec);
        write_uint32(f, 0); // ctime_nsec
        write_uint32(f, e->mtime_sec);
        write_uint32(f, 0); // mtime_nsec
        write_uint32(f, e->dev);
        write_uint32(f, e->ino);
        write_uint32(f, e->mode);
        write_uint32(f, e->uid);
        write_uint32(f, e->gid);
        write_uint32(f, e->file_size);
        
        fwrite(e->hash, 1, 20, f);
        
        uint16_t flags = htons(e->flags);
        fwrite(&flags, 1, 2, f);
        
        uint16_t path_len = strlen(e->path);
        fwrite(e->path, 1, path_len, f);
        
        int entry_len = 62 + path_len;
        int pad = 8 - (entry_len % 8);
        if (pad == 8) pad = 0;
        for (int p = 0; p < pad; p++) fputc('\0', f);
    }
    
    // Simplification: In real git, a 20-byte SHA-1 over the index content is appended.
    // For educational purposes, we skip this or just write 20 empty bytes.
    for (int i=0; i<20; i++) fputc('\0', f);

    fclose(f);
    return 0;
}

int add_to_index(GitIndex *idx, const char *path, const unsigned char *hash, struct stat *st) {
    // Check if the file is already in the index
    for (uint32_t i = 0; i < idx->entry_count; i++) {
        if (strcmp(idx->entries[i].path, path) == 0) {
            // Update existing entry
            memcpy(idx->entries[i].hash, hash, 20);
            idx->entries[i].mtime_sec = st->st_mtime;
            idx->entries[i].file_size = st->st_size;
            // Update other stat fields as needed
            return 0;
        }
    }

    // Add new entry
    if (idx->entry_count >= idx->entry_capacity) {
        idx->entry_capacity = idx->entry_capacity == 0 ? 10 : idx->entry_capacity * 2;
        idx->entries = realloc(idx->entries, idx->entry_capacity * sizeof(IndexEntry));
    }

    IndexEntry *e = &idx->entries[idx->entry_count];
    e->ctime_sec = st->st_ctime;
    e->mtime_sec = st->st_mtime;
    e->dev = st->st_dev;
    e->ino = st->st_ino;
    e->mode = st->st_mode;
    e->uid = st->st_uid;
    e->gid = st->st_gid;
    e->file_size = st->st_size;
    memcpy(e->hash, hash, 20);
    
    uint16_t path_len = strlen(path);
    if (path_len > 0x0FFF) path_len = 0x0FFF;
    e->flags = path_len;
    e->path = strdup(path);

    idx->entry_count++;

    // Very basic sorting would normally happen here (real git index is sorted by path)
    return 0;
}

void free_index(GitIndex *idx) {
    if (!idx) return;
    for (uint32_t i = 0; i < idx->entry_count; i++) {
        free(idx->entries[i].path);
    }
    free(idx->entries);
    free(idx);
}

/* In-memory tree builder for write-tree */
typedef struct InMemTree {
    char *name;
    int is_dir;
    unsigned char hash[20];
    struct InMemTree **children;
    int child_count;
} InMemTree;

static InMemTree *create_in_mem_tree(const char *name, int is_dir) {
    InMemTree *t = calloc(1, sizeof(InMemTree));
    t->name = strdup(name);
    t->is_dir = is_dir;
    return t;
}

static InMemTree *find_or_create_child(InMemTree *parent, const char *name, int is_dir) {
    for (int i = 0; i < parent->child_count; i++) {
        if (strcmp(parent->children[i]->name, name) == 0) {
            return parent->children[i];
        }
    }
    parent->children = realloc(parent->children, (parent->child_count + 1) * sizeof(InMemTree*));
    InMemTree *child = create_in_mem_tree(name, is_dir);
    parent->children[parent->child_count++] = child;
    return child;
}

static void add_path_to_tree(InMemTree *root, const char *path, const unsigned char *hash) {
    char *path_copy = strdup(path);
    char *saveptr;
    char *token = strtok_r(path_copy, "/", &saveptr);
    
    InMemTree *current = root;
    while (token != NULL) {
        char *next_token = strtok_r(NULL, "/", &saveptr);
        if (next_token == NULL) {
            // It's a file
            InMemTree *file_node = find_or_create_child(current, token, 0);
            memcpy(file_node->hash, hash, 20);
        } else {
            // It's a directory
            current = find_or_create_child(current, token, 1);
        }
        token = next_token;
    }
    free(path_copy);
}

static void free_in_mem_tree(InMemTree *t) {
    for (int i = 0; i < t->child_count; i++) free_in_mem_tree(t->children[i]);
    free(t->children);
    free(t->name);
    free(t);
}

static char *hash_tree(InMemTree *t) {
    if (!t->is_dir) return NULL; // Files already have their hashes

    TreeEntry *entries = malloc(t->child_count * sizeof(TreeEntry));
    for (int i = 0; i < t->child_count; i++) {
        InMemTree *child = t->children[i];
        if (child->is_dir) {
            char *child_hex = hash_tree(child);
            hex_to_hash(child_hex, child->hash);
            free(child_hex);
        }
        entries[i].mode = child->is_dir ? MODE_DIR : MODE_FILE;
        entries[i].name = strdup(child->name);
        memcpy(entries[i].hash, child->hash, 20);
    }

    char *hex = write_tree(entries, t->child_count);
    for (int i = 0; i < t->child_count; i++) free(entries[i].name);
    free(entries);
    return hex;
}

char *write_tree_from_index(GitIndex *idx) {
    if (idx->entry_count == 0) return NULL;

    InMemTree *root = create_in_mem_tree("", 1);

    for (uint32_t i = 0; i < idx->entry_count; i++) {
        add_path_to_tree(root, idx->entries[i].path, idx->entries[i].hash);
    }

    char *root_hex = hash_tree(root);
    free_in_mem_tree(root);
    return root_hex;
}
