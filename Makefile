CC = gcc

CFLAGS = -D_POSIX_C_SOURCE=200809L \
	 -Wall -Wextra -std=c11 -Iinclude

SRC = $(wildcard src/core/*.c) \
      $(wildcard src/lba/*.c) \
      $(wildcard src/cli/*.c) \
      $(wildcard tests/*.c)

OBJ = $(SRC:.c=.o)

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
	rm -f $(BIN)
