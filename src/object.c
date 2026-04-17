#include "../include/object.h"
#include "../include/hash.h"
#include "../include/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>
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

  uLongf compressed_size = compressBound(total);
  unsigned char *compressed_data = malloc(compressed_size);
  if (!compressed_data) {
    free(store);
    return -1;
  }

  if (compress(compressed_data, &compressed_size, store, total) != Z_OK) {
    free(store);
    free(compressed_data);
    return -1;
  }

  int rc = write_file(fullpath, compressed_data, compressed_size);
  free(store);
  free(compressed_data);
  return rc;
}

unsigned char *read_object(const char *hex, size_t *size_out) {
  char path[300];
  snprintf(path, sizeof(path), ".mygit/objects/%c%c/%s", hex[0], hex[1],
           hex + 2);

  size_t compressed_size;
  char *compressed_data = read_file(path, &compressed_size);
  if (!compressed_data)
    return NULL;

  uLongf uncompressed_max = 10 * 1024 * 1024; // Support up to 10MB loose objects
  unsigned char *uncompressed_data = malloc(uncompressed_max);
  if (!uncompressed_data) {
    free(compressed_data);
    return NULL;
  }

  if (uncompress(uncompressed_data, &uncompressed_max, (const unsigned char *)compressed_data, compressed_size) != Z_OK) {
    free(compressed_data);
    free(uncompressed_data);
    return NULL;
  }

  free(compressed_data);
  *size_out = uncompressed_max;
  return uncompressed_data;
}