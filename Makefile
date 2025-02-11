# Makefile for building libvcparser.so
# This Makefile compiles VCParser.c and LinkedListAPI.c using the required flags
# and produces the shared library (libvcparser.so) in the bin directory.

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -fPIC -g
LDFLAGS = -shared

# Source files (explicitly listed)
SRC = src/VCParser.c src/LinkedListAPI.c

# Object files corresponding to the source files
OBJ = src/VCParser.o src/LinkedListAPI.o

# Output shared library name and target bin directory
TARGET = libvcparser.so
BIN_DIR = bin

# Default target to build the shared library
parser: $(OBJ)
	@echo "Linking object files to create $(TARGET)..."
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJ)
	@echo "Creating directory $(BIN_DIR) if needed..."
	@mkdir -p $(BIN_DIR)
	@echo "Moving $(TARGET) to $(BIN_DIR)..."
	mv $(TARGET) $(BIN_DIR)/

# Compile VCParser.c into an object file.
src/VCParser.o: src/VCParser.c
	@echo "Compiling VCParser.c..."
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

# Compile LinkedListAPI.c into an object file.
src/LinkedListAPI.o: src/LinkedListAPI.c
	@echo "Compiling LinkedListAPI.c..."
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

# Clean up all generated files.
clean:
	@echo "Cleaning up object files and shared library..."
	rm -f $(OBJ) $(BIN_DIR)/$(TARGET) $(TARGET)
