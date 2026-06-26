#include "vna_core.h"
#include "vna_cellular.h"
#include "vna_rule.h"
#include "vna_fault_tolerant.h"
#include "vna_game_of_life.h"
#include <stdio.h>
#include <stdlib.h>

/* Demo: interactive overview of von Neumann automata concepts */
int main(void) {
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Von Neumann Automata — Demo Overview   ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* 1. Cellular Automata Basics */
    printf("1. CELLULAR AUTOMATA BASICS\n");
    printf("   von Neumann neighborhood (r=1): %d cells\n",
           vna_von_neumann_count(1));
    printf("   Moore neighborhood (r=1): %d cells\n",
           vna_moore_count(1));
    printf("   Rule space for binary CA with 9 neighbors: 2^512 ≈ 10^154\n");

    /* 2. Game of Life Demo */
    printf("\n2. CONWAY'S GAME OF LIFE\n");
    VnaLattice* gol = vna_gol_create_lattice(20, 10);
    VnaNeighborhood* nb = vna_gol_create_neighborhood();
    VnaTransitionRule* rule = vna_gol_create_rule();

    vna_gol_insert_glider(gol, 2, 2);
    vna_gol_insert_block(gol, 10, 5);

    printf("   Initial state (glider + block):\n");
    vna_lattice_print_ascii(gol, 0, 0, 20, 10);

    printf("\n   After 4 generations (glider period):\n");
    vna_lattice_evolve_n_steps(gol, nb, rule, 4);
    vna_lattice_print_ascii(gol, 0, 0, 20, 10);

    vna_lattice_free(gol);
    vna_neighborhood_free(nb);
    vna_rule_free(rule);

    /* 3. Fault Tolerance */
    printf("\n3. VON NEUMANN'S FAULT-TOLERANT COMPUTING\n");
    double ecrit = vna_critical_error_threshold(2, 1);
    printf("   Error threshold ε_crit = %.4f\n", ecrit);

    printf("   TMR error reduction:\n");
    for (double eps = 0.001; eps <= 0.1; eps *= 10.0) {
        double tmr_err = vna_tmr_error_probability(eps);
        printf("     ε=%.4f → TMR error=%.6f (%.0fx reduction)\n",
               eps, tmr_err, eps / tmr_err);
    }

    /* 4. Game of Life Lexicon */
    printf("\n4. GAME OF LIFE PATTERN LEXICON\n");
    for (int i = 0; i < vna_gol_lexicon_size(); i++) {
        printf("   %-15s  %-10s  (period %d)\n",
               vna_gol_lexicon_name(i),
               vna_gol_lexicon_category(i),
               i < 4 ? 1 : (i < 6 ? 2 : (i < 8 ? 4 : 30)));
    }

    printf("\n[Demonstration complete]\n");
    return 0;
}
