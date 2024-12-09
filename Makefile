CC = gcc
CFLAGS = -Wall -Wextra -I./include -O2 $(shell llvm-config --cflags)
LDFLAGS = $(shell llvm-config --ldflags --libs core executionengine mcjit x86 aarch64)
DEPS = $(wildcard include/*.h)
SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=build/%.o)

# Ensure build directory exists
$(shell mkdir -p build)

# Main target
all: build/arm64_jit

# Linking
build/arm64_jit: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compilation
build/%.o: src/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Test target
test: build/arm64_jit
	for test in tests/test_*.c; do \
		$(CC) -o build/$$(basename $$test .c) $$test $(CFLAGS) $(LDFLAGS) -I./include; \
		./build/$$(basename $$test .c); \
	done

# Clean target
clean:
	rm -rf build/*

.PHONY: all test clean 