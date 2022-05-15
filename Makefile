##
# Stygatore
#
# @file
# @version 0.1

C = clang
SRC_FILES = code/stygatore.c
OUTPUT_EXE = build/stygatore

all: build build/stygatore

build/stygatore: code/stygatore.c code/stygatore.h code/linux/linux_platform.c code/linux/linux_platform.h
	$(C) -g $(SRC_FILES) -o $(OUTPUT_EXE)

build:
	mkdir build

.PHONY: clean
clean:
	rm -rf build/*

# end
