# Mini-Autopoiesis-Theory Makefile
# Builds the autopoiesis theory library, tests, and examples.
# Reference: Maturana & Varela, Gánti, Eigen, Kauffman.

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
CFLAGS += -Iinclude
LDFLAGS = -lm -Wl,--stack,16777216

# Source files
SRC = src/autopoiesis.c \
      src/reaction_network.c \
      src/stoichiometry.c \
      src/organizational_closure.c \
      src/autocatalytic_set.c \
      src/structural_coupling.c \
      src/chemoton.c \
      src/hypercycle.c

# Test files
TEST_SRC = tests/test_autopoiesis.c \
           tests/test_network.c \
           tests/test_closure.c \
           tests/test_models.c

# Example files
EXAMPLE_SRC = examples/minimal_cell.c \
              examples/chemoton_demo.c \
              examples/hypercycle_demo.c \
              examples/social_autopoiesis.c

# Object files
OBJ = $(SRC:.c=.o)

# Test and example executables
TEST_BIN = $(TEST_SRC:.c=.exe)
EXAMPLE_BIN = $(EXAMPLE_SRC:.c=.exe)

# ==========================================================================
# Primary targets
# ==========================================================================

.PHONY: all clean test examples docs

all: libautopoiesis.a examples

# Build static library
libautopoiesis.a: $(OBJ)
	$(AR) rcs $@ $(OBJ)

# Build tests
test: libautopoiesis.a $(TEST_BIN)
	@rc=0; \
	for t in $(TEST_BIN); do \
		echo "--- $$t ---"; \
		./$$t || rc=1; \
	done; \
	if [ $$rc -eq 0 ]; then echo "All tests passed"; \
	else echo "FAILED"; exit 1; fi

# Build examples
examples: libautopoiesis.a $(EXAMPLE_BIN)

# Run examples
run-examples: examples
	@for e in $(EXAMPLE_BIN); do \
		if [ -f "$$e" ]; then \
			echo "=== $$e ==="; ./$$e; echo ""; \
		fi; \
	done

# ==========================================================================
# Compilation rules
# ==========================================================================

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

tests/%.exe: tests/%.c libautopoiesis.a
	$(CC) $(CFLAGS) $< -L. -lautopoiesis $(LDFLAGS) -o $@

examples/%.exe: examples/%.c libautopoiesis.a
	$(CC) $(CFLAGS) $< -L. -lautopoiesis $(LDFLAGS) -o $@

# ==========================================================================
# Documentation targets
# ==========================================================================

docs:
	@echo "Documentation files in docs/"
	@echo "  knowledge-graph.md   - Knowledge coverage by level"
	@echo "  coverage-report.md   - Detailed coverage assessment"
	@echo "  gap-report.md        - Missing knowledge points"
	@echo "  course-alignment.md  - Nine-school curriculum mapping"
	@echo "  course-tree.md       - Prerequisite dependency tree"
	@cat docs/knowledge-graph.md 2>/dev/null || echo "(docs/ not yet built)"

# ==========================================================================
# Utility
# ==========================================================================

clean:
	rm -f src/*.o
	rm -f tests/*.exe
	rm -f examples/*.exe
	rm -f libautopoiesis.a

# Quick line count check
count:
	@echo "include/ lines: $$(find include -name '*.h' -exec cat {} + | wc -l)"
	@echo "src/ lines:     $$(find src -name '*.c' -exec cat {} + | wc -l)"
	@echo "total:          $$(find include src -name '*.h' -o -name '*.c' | xargs cat | wc -l)"

# Safety scan (per SKILL.md §10)
safety:
	@echo "=== Filler detection ==="
	@grep -rn "_fn[0-9]" --include="*.c" --include="*.h" . || true
	@grep -rn "_aux[0-9]" --include="*.c" --include="*.h" . || true
	@grep -rn "_ext[0-9][^a-z]" --include="*.c" --include="*.h" . || true
	@grep -rn "algorithm variant" --include="*.c" . || true
	@grep -rn "auxiliary function" --include="*.c" . || true
	@grep -rn "extension point" --include="*.c" . || true
	@grep -rn "Module extension.*line" --include="*.c" . || true
	@grep -rn "supplemental assert" --include="*.c" . || true
	@echo "=== TODO/FIXME/stub/placeholder ==="
	@grep -rn "TODO\|FIXME\|stub\|placeholder" --include="*.c" --include="*.h" . || true
	@echo "=== Stub detection (<3 line functions) ==="
	@grep -rn "return\|;}" --include="*.c" . | head -20 || true
	@echo "=== Empty file detection (<200 bytes) ==="
	@find . -name "*.c" -size -200c -exec echo "SUSPECT: {}" \; || true
	@echo "=== Safety scan complete ==="
