#include "homeostat_model.h"
#include "ashby_homeostasis.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void) {
    printf("=== Ashby Homeostat Simulation ===\n\n");
    printf("This example demonstrates a 4-unit homeostat as described\n");
    printf("in Ashby's 'Design for a Brain' (1952/1960).\n");
    printf("Each unit is an electromechanical assembly with a magnet,\n");
    printf("coil, vane (needle), and uniselector step-function mechanism.\n\n");

    srand(42);
    HomeostatSystem* hs = homeostat_system_create(4, 25);
    double coupling[16] = {0};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (i != j) coupling[i*4+j] = -0.02;
    homeostat_system_set_connectivity(hs, coupling, 4);

    printf("Initial state (t=0):\n");
    homeostat_system_print(hs);

    printf("\nApplying disturbance to unit 0 (impulse = 2.0)...\n");
    homeostat_system_apply_disturbance(hs, 0, 2.0);

    printf("\nSimulating for 5 seconds with dt=0.01...\n");
    double dt = 0.01;
    int report_interval = 50;
    for (int step = 0; step < 500; step++) {
        homeostat_system_step(hs, dt);
        if (hs->is_converged && hs->convergence_time > 0.0) {
            if (step % report_interval == 0) {
                printf("[t=%.2f] Converged! Stability index=%.3f, Steps=%d\n",
                       hs->time, homeostat_system_stability_index(hs),
                       hs->total_steps_taken);
                break;
            }
        }
        if (step % (report_interval*2) == 0 && !hs->is_converged) {
            double si = homeostat_system_stability_index(hs);
            printf("[t=%.2f] Stability=%.3f, Energy=%.4f, Steps=%d\n",
                   hs->time, si, hs->total_energy, hs->total_steps_taken);
            if (si < 0.3) {
                homeostat_system_trigger_search(hs);
                printf("  -> Triggered parameter search (step-function)\n");
            }
        }
    }

    printf("\nFinal state:\n");
    homeostat_system_print(hs);
    homeostat_system_free(hs);
    printf("\nDemonstration complete.\n");
    return 0;
}