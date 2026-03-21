#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

/* Read entire file; sets *size_out.  Returns heap buffer or NULL on error. */
char *read_file(const char *path, size_t *size_out);

/* Create a directory (single level). Returns 0 on success, -1 on error. */
int create_dir(const char *path);

/* Write `size` bytes of `data` to `path`. Returns 0 on success, -1 on error. */
int write_file(const char *path, const void *data, size_t size);

#endif