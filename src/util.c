#include "../include/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

char *read_file(const char *path, size_t *size_out) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long size = ftell(f);
    if (size < 0)                     { fclose(f); return NULL; }
    rewind(f);

    char *buffer = malloc((size_t)size);
    if (!buffer)                      { fclose(f); return NULL; }

    if (fread(buffer, 1, (size_t)size, f) != (size_t)size) {
        free(buffer); fclose(f); return NULL;
    }

    fclose(f);
    *size_out = (size_t)size;
    return buffer;
}

int create_dir(const char *path) {
    return mkdir(path, 0755);
}

int write_file(const char *path, const void *data, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f)
        return -1;

    if (fwrite(data, 1, size, f) != size) {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}