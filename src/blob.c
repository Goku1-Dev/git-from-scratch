#define _POSIX_C_SOURCE 200809L
#include "blob.h"
#include "object.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

char *hash_blob_file(const char *path) {
    size_t size;
    unsigned char *data = (unsigned char *)read_file(path, &size);
    if (!data)
        return NULL;

    char hex[41];
    if (write_object("blob", data, size, hex) != 0) {
        free(data);
        return NULL;
    }

    free(data);
    return strdup(hex);
}