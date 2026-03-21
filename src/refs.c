#define _POSIX_C_SOURCE 200809L
#include "refs.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int update_ref(const char *refname, const char *hex) {
    /* Ensure intermediate dirs exist, e.g. .mygit/refs/heads */
    char path[512];
    snprintf(path, sizeof(path), ".mygit/%s", refname);

    /* Create parent directory if needed */
    char dirpath[512];
    snprintf(dirpath, sizeof(dirpath), "%s", path);
    char *slash = strrchr(dirpath, '/');
    if (slash) {
        *slash = '\0';
        mkdir(dirpath, 0755);   /* best-effort; may already exist */
    }

    /* Write "<hex>\n" */
    char content[43];
    snprintf(content, sizeof(content), "%s\n", hex);
    return write_file(path, content, strlen(content));
}

int read_ref(const char *refname, char *hex_out) {
    char path[512];
    snprintf(path, sizeof(path), ".mygit/%s", refname);

    size_t size;
    char *data = read_file(path, &size);
    if (!data)
        return -1;

    /* Strip trailing newline */
    if (size >= 40) {
        memcpy(hex_out, data, 40);
        hex_out[40] = '\0';
        free(data);
        return 0;
    }

    free(data);
    return -1;
}

char *resolve_head(void) {
    size_t size;
    char *head = read_file(".mygit/HEAD", &size);
    if (!head)
        return NULL;

    /* Strip newline */
    head[strcspn(head, "\n")] = '\0';

    /* Symbolic ref: "ref: refs/heads/main" */
    const char *prefix = "ref: ";
    char hex[41];

    if (strncmp(head, prefix, strlen(prefix)) == 0) {
        const char *refname = head + strlen(prefix);
        int rc = read_ref(refname, hex);
        free(head);
        if (rc != 0)
            return NULL;
        return strdup(hex);
    }

    /* Detached HEAD — the file itself contains the hash */
    if (size >= 40) {
        memcpy(hex, head, 40);
        hex[40] = '\0';
        free(head);
        return strdup(hex);
    }

    free(head);
    return NULL;
}