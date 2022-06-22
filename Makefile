# Time to write a Makefile because I forgot how to write one.
# https://makefiletutorial.com/

# A comment so I can push more things
CC = clang
CFLAGS = -g

TARGET_EXE = stygatore

BUILD_DIR = build
SRC_DIR = src

all:
	$(CC) $(CFLAGS) src/stygatore.cpp -lstdc++ -o $(BUILD_DIR)/$(TARGET_EXE)

.PHONY: clean test

clean:
	rm -rf $(BUILD_DIR)/*

test:
	./$(BUILD_DIR)/$(TARGET_EXE) ./examples/