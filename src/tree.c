#define _POSIX_C_SOURCE 200809L

#include "../include/tree.h"
#include "../include/object.h"
#include "../include/blob.h"
#include "../include/hash.h"
#include "../include/util.h"

#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy)
        memcpy(copy, s, len);
    return copy;
}

/*
 * Write a tree object from an array of TreeEntry.
 *
 * Git tree format (per entry):
 *   "<mode> <name>\0<20-byte-raw-hash>"
 */
char *write_tree(TreeEntry *entries, size_t n) {
    /* Calculate payload size exactly */
    size_t payload_size = 0;
    for (size_t i = 0; i < n; i++) {
        /* octal mode string length: 6 digits max */
        char mode_str[16];
        int mode_len = snprintf(mode_str, sizeof(mode_str), "%o",
                                (unsigned)entries[i].mode);
        /* "<mode> <name>\0" + 20 raw bytes */
        payload_size += (size_t)mode_len + 1 + strlen(entries[i].name) + 1 + 20;
    }

    unsigned char *payload = malloc(payload_size);
    if (!payload)
        return NULL;

    unsigned char *p = payload;
    for (size_t i = 0; i < n; i++) {
        /* "<mode> <name>\0" */
        int written = sprintf((char *)p, "%o %s", (unsigned)entries[i].mode,
                              entries[i].name);
        p += written;
        *p++ = '\0';            /* null separator — NOT a space */

        /* 20 raw binary hash bytes */
        memcpy(p, entries[i].hash, 20);
        p += 20;
    }

    char hex[41];
    if (write_object("tree", payload, payload_size, hex) != 0) {
        free(payload);
        return NULL;
    }

    free(payload);
    return my_strdup(hex);
}

/* Recursively write tree for a directory */
char *write_tree_dir(const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (!dir)
        return NULL;

    TreeEntry  entries[1024];
    size_t     n = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0  ||
            strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, ".mygit") == 0)
            continue;

        if (n >= 1024)
            break;

        char filepath[PATH_MAX];
        snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);

        struct stat st;
        if (stat(filepath, &st) != 0)
            continue;

        entries[n].name = my_strdup(entry->d_name);
        if (!entries[n].name)
            continue;

        if (S_ISDIR(st.st_mode)) {
            entries[n].mode = MODE_DIR;
            char *subtree_hash = write_tree_dir(filepath);
            if (!subtree_hash) {
                free(entries[n].name);
                continue;
            }
            hex_to_hash(subtree_hash, entries[n].hash);
            free(subtree_hash);
        } else if (S_ISREG(st.st_mode)) {
            entries[n].mode = MODE_FILE;

            size_t size;
            unsigned char *data = (unsigned char *)read_file(filepath, &size);
            if (!data) {
                free(entries[n].name);
                continue;
            }

            char hex[41];
            if (write_object("blob", data, size, hex) != 0) {
                free(data);
                free(entries[n].name);
                continue;
            }
            free(data);
            hex_to_hash(hex, entries[n].hash);
        } else {
            /* Skip symlinks, sockets, etc. */
            free(entries[n].name);
            continue;
        }

        n++;
    }

    closedir(dir);

    char *result = write_tree(entries, n);

    for (size_t i = 0; i < n; i++)
        free(entries[i].name);

    return result;
}