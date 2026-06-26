/**
 * @file hypercycle_demo.c
 * @brief Example: Eigen's Hypercycle — catalytic cooperation and error threshold.
 *
 * Demonstrates:
 *   - Hypercycle dynamics for n = 2, 3, 4, 5 species
 *   - Fixed-point stability analysis
 *   - Eigen's error catastrophe threshold
 *   - Quasispecies distribution computation
 *
 * L6 - Canonical problem: hypercycle cooperative stability
 * L8 - Advanced: error threshold, quasispecies
 *
 * Reference: Eigen & Schuster, "The Hypercycle" (1979)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define HC_MAX_SPECIES 20

typedef struct {
    int n_species;
    double concentrations[HC_MAX_SPECIES];
    double rate_constants[HC_MAX_SPECIES];
    double decay_rates[HC_MAX_SPECIES];
    double dilution_flow;
    double total_concentration;
    double fitness[HC_MAX_SPECIES];
    double mean_fitness;
    int is_quasispecies;
} hypercycle_t;

void hypercycle_init(hypercycle_t *hc, int n_species);
void hypercycle_integrate_rk4(hypercycle_t *hc, double dt);
void hypercycle_ode(const hypercycle_t *hc, double *dxdt);
int hypercycle_find_fixed_point(const hypercycle_t *hc,
                                 double *fixed_point, int *is_stable);
void hypercycle_error_threshold(int n_components, double fitness_ratio,
                                 double *q_min, double *threshold);
void hypercycle_compute_quasispecies(hypercycle_t *hc);
void hypercycle_print_state(const hypercycle_t *hc);

int main(void)
{
    printf("============================================================\n");
    printf("  Eigen Hypercycle — Catalytic Cooperation & Error Threshold\n");
    printf("============================================================\n\n");

    /* ==================================================================
     * Part 1: Hypercycle stability for different sizes
     * ================================================================== */
    printf("[1] Hypercycle fixed-point stability vs size:\n");
    printf("    (Eigen's theorem: stable for n ≤ 4, unstable for n ≥ 5)\n\n");

    for (int n = 2; n <= 6; n++) {
        hypercycle_t hc;
        hypercycle_init(&hc, n);

        double fp[HC_MAX_SPECIES];
        int is_stable;
        hypercycle_find_fixed_point(&hc, fp, &is_stable);

        printf("    n = %d: ", n);
        printf("x*_i = %.3f  ", fp[0]);
        printf("stability = %s\n", is_stable ? "STABLE" : "UNSTABLE");
    }

    /* ==================================================================
     * Part 2: Time evolution of n=4 hypercycle
     * ================================================================== */
    printf("\n[2] Simulating n=4 hypercycle dynamics...\n\n");

    hypercycle_t hc4;
    hypercycle_init(&hc4, 4);

    /* Set different initial concentrations */
    hc4.concentrations[0] = 0.1;
    hc4.concentrations[1] = 0.3;
    hc4.concentrations[2] = 0.4;
    hc4.concentrations[3] = 0.2;
    hc4.total_concentration = 1.0;

    printf("    t=0:    ");
    for (int i = 0; i < 4; i++) printf("x%d=%.3f ", i, hc4.concentrations[i]);
    printf("\n");

    for (int t = 1; t <= 5; t++) {
        for (int step = 0; step < 20; step++) {
            hypercycle_integrate_rk4(&hc4, 0.05);
        }
        printf("    t=%.0f:   ", (double)t);
        for (int i = 0; i < 4; i++) printf("x%d=%.3f ", i, hc4.concentrations[i]);
        printf(" Σ=%.3f\n", hc4.total_concentration);
    }

    printf("    (Concentrations approach symmetric equilibrium x_i = 0.25)\n");

    /* ==================================================================
     * Part 3: Error threshold analysis
     * ================================================================== */
    printf("\n[3] Eigen Error Threshold Analysis:\n\n");

    int genome_lengths[] = {10, 50, 100, 500, 1000};
    for (int g = 0; g < 5; g++) {
        int L = genome_lengths[g];
        double sigma = 100.0; /* Master fitness advantage */

        double q_min, error_threshold;
        hypercycle_error_threshold(L, sigma, &q_min, &error_threshold);

        printf("    Genome length L = %4d:  q_min = %.6f  error_max = %.6f (%.4f%%)\n",
               L, q_min, error_threshold, error_threshold * 100.0);
    }
    printf("\n    Eigen's paradox: longer genomes require higher replication fidelity.\n");
    printf("    RNA virus: L≈10^4, but error rate ≈10^-4 → near error threshold.\n");

    /* ==================================================================
     * Part 4: Quasispecies distribution
     * ================================================================== */
    printf("\n[4] Quasispecies equilibrium distribution:\n\n");

    hypercycle_t hc_qs;
    hypercycle_init(&hc_qs, 5);

    /* Set master (species 0) to have high fitness */
    hc_qs.rate_constants[0] = 10.0;  /* Master */
    for (int i = 1; i < 5; i++) {
        hc_qs.rate_constants[i] = 1.0; /* Mutants */
    }

    hypercycle_compute_quasispecies(&hc_qs);

    printf("    Quasispecies distribution:\n");
    for (int i = 0; i < hc_qs.n_species; i++) {
        printf("    species[%d]: x=%.4f  fitness=%.1f\n",
               i, hc_qs.concentrations[i], hc_qs.fitness[i]);
    }
    printf("    Master (species 0) dominates: %.1f%%\n",
           hc_qs.concentrations[0] / hc_qs.total_concentration * 100.0);

    printf("\n============================================================\n");
    printf("  The hypercycle model shows how catalytic cooperation can\n");
    printf("  stabilize molecular information against degradation.\n");
    printf("  Key insight: hypercycles enable the transition from\n");
    printf("  chemical evolution to biological evolution.\n");
    printf("============================================================\n");

    return 0;
}
