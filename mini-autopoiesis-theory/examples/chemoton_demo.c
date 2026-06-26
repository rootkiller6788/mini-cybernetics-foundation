/**
 * @file chemoton_demo.c
 * @brief Example: Gánti's Chemoton — the minimal chemical autopoietic system.
 *
 * Demonstrates the three coupled subsystems:
 *   1. Metabolic cycle (autocatalytic)
 *   2. Template replication (information preserving)
 *   3. Membrane growth (boundary forming)
 *
 * L6 - Canonical problem: chemoton assembly and steady-state analysis
 * L4 - Verification of Gánti's autopoiesis criteria
 *
 * Reference: Gánti, "The Principles of Life" (1971/2003)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* Type definitions and function declarations (matching chemoton.c) */
typedef struct {
    double metabolites[8];
    int n_metabolites;
    double nutrient;
    double waste;
    double template_concentration;
    double monomers[4];
    int n_monomers;
    double template_polymerase;
    double membrane_precursors;
    double membrane_area;
    double membrane_permeability;
    double encapsulated_volume;
    double metabolic_efficiency;
    double template_replication_rate;
    double membrane_growth_rate;
    double decay_rate;
    double nutrient_influx;
    double total_energy;
    double organization_measure;
} chemoton_t;

void chemoton_init(chemoton_t *che);
void chemoton_integrate_rk4(chemoton_t *che, double dt);
int chemoton_check_autopoiesis(const chemoton_t *che);
int chemoton_find_steady_state(chemoton_t *che, double max_time, double tolerance);
void chemoton_bifurcation_scan(chemoton_t *che, const char *param_name,
                                double param_start, double param_end,
                                int n_points, double *results);
void chemoton_print_state(const chemoton_t *che);

int main(void)
{
    printf("============================================================\n");
    printf("  Gánti Chemoton — Minimal Autopoietic Chemical System\n");
    printf("  Three coupled subsystems: Metabolism + Template + Membrane\n");
    printf("============================================================\n\n");

    /* ==================================================================
     * Part 1: Build and initialize the chemoton
     * ================================================================== */
    printf("[1] Initializing chemoton...\n");
    chemoton_t che;
    chemoton_init(&che);

    /* Customize parameters for a more interesting demonstration */
    che.n_metabolites = 5;
    che.n_monomers = 3;
    che.nutrient = 20.0;
    che.metabolic_efficiency = 1.2;
    che.template_replication_rate = 0.6;
    che.membrane_growth_rate = 0.4;
    che.decay_rate = 0.005;
    che.nutrient_influx = 0.15;

    /* Set initial concentrations explicitly */
    for (int i = 0; i < che.n_metabolites; i++) {
        che.metabolites[i] = 1.0 + 0.2 * i;
    }
    che.template_concentration = 0.2;
    for (int i = 0; i < che.n_monomers; i++) {
        che.monomers[i] = 0.5;
    }
    che.membrane_precursors = 2.0;
    che.membrane_area = 1.5;
    che.total_energy = 5.0;

    printf("  %d metabolites, %d monomers\n", che.n_metabolites, che.n_monomers);
    printf("  Initial state:\n");
    chemoton_print_state(&che);

    /* ==================================================================
     * Part 2: Time evolution to steady state
     * ================================================================== */
    printf("\n[2] Evolving to steady state (50 time units)...\n");

    for (int t = 0; t <= 50; t += 10) {
        for (int step = 0; step < 500; step++) {
            chemoton_integrate_rk4(&che, 0.02);
        }

        int is_apo = chemoton_check_autopoiesis(&che);
        printf("  t=%.0f  energy=%.2f  template=%.3f  membrane_area=%.2f  apo=%s\n",
               (double)(t + 10),
               che.total_energy, che.template_concentration,
               che.membrane_area, is_apo ? "YES" : "NO");
    }

    /* ==================================================================
     * Part 3: Steady-state analysis
     * ================================================================== */
    printf("\n[3] Finding steady state...\n");

    chemoton_t che_ss;
    memcpy(&che_ss, &che, sizeof(chemoton_t));

    int found_ss = chemoton_find_steady_state(&che_ss, 100.0, 1e-5);
    if (found_ss) {
        printf("  Steady state found!\n");
        chemoton_print_state(&che_ss);
    } else {
        printf("  Steady state not reached within max_time\n");
    }

    /* ==================================================================
     * Part 4: Bifurcation analysis (parameter scan)
     * ================================================================== */
    printf("\n[4] Bifurcation analysis: varying nutrient influx...\n");
    printf("  Scanning nutrient_influx from 0.05 to 0.50...\n");

    int n_points = 10;
    double *results = (double *)malloc(n_points * sizeof(double));
    if (results) {
        chemoton_bifurcation_scan(&che, "nutrient_influx", 0.05, 0.50,
                                   n_points, results);
        printf("  Results (organization measure vs influx):\n");
        for (int i = 0; i < n_points; i++) {
            double influx = 0.05 + (0.45 * i / (n_points - 1));
            printf("    influx=%.3f  org=%.3f  %s\n",
                   influx, results[i],
                   results[i] > 0.66 ? "AUTOPOIETIC" : "NON-VIABLE");
        }
        free(results);
    }

    /* ==================================================================
     * Part 5: Final assessment
     * ================================================================== */
    printf("\n[5] Final chemoton assessment:\n");
    int final_apo = chemoton_check_autopoiesis(&che);
    printf("  Autopoietic: %s\n", final_apo ? "YES" : "NO");
    printf("  Metabolic cycle energy: %.2f\n", che.total_energy);
    printf("  Template concentration:  %.4f\n", che.template_concentration);
    printf("  Membrane area:           %.2f\n", che.membrane_area);
    printf("  Organization measure:    %.3f\n", che.organization_measure);

    printf("\n============================================================\n");
    printf("  The chemoton demonstrates that autopoiesis can emerge\n");
    printf("  from stoichiometrically coupled chemical subsystems.\n");
    printf("  Gánti's criteria: metabolism, template, membrane — all\n");
    printf("  three must be present for a minimal living system.\n");
    printf("============================================================\n");

    return 0;
}
