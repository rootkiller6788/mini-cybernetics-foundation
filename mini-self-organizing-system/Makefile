CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wno-misleading-indentation -O2 -Iinclude
LDFLAGS = -lm
LIB = build/libself_organizing_system.a
SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))
TEST_SRC = tests/test_self_organizing.c
TEST_BIN = build/test_self_organizing
EXAMPLE_SRCS = $(wildcard examples/*.c)
EXAMPLE_BINS = $(patsubst examples/%.c,build/%,$(EXAMPLE_SRCS))
BENCH_SRC = benches/bench_core.c
BENCH_BIN = build/bench_core
DEMO_SRC = demos/demo_overview.c
DEMO_BIN = build/demo_overview

.PHONY: all builddir clean test examples demo bench

all: builddir $(LIB)

builddir:
	@mkdir -p build

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: all
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(TEST_SRC) -Lbuild -lself_organizing_system $(LDFLAGS)
	./$(TEST_BIN)

examples: all $(EXAMPLE_BINS)
	@echo "All examples built."

build/%: examples/%.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lself_organizing_system $(LDFLAGS)

demo: examples
	@echo "=== Running example1_game_of_life ==="
	./build/example1_game_of_life
	@echo ""
	@echo "=== Running example2_sandpile_soc ==="
	./build/example2_sandpile_soc
	@echo ""
	@echo "=== Running example3_turing_pattern ==="
	./build/example3_turing_pattern

bench: all
	$(CC) $(CFLAGS) -o $(BENCH_BIN) $(BENCH_SRC) -Lbuild -lself_organizing_system $(LDFLAGS)
	./$(BENCH_BIN)

clean:
	rm -rf build
