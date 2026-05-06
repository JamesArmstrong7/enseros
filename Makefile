CC = gcc
CFLAGS = -Wall -Wextra -std=c11

SRC = $(wildcard core/*.c) $(wildcard test/*.c)
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
	rm -f core/*.o test/*.o $(BIN)
