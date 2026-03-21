#include "object.h"
#include "hash.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int write_object(const char *type, const unsigned char *data, size_t size,
                 char *hex_out) {
    /* Build header: "<type> <size>\0" */
    char header[64];
    int header_len = snprintf(header, sizeof(header), "%s %zu", type, size) + 1;

    size_t total = (size_t)header_len + size;
    unsigned char *store = malloc(total);
    if (!store)
        return -1;

    memcpy(store, header, (size_t)header_len);
    memcpy(store + header_len, data, size);

    /* Hash the full store */
    unsigned char hash[20];
    sha1_hash(store, total, hash);
    hash_to_hex(hash, hex_out);
    hex_out[40] = '\0';

    /* .mygit/objects/<xx>/<38> */
    char dir[256];
    snprintf(dir, sizeof(dir), ".mygit/objects/%c%c", hex_out[0], hex_out[1]);
    mkdir(dir, 0755);

    char fullpath[300];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, hex_out + 2);

    int rc = write_file(fullpath, store, total);
    free(store);
    return rc;
}

unsigned char *read_object(const char *hex, size_t *size_out) {
    char path[300];
    snprintf(path, sizeof(path), ".mygit/objects/%c%c/%s", hex[0], hex[1],
             hex + 2);

    size_t size;
    char *data = read_file(path, &size);
    if (!data)
        return NULL;

    *size_out = size;
    return (unsigned char *)data;
}