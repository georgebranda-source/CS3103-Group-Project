CC = gcc
CFLAGS = -Wall -Iinclude

SRC_DIR = src
OBJ_DIR = bin

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

all:
    @echo "Please specify a problem and pass as argument" && false

problem%: $(OBJ_DIR)/problem%.o $(OBJ_DIR)/helpers.o | $(OBJ_DIR)
    $(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
    $(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
    mkdir -p $(OBJ_DIR)

clean:
    rm -rf $(OBJ_DIR) problem*
