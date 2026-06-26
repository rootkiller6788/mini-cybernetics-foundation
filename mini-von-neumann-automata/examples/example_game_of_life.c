#include "vna_core.h"
#include "vna_cellular.h"
#include "vna_rule.h"
#include "vna_game_of_life.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * Example: Conway's Game of Life Simulation
 *
 * Demonstrates:
 *   - Lattice creation and initialization
 *   - Canonical pattern insertion (glider, blinker, block, Gosper gun)
 *   - Game of Life rule and neighborhood setup
 *   - Multi-step evolution
 *   - Population tracking and entropy analysis
 *   - ASCII visualization
 * ============================================================================ */

int main(void) {
    printf("=== Conway's Game of Life — Full Simulation ===\n\n");

    int width = 80, height = 40;
    VnaLattice* lattice = vna_gol_create_lattice(width, height);
    VnaNeighborhood* nb = vna_gol_create_neighborhood();
    VnaTransitionRule* rule = vna_gol_create_rule();

    if (!lattice || !nb || !rule) {
        fprintf(stderr, "Failed to initialize Game of Life\n");
        return 1;
    }

    /* Set up an interesting initial configuration */
    printf("Setting up initial configuration...\n");

    /* Insert Gosper glider gun (produces gliders) */
    vna_gol_insert_gosper_gun(lattice, 5, 5);
    printf("  + Gosper Glider Gun at (5, 5)\n");

    /* Insert some still lifes */
    vna_gol_insert_block(lattice, 50, 30);
    printf("  + Block at (50, 30)\n");
    vna_gol_insert_beehive(lattice, 60, 5);
    printf("  + Beehive at (60, 5)\n");

    /* Insert oscillators */
    vna_gol_insert_blinker(lattice, 10, 30);
    printf("  + Blinker at (10, 30)\n");
    vna_gol_insert_toad(lattice, 20, 25);
    printf("  + Toad at (20, 25)\n");

    /* Insert a glider */
    vna_gol_insert_glider(lattice, 50, 10);
    printf("  + Glider at (50, 10)\n");

    /* Insert LWSS */
    vna_gol_insert_lwss(lattice, 60, 15);
    printf("  + LWSS at (60, 15)\n");

    int64_t initial_pop = vna_gol_population(lattice);
    printf("\nInitial population: %lld cells\n\n", (long long)initial_pop);

    /* Run simulation */
    int max_gen = 50;
    printf("Running %d generations...\n", max_gen);
    printf("Legend: # = alive, . = dead\n\n");

    int64_t pop_history[51];
    double entropy_history[51];

    for (int gen = 0; gen <= max_gen; gen++) {
        int64_t pop = vna_gol_population(lattice);
        VnaStateHistogram* hist = vna_histogram_compute(lattice);
        double ent = hist ? hist->shannon_entropy : 0.0;
        vna_histogram_free(hist);

        pop_history[gen] = pop;
        entropy_history[gen] = ent;

        printf("Generation %3d: pop=%5lld  entropy=%.4f  ",
               gen, (long long)pop, ent);

        /* Show a visual snippet (top-left 15x8) */
        if (gen % 10 == 0 || gen == max_gen) {
            printf("\n");
            vna_lattice_print_ascii(lattice, 0, 0, 70, 20);
            printf("\n");
        } else {
            printf("(snippet at gen %% 10 == 0)\n");
        }

        vna_lattice_evolve(lattice, nb, rule);
    }

    /* Summary statistics */
    printf("=== Simulation Complete ===\n");
    printf("Final population: %lld\n", (long long)pop_history[max_gen]);
    printf("Population trend: %lld → %lld\n",
           (long long)pop_history[0], (long long)pop_history[max_gen]);

    double avg_entropy = 0.0;
    for (int i = 0; i <= max_gen; i++) avg_entropy += entropy_history[i];
    avg_entropy /= (max_gen + 1);
    printf("Average entropy: %.4f bits\n", avg_entropy);

    /* Detect gliders */
    int n_gliders = vna_gol_count_gliders(lattice, nb, rule);
    printf("Estimated glider count: %d\n", n_gliders);

    /* Loop lexicon */
    printf("\nKnown pattern catalog (%d patterns):\n", vna_gol_lexicon_size());
    for (int i = 0; i < vna_gol_lexicon_size(); i++) {
        printf("  %-15s — %s\n", vna_gol_lexicon_name(i),
               vna_gol_lexicon_category(i));
    }

    vna_lattice_free(lattice);
    vna_neighborhood_free(nb);
    vna_rule_free(rule);

    printf("\n[Game of Life simulation complete]\n");
    return 0;
}
