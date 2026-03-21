# git-from-scratch

A minimal Git implementation written in C that replicates core Git plumbing commands from scratch. It supports initialising a repository, hashing files, snapshotting directories as tree objects, creating commits, updating refs, and walking commit history — all stored in a `.mygit/` directory using the same loose object format as real Git.

---

## Table of Contents

- [How It Works](#how-it-works)
- [Project Layout](#project-layout)
- [Prerequisites](#prerequisites)
- [Build](#build)
- [Commands](#commands)
- [Complete Workflow](#complete-workflow)
- [Object Storage Format](#object-storage-format)
- [Internal Architecture](#internal-architecture)

---

## How It Works

mygit models the three core Git data types:

- **Blob** — stores raw file content
- **Tree** — stores a directory snapshot (list of blobs and sub-trees)
- **Commit** — stores a pointer to a tree, author info, and a message

Every object is identified by its SHA-1 hash and stored as a file under `.mygit/objects/<xx>/<38chars>`, where the first two hex characters form the directory name and the remaining 38 form the filename — exactly as real Git does.

---

## Project Layout

```
git-from-scratch/
├── include/          Header files
│   ├── blob.h
│   ├── commit.h
│   ├── hash.h
│   ├── object.h
│   ├── refs.h
│   ├── tree.h
│   └── util.h
├── src/              Source files
│   ├── main.c        CLI entry point and command dispatch
│   ├── blob.c        Blob object creation
│   ├── commit.c      Commit object creation
│   ├── hash.c        SHA-1 hashing and hex conversion
│   ├── object.c      Shared object read/write
│   ├── refs.c        Ref and HEAD management
│   ├── tree.c        Tree object creation (recursive)
│   └── util.c        File I/O utilities
├── Makefile
├── .gitignore
└── README.md
```

---

## Prerequisites

GCC and OpenSSL development headers are required.

```bash
# Ubuntu / Debian
sudo apt install gcc libssl-dev

# macOS
brew install openssl
```

---

## Build

```bash
make
```

This compiles all sources and produces the binary at `build/mygit`.

To clean the build:

```bash
make clean
```

---

## Commands

| Command                                      | Description                                                   |
| -------------------------------------------- | ------------------------------------------------------------- |
| `mygit init`                                 | Initialise a new `.mygit` repository in the current directory |
| `mygit hash-object <file>`                   | Hash a file, store it as a blob, and print its hash           |
| `mygit cat-file <hash>`                      | Print the content of any stored object by its hash            |
| `mygit write-tree`                           | Recursively snapshot the working directory as a tree object   |
| `mygit commit-tree <tree-hash> -m <message>` | Create a commit object pointing at a tree                     |
| `mygit update-ref <refname> <commit-hash>`   | Point a ref (e.g. `refs/heads/main`) to a commit              |
| `mygit log`                                  | Walk and print the full commit history from HEAD              |

---

## Complete Workflow

### Step 1 — Initialise the repository

```bash
./build/mygit init
```

Creates the `.mygit/` directory structure:

```
.mygit/
├── HEAD              → ref: refs/heads/main
├── objects/          Object store (blobs, trees, commits)
└── refs/
    └── heads/        Branch refs
```

---

### Step 2 — Hash a file (optional)

```bash
./build/mygit hash-object test.txt
# 3b18e512dba79e4c8300dd08aeb37f8e728b8dad
```

Reads the file, wraps it in a Git blob header (`blob <size>\0<content>`), computes its SHA-1, and writes it to `.mygit/objects/`. Prints the 40-character hex hash.

---

### Step 3 — Snapshot the working directory

```bash
./build/mygit write-tree
# 692d1af001012b3ed8d35f7079e532eee369e0fd
```

Recursively walks the current directory. For every file it creates a blob object; for every subdirectory it creates a sub-tree. Finally writes a tree object representing the full snapshot and prints its hash.

---

### Step 4 — Create a commit

```bash
./build/mygit commit-tree 692d1af001012b3ed8d35f7079e532eee369e0fd -m "initial commit"
# 4e15f99a935a1787752661d6ea8ba1673693faae
```

Builds a commit object containing the tree hash, author information, and the message. Writes it to the object store and prints the commit hash.

---

### Step 5 — Update the branch ref

```bash
./build/mygit update-ref refs/heads/main 4e15f99a935a1787752661d6ea8ba1673693faae
```

Writes the commit hash into `.mygit/refs/heads/main`, making `main` point to this commit. This is what `HEAD` resolves to via the symbolic ref set during `init`.

---

### Step 6 — View commit history

```bash
./build/mygit log
```

Resolves `HEAD` → `refs/heads/main` → commit hash, reads the commit object, prints it, then follows any `parent` pointer to walk the full history.

Example output:

```
commit 4e15f99a935a1787752661d6ea8ba1673693faae
tree 692d1af001012b3ed8d35f7079e532eee369e0fd
author you <you@example.com>

initial commit
```

---

### Step 7 — Inspect any object

```bash
./build/mygit cat-file 4e15f99a935a1787752661d6ea8ba1673693faae
```

Reads the raw object file, strips the header, and prints the content. Works for blobs, trees, and commits.

---

## Object Storage Format

Every object is stored as:

```
<type> <size>\0<content>
```

The entire thing (header + content) is SHA-1 hashed. The hash determines the storage path:

```
.mygit/objects/<first-2-hex-chars>/<remaining-38-hex-chars>
```

Tree objects use the binary Git format per entry:

```
<octal-mode> <name>\0<20-byte-raw-hash>
```

---

## Internal Architecture

```
CLI (main.c)
    │
    ├── blob.c        hash_blob_file()
    ├── tree.c        write_tree_dir() → write_tree()
    ├── commit.c      commit_tree()
    ├── refs.c        update_ref() / read_ref() / resolve_head()
    │
    └── object.c      write_object() / read_object()   ← shared by all
            │
            └── hash.c        sha1_hash() / hash_to_hex() / hex_to_hash()
            └── util.c        read_file() / write_file() / create_dir()
```

All object types (blob, tree, commit) go through the shared `write_object()` in `object.c`, which handles header construction, SHA-1 hashing, and writing to the correct path under `.mygit/objects/`.