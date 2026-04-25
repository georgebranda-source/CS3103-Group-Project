CC = gcc
CFLAGS = -Wall -Iinclude

SRC_DIR = src
OBJ_DIR = bin

SRCS = $(wildcard $(SRC_DIR)/*.c)

OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

TARGET = program

all:
	@echo "Run 'make problemN' where N is the problem number."

# Build problemN executable from problemN.c + helpers.c
problem%: $(OBJ_DIR)/problem%.o $(OBJ_DIR)/helpers.o | $(OBJ_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Compile each .c file into an object file in bin
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create bin if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

#Remove all that stuff
clean:
	rm -rf $(OBJ_DIR) $(TARGET) problem*