/* ============================================================================
 * Example 2: Observer and Second-Order Observation
 *
 * Demonstrates observer construction of reality, blind spots,
 * and meta-observation (second-order observation).
 * ============================================================================ */

#include <stdio.h>
#include <math.h>
#include "../include/soc_core.h"
#include "../include/soc_observer.h"

int main(void) {
    printf("============================================\n");
    printf("  Example 2: Observer and Blind Spot\n");
    printf("============================================\n\n");

    /* Create first-order observer with partial view */
    printf("--- First-order observer (Alice) ---\n");
    SOCObserver* observer = soc_observer_create("Alice", SOC_OBSERVER_EXTERNAL, 4, 2);
    if (!observer) { printf("Failed to create observer.\n"); return 1; }

    double H[] = {1.0,0,0,0, 0,1.0,0,0};
    soc_observer_set_observation_matrix(observer, H);

    printf("  World dim: %d, Obs dim: %d\n", observer->world_dim, observer->obs_dim);
    printf("  Blind spot: %.1f%%\n", soc_observer_blind_spot_size(observer) * 100.0);

    double world[] = {1.0, -2.0, 3.0, 4.0};
    double obs_out[2];
    soc_observer_observe(observer, world, obs_out);
    printf("  World: [%.1f, %.1f, %.1f, %.1f]\n", world[0],world[1],world[2],world[3]);
    printf("  Alice sees: [%.1f, %.1f]\n", obs_out[0], obs_out[1]);
    printf("  Alice does NOT see: [%.1f, %.1f]\n\n", world[2], world[3]);

    soc_observer_update_belief(observer, obs_out);
    printf("  Alice's belief: [%.3f, %.3f, %.3f, %.3f]\n",
           observer->belief_state[0], observer->belief_state[1],
           observer->belief_state[2], observer->belief_state[3]);

    /* Self-observation */
    printf("\n--- Self-observation ---\n");
    observer->has_self_observation = true;
    observer->self_observation_gain = 0.7;
    double self_obs[4];
    soc_observer_self_observe(observer, self_obs);
    printf("  Self-observation gain kappa=%.2f\n", observer->self_observation_gain);

    /* Second-order: meta-observer */
    printf("\n--- Meta-observer (Bob) watches Alice ---\n");
    SOCObserver* meta = soc_observer_create("Bob", SOC_OBSERVER_META, 4, 4);
    SOCSecondOrderObservation* soo = soc_soo_create(observer, meta);
    soc_soo_analyze(soo);
    printf("  Bob's blind spot: %.1f%%\n", soc_observer_blind_spot_size(meta) * 100.0);
    printf("  Bob sees Alice's %d distinction(s)\n", soo->n_observed_distinctions);
    soc_soo_print(soo);

    /* Frame relativity */
    printf("\n--- Frame Relativity ---\n");
    SOCReferenceFrame* fa = soc_frame_create(2);
    SOCReferenceFrame* fb = soc_frame_create(2);
    double rot90[] = {0,-1, 1,0};
    soc_frame_set_rotation(fb, rot90);
    printf("  Frame distance: %.4f\n", soc_frame_distance(fa, fb));
    double pt[] = {3,0}, pt_b[2];
    soc_frame_transform(fb, pt, pt_b);
    printf("  [3,0] in A -> [%.1f,%.1f] in B (90 deg rotation)\n", pt_b[0], pt_b[1]);

    soc_frame_free(fa);
    soc_frame_free(fb);
    soc_soo_free(soo);
    soc_observer_free(meta);
    soc_observer_free(observer);

    printf("\n=== Key Insight ===\n");
    printf("Every observation depends on the observer's structure.\n");
    printf("There is no observation without a blind spot.\n");
    printf("Second-order cybernetics studies the observer, not the observed.\n");
    return 0;
}

