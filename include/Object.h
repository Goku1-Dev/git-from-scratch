#ifndef OBJECT_H
#define OBJECT_H

#include <stddef.h>

/*
 * Write a loose object to .mygit/objects/<xx>/<38 chars>.
 * `type`  – e.g. "blob", "tree", "commit"
 * `data`  – raw payload (without header)
 * `size`  – payload length
 * `hex_out` – caller-supplied buffer of at least 41 bytes; filled with hash
 * Returns 0 on success, -1 on error.
 */
int write_object(const char *type, const unsigned char *data, size_t size,
                 char *hex_out);

/*
 * Read a loose object.  On success returns a malloc'd buffer containing
 * the full file (header + payload) and sets *size_out to its length.
 * Returns NULL on error.
 */
unsigned char *read_object(const char *hex, size_t *size_out);

#endif