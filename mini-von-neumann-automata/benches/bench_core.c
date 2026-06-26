#include "vna_core.h"
#include "vna_cellular.h"
#include "vna_rule.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Benchmarks for core CA operations */
int main(void) {
    printf("=== Von Neumann Automata Benchmarks ===\n\n");

    int width = 200, height = 200;
    int steps = 100;

    VnaLattice* lattice = vna_lattice_create(width, height, 2,
                                              VNA_LATTICE_2D_SQUARE);
    VnaNeighborhood* nb = vna_neighborhood_create(VNA_MOORE, 1, 2);
    VnaTransitionRule* rule = vna_rule_game_of_life();

    if (!lattice || !nb || !rule) {
        fprintf(stderr, "Benchmark setup failed\n");
        return 1;
    }

    vna_lattice_randomize(lattice, 0.5, 42);

    printf("Lattice: %dx%d, %d generations\n", width, height, steps);

    clock_t start = clock();
    vna_lattice_evolve_n_steps(lattice, nb, rule, steps);
    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    int64_t cell_updates = (int64_t)width * height * steps;
    double mcups = cell_updates / (elapsed * 1e6);

    printf("Time: %.3f seconds\n", elapsed);
    printf("Cell updates: %lld\n", (long long)cell_updates);
    printf("Performance: %.2f MCUPS (million cell updates/sec)\n", mcups);

    vna_lattice_free(lattice);
    vna_neighborhood_free(nb);
    vna_rule_free(rule);

    printf("\n[Benchmarks complete]\n");
    return 0;
}
