CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lm
LIB = build/libwiener.a
SRCS = src/wiener_core.c src/wiener_process.c src/wiener_filter.c src/wiener_feedback.c src/wiener_information.c src/wiener_prediction.c src/wiener_applications.c src/wiener_advanced.c
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))
TEST_SRC = tests/test_wiener.c
TEST_BIN = build/test_wiener
EXAMPLE_SRCS = $(wildcard examples/*.c)
EXAMPLE_BINS = $(patsubst examples/%.c,build/%,$(EXAMPLE_SRCS))

.PHONY: all clean test examples demo

all: builddir $(LIB)

builddir:
	mkdir -p build

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: all
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(TEST_SRC) -Lbuild -lwiener $(LDFLAGS)
	./$(TEST_BIN)

examples: all $(EXAMPLE_BINS)
	@echo "All examples built."

build/%: examples/%.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lwiener $(LDFLAGS)

demo: examples
	@echo "=== example_signal_denoising ==="
	./build/example_signal_denoising
	@echo ""
	@echo "=== example_feedback_regulation ==="
	./build/example_feedback_regulation
	@echo ""
	@echo "=== example_time_series ==="
	./build/example_time_series

clean:
	rm -rf build
