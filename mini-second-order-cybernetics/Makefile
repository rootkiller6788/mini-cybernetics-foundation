CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lm
LIB = build/libsecond_order_cybernetics.a
SRCS = $(wildcard src/*.c)
SRCS := $(filter-out src/second_order_cybernetics.lean,$(SRCS))
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))
TEST_SRC = tests/test_second_order.c
TEST_BIN = build/test_second_order
EXAMPLE_SRCS = $(wildcard examples/*.c)
EXAMPLE_BINS = $(patsubst examples/%.c,build/%,$(EXAMPLE_SRCS))
DEMO_SRC = demos/demo_overview.c
DEMO_BIN = build/demo_overview
BENCH_SRC = benches/bench_core.c
BENCH_BIN = build/bench_core

.PHONY: all builddir clean test examples demo bench

all: builddir $(LIB)

builddir:
	@mkdir -p build

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: all
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(TEST_SRC) -Lbuild -lsecond_order_cybernetics $(LDFLAGS)
	-./$(TEST_BIN).exe

examples: all $(EXAMPLE_BINS)
	@echo "All examples built."

build/%: examples/%.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lsecond_order_cybernetics $(LDFLAGS)

demo: examples
	@echo "=== Running demo_overview ==="
	$(CC) $(CFLAGS) -o $(DEMO_BIN) $(DEMO_SRC) -Lbuild -lsecond_order_cybernetics $(LDFLAGS)
	./$(DEMO_BIN)

bench: all
	$(CC) $(CFLAGS) -o $(BENCH_BIN) $(BENCH_SRC) -Lbuild -lsecond_order_cybernetics $(LDFLAGS)
	./$(BENCH_BIN)

clean:
	rm -rf build

