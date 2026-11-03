CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I../../vendor/raylib/include -O2
LDFLAGS = -L../../vendor/raylib/lib -L../../lib -lraylib -lGL -lX11 -lm -lpthread -ldl -lrt \
          -Wl,-rpath,'$$ORIGIN/../../vendor/raylib/lib:$$ORIGIN/../../lib'

TARGET = perf_demo
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
