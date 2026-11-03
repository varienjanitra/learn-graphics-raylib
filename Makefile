CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./vendor/raylib/include
LDFLAGS = -L./vendor/raylib/lib -L./lib -lraylib -lGL -lX11 -lm -lpthread -ldl -lrt \
          -Wl,-rpath,'$$ORIGIN/../vendor/raylib/lib:$$ORIGIN/../lib:$$ORIGIN/vendor/raylib/lib:$$ORIGIN/lib'

SRC_DIR = lessons
BIN_DIR = bin

SRCS = $(wildcard $(SRC_DIR)/*.c)
BINS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(SRCS))

# Default lesson
LESSON ?= lesson01_primitives

all: $(BINS)

$(BIN_DIR)/%: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Run with: `make run` or `make run LESSON=lesson02_transforms`
run: $(BIN_DIR)/$(LESSON)
	./$<

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean run
