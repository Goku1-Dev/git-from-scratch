CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude
TARGET  = build/mygit
LIBS    = -lssl -lcrypto

SRC = src/main.c   \
	src/util.c   \
	src/hash.c   \
	src/object.c \
	src/blob.c   \
	src/tree.c   \
	src/commit.c \
	src/refs.c

all:
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -rf build