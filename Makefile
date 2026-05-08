CC = gcc

CFLAGS = \
	-D_POSIX_C_SOURCE=200809L \
	-Wall \
	-Wextra \
	-std=c11 \
	-Iinclude

LIB_SRC = \
	$(wildcard src/core/*.c) \
	$(wildcard src/lba/*.c)

TEST_SRC = \
	$(wildcard tests/*.c)

SRC = \
	$(LIB_SRC) \
	$(TEST_SRC)

OBJ = \
	$(SRC:.c=.o)

BIN = bin/test

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p bin
	$(CC) $(OBJ) -o $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(BIN)

clean:
	find src tests -name "*.o" -delete
	rm -rf bin storage

.PHONY: all run clean
