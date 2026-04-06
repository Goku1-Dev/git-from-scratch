#define _POSIX_C_SOURCE 200809L
#include "../include/commit.h"
#include "../include/hash.h"
#include "../include/object.h"
#include "../include/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *commit_tree(const char *tree_hash, const char *parent_hash,
                  const char *message) {
  char content[4096];
  int len;

  if (parent_hash) {
    len = snprintf(content, sizeof(content),
                   "tree %s\n"
                   "parent %s\n"
                   "author you <you@example.com>\n"
                   "\n"
                   "%s\n",
                   tree_hash, parent_hash, message);
  } else {
    len = snprintf(content, sizeof(content),
                   "tree %s\n"
                   "author you <you@example.com>\n"
                   "\n"
                   "%s\n",
                   tree_hash, message);
  }
  if (len < 0 || (size_t)len >= sizeof(content))
    return NULL;

  char hex[41];
  if (write_object("commit", (unsigned char *)content, (size_t)len, hex) != 0)
    return NULL;

  return strdup(hex);
}