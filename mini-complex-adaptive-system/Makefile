CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g -Iinclude
LDFLAGS = -lm

SRC = src/cas_agent.c src/cas_landscape.c src/cas_network.c src/cas_dynamics.c src/cas_evolution.c src/cas_emergence.c
OBJ = $(SRC:.c=.o)
HEADERS = include/cas_agent.h include/cas_landscape.h include/cas_network.h include/cas_dynamics.h include/cas_evolution.h include/cas_emergence.h

TEST_BIN = test_cas

EX_NK = examples/example_nk_navigation
EX_EF = examples/example_el_farol
EX_SS = examples/example_sandpile_soc
EX_GA = examples/example_ga_optimization
DEMO = demos/demo_cas
BENCH = benches/bench_cas

.PHONY: all clean test examples demo bench

all: $(OBJ)
	@echo "All source files compiled. Run make test for tests."

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_BIN): $(OBJ) tests/test_cas.c
	$(CC) $(CFLAGS) -o $@ $(OBJ) tests/test_cas.c $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(EX_NK): $(OBJ) examples/example_nk_navigation.c
	$(CC) $(CFLAGS) -o $@ $(OBJ) examples/example_nk_navigation.c $(LDFLAGS)

$(EX_EF): examples/example_el_farol.c
	$(CC) $(CFLAGS) -o $@ examples/example_el_farol.c $(LDFLAGS)

$(EX_SS): $(OBJ) examples/example_sandpile_soc.c
	$(CC) $(CFLAGS) -o $@ $(OBJ) examples/example_sandpile_soc.c $(LDFLAGS)

$(EX_GA): $(OBJ) examples/example_ga_optimization.c
	$(CC) $(CFLAGS) -o $@ $(OBJ) examples/example_ga_optimization.c $(LDFLAGS)

examples: $(EX_NK) $(EX_EF) $(EX_SS) $(EX_GA)
	@echo "=== NK Navigation ===" && ./$(EX_NK)
	@echo "=== El Farol ===" && ./$(EX_EF)
	@echo "=== Sandpile SOC ===" && ./$(EX_SS)
	@echo "=== GA Optimization ===" && ./$(EX_GA)

$(DEMO): $(OBJ) demos/demo_cas.c
	$(CC) $(CFLAGS) -o $@ $(OBJ) demos/demo_cas.c $(LDFLAGS)

demo: $(DEMO)
	./$(DEMO)

$(BENCH): $(OBJ) benches/bench_cas.c
	$(CC) $(CFLAGS) -o $@ $(OBJ) benches/bench_cas.c $(LDFLAGS)

bench: $(BENCH)
	./$(BENCH)

clean:
	rm -f src/*.o $(TEST_BIN) $(EX_NK) $(EX_EF) $(EX_SS) $(EX_GA) $(DEMO) $(BENCH)
