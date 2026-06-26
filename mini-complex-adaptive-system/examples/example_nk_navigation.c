/**
 * @file example_nk_navigation.c
 * @brief L6: NK Landscape Navigation Example
 *
 * Demonstrates adaptive walk on Kauffman's NK fitness landscape.
 * Shows how ruggedness (K) affects adaptation success.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/cas_landscape.h"

int main(void) {
    printf("=== NK Landscape Navigation ===\n\n");

    uint32_t N = 16;
    uint32_t K_vals[] = {0, 2, 4, 8, 15};
    uint32_t n_vals = 5;
    uint32_t walks_per_K = 20;
    uint32_t max_steps = 100;

    uint32_t k_idx;
    for (k_idx = 0; k_idx < n_vals; k_idx++) {
        uint32_t K = K_vals[k_idx];
        cas_nk_landscape_t land;
        cas_nk_init(&land, N, K, (uint64_t)(k_idx * 1000 + 42));

        double total_peak = 0.0;
        double total_steps = 0.0;
        uint32_t w;

        for (w = 0; w < walks_per_K; w++) {
            uint8_t genotype[4];
            cas_nk_random_genotype(&land, genotype, NULL);

            cas_landscape_point_t pt;
            memcpy(pt.genotype, genotype, 4);
            cas_nk_evaluate_full(&land, &pt);

            double start_fit = pt.fitness;
            uint32_t steps = 0;

            while (!pt.is_local_optimum && steps < max_steps) {
                cas_nk_adaptive_walk_step(&land, &pt);
                steps++;
            }

            total_peak += pt.fitness;
            total_steps += steps;
        }

        double avg_peak = total_peak / walks_per_K;
        double avg_steps = total_steps / walks_per_K;
        double rho = cas_nk_fitness_correlation(N, K, 1);

        printf("K=%2u | rho(1)=%.3f | avg_steps=%.1f | avg_peak=%.4f\n",
               K, rho, avg_steps, avg_peak);
    }

    printf("\nComplexity Catastrophe: As K increases, ");
    printf("adaptive walks become shorter (trapped sooner)\n");
    printf("and attain lower peak fitness.\n");

    return 0;
}
