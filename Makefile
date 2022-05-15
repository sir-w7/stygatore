##
# Stygatore
#
# @file
# @version 0.1

C = clang
SRC_FILES = code/stygatore.c
OUTPUT_EXE = build/stygatore

build/stygatore: build code/stygatore.c code/linux/linux_platform.c code/linux/linux_platform.h
	$(C) -g $(SRC_FILES) -o $(OUTPUT_EXE)

build:
	mkdir build

.PHONY: clean test
clean:
	rm -rf build/*

test:
	./build/stygatore ./examples/struct.gs ./examples/struct2.gs
# end
