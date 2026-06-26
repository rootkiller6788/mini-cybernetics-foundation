CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lm
LIB = build/libcontrol_philosophy.a
SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))
TEST_SRC = tests/test_control_philosophy.c
TEST_BIN = build/test_control_philosophy
EXAMPLE_SRCS = $(wildcard examples/*.c)
EXAMPLE_BINS = $(patsubst examples/%.c,build/%,$(EXAMPLE_SRCS))

.PHONY: all builddir clean test examples demo

all: builddir $(LIB)

builddir:
	@mkdir -p build

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: all
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(TEST_SRC) -Lbuild -lcontrol_philosophy $(LDFLAGS)
	./$(TEST_BIN)

examples: all $(EXAMPLE_BINS)
	@echo "All examples built."

build/%: examples/%.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lcontrol_philosophy $(LDFLAGS)

demo: examples
	@echo "=== Running example1_feedback_loop ==="
	./build/example1_feedback_loop
	@echo ""
	@echo "=== Running example2_requisite_variety ==="
	./build/example2_requisite_variety
	@echo ""
	@echo "=== Running example3_hierarchical_control ==="
	./build/example3_hierarchical_control

clean:
	rm -rf build
