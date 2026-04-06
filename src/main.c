#define _POSIX_C_SOURCE 200809L
#include "../include/blob.h"
#include "../include/commit.h"
#include "../include/hash.h"
#include "../include/object.h"
#include "../include/refs.h"
#include "../include/tree.h"
#include "../include/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef int (*command_fn)(int argc, char *argv[]);

typedef struct {
  const char *name;
  command_fn fn;
} Command;

/* ── init ──────────────────────────────────────────────────────────────── */
static int cmd_init(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  create_dir(".mygit");
  create_dir(".mygit/objects");
  create_dir(".mygit/refs");
  create_dir(".mygit/refs/heads");

  if (write_file(".mygit/HEAD", "ref: refs/heads/main\n", 21) != 0) {
    perror("HEAD");
    return 1;
  }

  printf("Initialized empty mygit repository in .mygit/\n");
  return 0;
}

/* ── hash-object ───────────────────────────────────────────────────────── */
static int cmd_hash_object(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: mygit hash-object <file>\n");
    return 1;
  }

  char *hex = hash_blob_file(argv[2]);
  if (!hex) {
    perror("hash_blob_file");
    return 1;
  }

  printf("%s\n", hex);
  free(hex);
  return 0;
}

/* ── cat-file ──────────────────────────────────────────────────────────── */
static int cmd_cat_file(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: mygit cat-file <hash>\n");
    return 1;
  }

  size_t size;
  unsigned char *data = read_object(argv[2], &size);
  if (!data) {
    perror("read_object");
    return 1;
  }

  /* Find null separator between header and content */
  unsigned char *content = memchr(data, '\0', size);
  if (!content) {
    fprintf(stderr, "Invalid object format\n");
    free(data);
    return 1;
  }

  content++; /* move past '\0' */
  size_t content_size = size - (size_t)(content - data);
  fwrite(content, 1, content_size, stdout);
  putchar('\n');

  free(data);
  return 0;
}

/* ── write-tree ────────────────────────────────────────────────────────── */
static int cmd_write_tree(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  char *hex = write_tree_dir(".");
  if (!hex) {
    fprintf(stderr, "write-tree failed\n");
    return 1;
  }

  printf("%s\n", hex);
  free(hex);
  return 0;
}

/* ── commit-tree ───────────────────────────────────────────────────────── */
static int cmd_commit_tree(int argc, char *argv[]) {
  if (argc < 4) {
    fprintf(stderr, "Usage: mygit commit-tree <tree_hash> -m <message>\n");
    return 1;
  }

  const char *tree_hash = argv[2];
  const char *message = NULL;

  for (int i = 3; i < argc; i++) {
    if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
      message = argv[i + 1];
      break;
    }
  }

  if (!message) {
    fprintf(stderr, "Commit message required (-m)\n");
    return 1;
  }

  char *hex = commit_tree(tree_hash, NULL, message);
  if (!hex)
    return 1;

  printf("%s\n", hex);
  free(hex);
  return 0;
}

/* ── commit ────────────────────────────────────────────────────────────── */
static int cmd_commit(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: mygit commit -m <message>\n");
    return 1;
  }

  const char *message = NULL;
  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
      message = argv[i + 1];
      break;
    }
  }

  if (!message) {
    fprintf(stderr, "Commit message required (-m)\n");
    return 1;
  }

  /* 1. Write tree */
  char *tree_hash = write_tree_dir(".");
  if (!tree_hash) {
    fprintf(stderr, "write-tree failed\n");
    return 1;
  }

  /* 2. Get parent (HEAD) */
  char *parent_hash = resolve_head();

  /* 3. Create commit */
  char *commit_hash = commit_tree(tree_hash, parent_hash, message);
  free(tree_hash);
  if (parent_hash)
    free(parent_hash);

  if (!commit_hash) {
    fprintf(stderr, "commit-tree failed\n");
    return 1;
  }

  /* 4. Update ref (always update refs/heads/main for now, or follow HEAD) */
  /* Let's see what HEAD points to */
  size_t head_size;
  char *head_content = read_file(".mygit/HEAD", &head_size);
  if (head_content) {
    head_content[strcspn(head_content, "\n")] = '\0';
    if (strncmp(head_content, "ref: ", 5) == 0) {
      update_ref(head_content + 5, commit_hash);
    } else {
      /* Detached HEAD - update HEAD directly?
         Usually commit updates the branch HEAD points to. */
      write_file(".mygit/HEAD", commit_hash, 40);
      write_file(".mygit/HEAD", "\n", 1);
    }
    free(head_content);
  } else {
    /* Fallback */
    update_ref("refs/heads/main", commit_hash);
  }

  printf("[%s] %s\n", commit_hash, message);
  free(commit_hash);
  return 0;
}

/* ── update-ref ────────────────────────────────────────────────────────── */
static int cmd_update_ref(int argc, char *argv[]) {
  if (argc < 4) {
    fprintf(stderr, "Usage: mygit update-ref <refname> <hash>\n");
    fprintf(stderr, "  e.g. mygit update-ref refs/heads/main <commit-hash>\n");
    return 1;
  }

  if (update_ref(argv[2], argv[3]) != 0) {
    perror("update_ref");
    return 1;
  }

  return 0;
}

/* ── log ───────────────────────────────────────────────────────────────── */
static int cmd_log(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  char *hex = resolve_head();
  if (!hex) {
    fprintf(stderr, "No commits yet\n");
    return 1;
  }

  /* Walk commit chain */
  while (hex) {
    printf("commit %s\n", hex);

    size_t size;
    unsigned char *raw = read_object(hex, &size);
    free(hex);
    hex = NULL;

    if (!raw)
      break;

    /* Skip header */
    unsigned char *content = memchr(raw, '\0', size);
    if (!content) {
      free(raw);
      break;
    }
    content++;

    /* Print commit body */
    size_t csize = size - (size_t)(content - raw);
    fwrite(content, 1, csize, stdout);
    putchar('\n');

    /* Look for "parent <hex>" line to continue walking */
    char *parent_line = strstr((char *)content, "parent ");
    if (parent_line) {
      parent_line += 7; /* skip "parent " */
      hex = strndup(parent_line, 40);
    }

    free(raw);
  }

  return 0;
}

/* ── dispatch table ────────────────────────────────────────────────────── */
static Command commands[] = {
    {"init", cmd_init},
    {"hash-object", cmd_hash_object},
    {"cat-file", cmd_cat_file},
    {"write-tree", cmd_write_tree},
    {"commit-tree", cmd_commit_tree},
    {"commit", cmd_commit},
    {"update-ref", cmd_update_ref},
    {"log", cmd_log},
};

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: mygit <command>\n");
    return 1;
  }

  for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    if (strcmp(argv[1], commands[i].name) == 0)
      return commands[i].fn(argc, argv);
  }

  fprintf(stderr, "Unknown command: %s\n", argv[1]);
  return 1;
}