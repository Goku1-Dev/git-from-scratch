#ifndef REFS_H
#define REFS_H

/* Write a commit hash (hex) to a named ref, e.g. "refs/heads/main". */
int update_ref(const char *refname, const char *hex);

/* Read the commit hash stored in a ref into hex_out (must be ≥41 bytes). */
int read_ref(const char *refname, char *hex_out);

/* Resolve HEAD -> ref -> commit hash.  Returns heap string or NULL. */
char *resolve_head(void);

#endif