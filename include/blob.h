#ifndef BLOB_H
#define BLOB_H

/*
 * Hash the file at `path` as a Git blob object, write it to the object store,
 * and return a heap-allocated 40-char hex string (caller must free).
 * Returns NULL on error.
 */
char *hash_blob_file(const char *path);

#endif