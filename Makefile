CC = gcc
CFLAGS = -Wall -Iinclude

SRC_DIR = src

SRCS = $(wildcard $(SRC_DIR)/*.c)

EXES = $(patsubst $(SRC_DIR)/%.c,%,$(SRCS))

all: $(EXES)

%: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXES)