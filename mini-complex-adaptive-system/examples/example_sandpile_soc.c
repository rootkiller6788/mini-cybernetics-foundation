/**
 * @file example_sandpile_soc.c
 * @brief L6: Self-Organized Criticality -- Sandpile Model
 *
 * Demonstrates Bak-Tang-Wiesenfeld sandpile model: a canonical
 * example of self-organized criticality where avalanche sizes
 * follow power-law distributions without parameter tuning.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/cas_dynamics.h"

int main(void) {
    printf("=== Self-Organized Criticality: Sandpile Model ===\n\n");

    uint32_t grid_sizes[] = {16, 32, 64};
    uint32_t n_sizes = 3;
    uint64_t n_drops = 50000;

    uint32_t si;
    for (si = 0; si < n_sizes; si++) {
        cas_sandpile_t sp;
        cas_sandpile_init(&sp, grid_sizes[si]);
        cas_sandpile_run(&sp, n_drops, NULL);

        double tau = cas_sandpile_powerlaw_exponent(&sp);

        printf("Grid %ux%u: %llu drops, %llu topplings, %llu avalanches\n",
               grid_sizes[si], grid_sizes[si],
               (unsigned long long)n_drops,
               (unsigned long long)sp.total_topplings,
               (unsigned long long)sp.total_avalanches);
        printf("  Power-law exponent tau = %.3f\n", tau);
        printf("  Mean avalanche size = %.1f\n\n",
               sp.total_avalanches > 0
               ? (double)sp.total_topplings / sp.total_avalanches : 0.0);
    }

    printf("Theorem (Bak-Tang-Wiesenfeld 1987): The sandpile self-organizes\n");
    printf("to a critical state with power-law avalanche distribution\n");
    printf("P(s) ~ s^{-tau} where tau ~ 1.1 in 2D, without any fine-tuning.\n");

    return 0;
}
