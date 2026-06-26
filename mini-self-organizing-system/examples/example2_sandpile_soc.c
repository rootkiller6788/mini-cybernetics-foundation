#include "sos_criticality.h"
#include "sos_cellular.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

int main(void) {
    srand((unsigned)time(NULL));
    printf("=== Sandpile Model — Self-Organized Criticality ===\n\n");
    printf("Bak, Tang & Wiesenfeld (1987) discovered that driven dissipative\n");
    printf("systems naturally evolve to a critical state where avalanches of\n");
    printf("all sizes occur, producing power-law distributions and 1/f noise.\n\n");
    printf("This is Self-Organized Criticality (SOC): the system tunes itself\n");
    printf("to the critical point without any external parameter adjustment.\n\n");

    int width = 15, height = 15, threshold = 4;
    SandpileModel* sp = sp_create(width, height, threshold);
    printf("Sandpile: %d x %d, toppling threshold = %d\n", width, height, threshold);

    /* Drive the system by adding grains randomly */
    int n_grains = 5000;
    printf("Adding %d grains of sand randomly...\n", n_grains);
    int total_toppled = sp_add_random_grains(sp, n_grains);

    printf("Total grains: %lld\n", sp_total_grains(sp));
    printf("Total topplings: %d\n", total_toppled);
    printf("Avalanches recorded: %lld\n", sp->total_avalanches);
    printf("Average slope: %.3f\n", sp_average_slope(sp));

    /* Analyze avalanche size distribution */
    int n_bins = 10;
    sp_compute_size_distribution(sp, 1000, n_bins);

    printf("\nAvalanche Size Distribution (log-binned):\n");
    printf("Size range          | Frequency\n");
    printf("--------------------|----------\n");
    double log_min = 0.0;
    long long max_s = 0;
    for (int i = 0; i < sp->aval_history_len; i++)
        if (sp->avalanche_sizes[i] > max_s) max_s = sp->avalanche_sizes[i];
    double log_max = log((double)(max_s + 1));
    double dlog = (log_max - log_min) / n_bins;

    for (int b = 0; b < n_bins; b++) {
        double low = exp(log_min + b*dlog);
        double high = exp(log_min + (b+1)*dlog);
        printf("[%6.0f - %6.0f]  | %.4f\n", low, high, sp->aval_size_dist[b]);
    }

    /* Test power-law fit */
    double p_value;
    int is_pl = soc_test_powerlaw((const double*)sp->avalanche_sizes,
                                   sp->aval_history_len, 1.0, &p_value);
    printf("\nPower-law test: %s (p-value=%.4f)\n",
           is_pl ? "PLAUSIBLE" : "NOT POWER-LAW", p_value);

    /* Compute 1/f noise exponent from the avalanche time series */
    double ts[128];
    for (int i = 0; i < 128 && i < sp->aval_history_len; i++)
        ts[i] = (double)sp->avalanche_sizes[i];
    double alpha = soc_noise_exponent(ts, 128, 1.0);
    printf("1/f noise exponent alpha: %.3f (SOC predicts alpha ~ 1)\n", alpha);

    printf("\nKey insight: The sandpile self-organizes to a critical state.\n");
    printf("Perturbations cause avalanches of all sizes following P(s) ~ s^{-tau}.\n");
    printf("No external tuning is needed — criticality emerges naturally.\n");

    sp_free(sp);
    return 0;
}
