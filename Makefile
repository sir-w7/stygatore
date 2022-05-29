# Time to write a Makefile because I forgot how to write one.
# https://makefiletutorial.com/

CC = clang
CFLAGS = -g -MMD -MP

TARGET_EXE = stygatore

BUILD_DIR = build
SRC_DIR = src

SRCS = $(wildcard src/*.c src/linux/*.c)
#OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SRCS)))))
OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS:$(SRC_DIR)/%.c=%.o))
DEPS = $(OBJS:.o=.d)

$(BUILD_DIR)/$(TARGET_EXE): $(OBJS)
	@$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "  LINK	$^ -> $@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "  CC	$< -> $@"

.PHONY: clean

clean:
	rm -rf $(BUILD_DIR)/*

-include $(DEPS)
