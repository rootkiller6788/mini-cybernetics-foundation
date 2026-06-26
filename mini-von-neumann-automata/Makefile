CC = gcc
CFLAGS = -std=c11 -Wall -Wno-unused-parameter -Wno-unused-function -O2 -Iinclude
LDFLAGS = -lm
LIB = build/libvna.a
SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))
TEST_SRC = tests/test_vna_core.c
TEST_BIN = build/test_vna_core
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
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(TEST_SRC) -Lbuild -lvna $(LDFLAGS)
	./$(TEST_BIN)

examples: all $(EXAMPLE_BINS)
	@echo "All examples built."

build/%: examples/%.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lvna $(LDFLAGS)

build/example_traffic_model: examples/example_traffic_model.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lvna $(LDFLAGS)

demo: examples
	@echo "=== Running example_game_of_life ==="
	./build/example_game_of_life
	@echo ""
	@echo "=== Running example_error_correction ==="
	./build/example_error_correction
	@echo ""
	@echo "=== Running example_self_replication ==="
	./build/example_self_replication
	@echo ""
	@echo "=== Running example_traffic_model ==="
	./build/example_traffic_model

clean:
	rm -rf build
