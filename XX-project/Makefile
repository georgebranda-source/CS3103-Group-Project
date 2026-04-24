# Compiler and flags
CC = gcc
CFLAGS = -Wall -Iinclude

# Directories
SRC_DIR = src
OBJ_DIR = bin

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Object files (src/foo.c -> bin/foo.o)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Final executable name for "all"
TARGET = program

# Default target: build everything into one program
all: $(TARGET)

# Link all object files into one executable in root
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Pattern rule: build problemN executable from problemN.c + helpers.c
problem%: $(OBJ_DIR)/problem%.o $(OBJ_DIR)/helpers.o | $(OBJ_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Compile each .c file into an object file in bin/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure bin directory exists
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean up: remove bin folder and executables in root
clean:
	rm -rf $(OBJ_DIR) $(TARGET) problem*

