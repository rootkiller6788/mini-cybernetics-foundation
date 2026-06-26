#include "cp_variety.h"
#include "cp_regulator.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Example 2: Ashby's Law of Requisite Variety ? A Regulatory Game
 *
 * Demonstrates Ashby's Law (1956):
 *   "Only variety can destroy variety."
 *
 * We model a regulator facing an unpredictable environment:
 *   - Disturbances come in 4 types (with different probabilities)
 *   - The regulator has N possible responses
 *   - Outcome = f(disturbance, response)
 *
 * Ashby's Law states: V(R) >= V(D) - K
 * If the regulator has fewer distinct responses than the disturbance requires,
 * perfect regulation is impossible.
 *
 * This example shows:
 *   1. A regulator with insufficient variety (fails)
 *   2. A regulator with sufficient variety (succeeds)
 */

int main(void) {
    printf("=== Example 2: Ashby''s Law of Requisite Variety ===\n");
    printf("Ashby (1956): Only variety can destroy variety.\n\n");

    /* Define disturbance ensemble: 4 disturbance types */
    const int N_DISTURBANCES = 4;
    double probs[4] = {0.4, 0.3, 0.2, 0.1};
    DisturbanceEnsemble* de = cp_disturbance_ensemble_create(N_DISTURBANCES);
    for (int i = 0; i < N_DISTURBANCES; i++)
        cp_disturbance_ensemble_set_probability(de, i, probs[i]);
    cp_disturbance_ensemble_normalize(de);

    printf("Disturbance Ensemble:\n");
    printf("  Types: %d\n", N_DISTURBANCES);
    printf("  Probabilities: %.1f, %.1f, %.1f, %.1f\n",
           de->probabilities[0], de->probabilities[1],
           de->probabilities[2], de->probabilities[3]);
    printf("  Entropy H(D) = %.4f bits\n", de->entropy);
    printf("  Variety V(D) = %.4f bits\n", de->variety);

    /* --- Case 1: Insufficient Variety --- */
    printf("\n--- Case 1: Regulator with Insufficient Variety (N_R=1) ---\n");
    AshbyRegulator* reg_weak = cp_ashby_regulator_create(N_DISTURBANCES, 1, 4);
    /* Only one response for all disturbances */
    for (int d = 0; d < N_DISTURBANCES; d++)
        cp_ashby_regulator_set_strategy(reg_weak, d, 0);
    cp_ashby_regulator_evaluate(reg_weak, de);

    printf("V(R) = %.4f bits (only 1 response = 0 bits of variety)\n",
           log2(1.0));
    printf("V(D) = %.4f bits\n", de->variety);
    printf("V(R)/V(D) = %.4f\n", reg_weak->requisite_ratio);
    printf("Ashby Law satisfied: %s\n",
           reg_weak->satisfies_ashby_law ? "YES" : "NO ? regulation may fail");
    cp_ashby_regulator_print(reg_weak);
    cp_ashby_regulator_outcome_variety(reg_weak, de);

    /* --- Case 2: Sufficient Variety --- */
    printf("\n--- Case 2: Regulator with Sufficient Variety (N_R=4) ---\n");
    AshbyRegulator* reg_strong = cp_ashby_regulator_create(N_DISTURBANCES, 4, 16);
    /* Each disturbance gets a distinct response */
    cp_ashby_regulator_set_strategy(reg_strong, 0, 0);  /* D0 ? R0 */
    cp_ashby_regulator_set_strategy(reg_strong, 1, 1);  /* D1 ? R1 */
    cp_ashby_regulator_set_strategy(reg_strong, 2, 2);  /* D2 ? R2 */
    cp_ashby_regulator_set_strategy(reg_strong, 3, 3);  /* D3 ? R3 */
    cp_ashby_regulator_evaluate(reg_strong, de);

    printf("V(R) = %.4f bits (4 distinct responses)\n", log2(4.0));
    printf("V(D) = %.4f bits\n", de->variety);
    printf("V(R)/V(D) = %.4f\n", reg_strong->requisite_ratio);
    printf("Ashby Law satisfied: %s ? perfect regulation possible\n",
           reg_strong->satisfies_ashby_law ? "YES" : "NO");
    cp_ashby_regulator_print(reg_strong);
    cp_ashby_regulator_outcome_variety(reg_strong, de);

    /* --- Information Channel Analysis --- */
    printf("\n--- Control Channel Analysis ---\n");
    ControlChannel* channel = cp_channel_create(4, 4);
    /* Noisy channel: 80% correct, 20% distributed among errors */
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            double p = (i == j) ? 0.70 : 0.10;
            cp_channel_set_transition(channel, i, j, p);
        }
    }
    double capacity = cp_channel_compute_capacity(channel, 100, 1e-8);
    double equivocation = cp_channel_equivocation(channel);
    double error_prob = cp_channel_error_probability(channel);

    printf("Channel capacity:   C = %.4f bits\n", capacity);
    printf("Equivocation:      H(X|Y) = %.4f bits\n", equivocation);
    printf("Error probability: Pe = %.4f\n", error_prob);
    printf("\nWith this channel, Ashby Index = V(R) - V(D) + C = %.4f + %.4f - %.4f = %.4f\n",
           reg_strong->variety_response, capacity, de->variety,
           cp_ashby_index(reg_strong->variety_response, de->variety, capacity));

    /* Cleanup */
    cp_ashby_regulator_free(reg_weak);
    cp_ashby_regulator_free(reg_strong);
    cp_channel_free(channel);
    cp_disturbance_ensemble_free(de);

    printf("\nExample 2 complete.\n");
    return 0;
}
