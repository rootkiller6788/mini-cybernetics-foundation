/**
 * @file social_autopoiesis.c
 * @brief Example: Social Autopoiesis — applying autopoiesis theory to social systems.
 *
 * Demonstrates Luhmann's theory of social autopoiesis:
 *   - Communication events as the components of social systems
 *   - Structural coupling between individual (psychological) and social systems
 *   - Niche construction: social systems modify their environment
 *
 * L7 - Application: social autopoiesis (Luhmann)
 * L7 - Application: organizational autopoiesis
 *
 * Reference: Luhmann, "Social Systems" (1984/1995)
 *            Maturana & Varela, "The Tree of Knowledge" (1987)
 */
#include "structural_coupling.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

int main(void)
{
    printf("============================================================\n");
    printf("  Social Autopoiesis — Luhmann's Theory of Social Systems\n");
    printf("  Communication events as autopoietic components\n");
    printf("============================================================\n\n");

    /* ==================================================================
     * Part 1: Create individual psychological systems
     * ================================================================== */
    printf("[1] Creating two psychological (individual) autopoietic systems...\n");

    /* Individual A: a person with 5 psychological "components"
     * (representing cognitive/emotional states) */
    sc_coupling_t person_A;
    sc_coupling_init(&person_A, 5, 2);

    double conc_A[] = {0.8, 0.6, 0.4, 0.3, 0.7};  /* cognitive states */
    double bound_A[] = {1.0, 0.5};                   /* boundary: identity, mood */
    sc_coupling_set_reference(&person_A, conc_A, bound_A);

    /* Person A's environment */
    double res_A[] = {1.0, 1.0, 1.0};  /* social resources */
    sc_coupling_set_environment(&person_A, 298.0, 7.0, res_A, 3);

    printf("  Person A: %d cognitive components, identity boundary\n",
           person_A.current_state.n_components);

    /* Individual B: another person with different initial states */
    sc_coupling_t person_B;
    sc_coupling_init(&person_B, 5, 2);

    double conc_B[] = {0.5, 0.8, 0.6, 0.2, 0.4};
    double bound_B[] = {1.0, 0.6};
    sc_coupling_set_reference(&person_B, conc_B, bound_B);

    double res_B[] = {1.0, 1.0, 1.0};
    sc_coupling_set_environment(&person_B, 298.0, 7.0, res_B, 3);

    printf("  Person B: %d cognitive components\n\n",
           person_B.current_state.n_components);

    /* ==================================================================
     * Part 2: Measure initial coupling
     * ================================================================== */
    printf("[2] Initial structural coupling analysis...\n");

    double coupling_AB = sc_cross_coupling_measure(&person_A, &person_B);
    printf("  Cross-coupling measure (A↔B): %.4f\n", coupling_AB);
    printf("  (Low initial coupling: individuals are distinct)\n\n");

    /* ==================================================================
     * Part 3: Consensual interactions (communication events)
     * ================================================================== */
    printf("[3] Simulating consensual interactions (communication)...\n");

    /* In Luhmann's theory, communication is a triple selection:
     * information → utterance → understanding.
     * Each communication event is a component of the social system. */

    for (int interaction = 0; interaction < 8; interaction++) {
        /* Alternate who initiates communication */
        sc_coupling_t *sender, *receiver;
        if (interaction % 2 == 0) {
            sender = &person_A;
            receiver = &person_B;
        } else {
            sender = &person_B;
            receiver = &person_A;
        }

        /* Communication magnitude: varies with "meaning content" */
        double signal = 0.1 + 0.05 * (double)(interaction % 3);

        sc_response_t response = sc_consensual_interaction(
            sender, receiver, signal);

        if (interaction < 4) {
            printf("  Interaction %d: %s → %s  signal=%.2f  response=%.3f  org=%.3f\n",
                   interaction + 1,
                   (interaction % 2 == 0) ? "A" : "B",
                   (interaction % 2 == 0) ? "B" : "A",
                   signal, response.response_magnitude,
                   receiver->current_state.organization_measure);
        }
    }
    printf("  ... (8 interactions total)\n\n");

    /* ==================================================================
     * Part 4: Social system emergence
     * ================================================================== */
    printf("[4] Social system emergence analysis...\n");

    /* After repeated interactions, a social system emerges.
     * This is the "consensual domain" in Maturana's terms or
     * the "social system" in Luhmann's terms. */

    coupling_AB = sc_cross_coupling_measure(&person_A, &person_B);

    printf("  Post-interaction cross-coupling: %.4f\n", coupling_AB);
    printf("  (Increased coupling: shared consensual domain forming)\n\n");

    /* ==================================================================
     * Part 5: Niche construction (institutionalization)
     * ================================================================== */
    printf("[5] Niche construction: social system modifies its environment...\n");

    /* Social systems construct their own niche: norms, institutions,
     * shared meanings that stabilize the social system. */

    printf("  Person A initial resources: ");
    for (int i = 0; i < person_A.env_resource_count; i++) {
        printf("%.2f ", person_A.env_resource_levels[i]);
    }
    printf("\n");

    /* Person A constructs niche by modifying resource 0 (e.g., creating a norm) */
    sc_niche_construction(&person_A, 0, 0.5);
    sc_niche_construction(&person_A, 0, 0.3);

    /* Person B also participates in niche construction */
    sc_niche_construction(&person_B, 1, 0.4);

    printf("  After niche construction:\n");
    printf("  Person A resources: ");
    for (int i = 0; i < person_A.env_resource_count; i++) {
        printf("%.2f ", person_A.env_resource_levels[i]);
    }
    printf("\n");

    double niche_A = sc_niche_construction_index(&person_A);
    double niche_B = sc_niche_construction_index(&person_B);
    printf("  Niche construction index — A: %.4f  B: %.4f\n", niche_A, niche_B);

    /* ==================================================================
     * Part 6: Structural drift over time
     * ================================================================== */
    printf("\n[6] Long-term structural drift (social evolution)...\n");

    for (int epoch = 0; epoch < 5; epoch++) {
        for (int step = 0; step < 20; step++) {
            sc_evolve_coupling(&person_A, 0.1);
            sc_evolve_coupling(&person_B, 0.1);
        }

        double drift_A = sc_compute_structural_drift(&person_A);
        double drift_B = sc_compute_structural_drift(&person_B);
        int org_A = sc_organization_preserved(&person_A, 0.3);
        int org_B = sc_organization_preserved(&person_B, 0.3);

        printf("  Epoch %d: drift_A=%.3f org_A=%s  drift_B=%.3f org_B=%s\n",
               epoch + 1, drift_A, org_A ? "YES" : "NO",
               drift_B, org_B ? "YES" : "NO");
    }

    /* ==================================================================
     * Part 7: Summary
     * ================================================================== */
    printf("\n============================================================\n");
    printf("  Social Autopoiesis Theory (Luhmann, 1984):\n");
    printf("  - Social systems are composed of communication events\n");
    printf("  - Each communication produces further communications\n");
    printf("  - Structural coupling between psychic and social systems\n");
    printf("  - Niche construction: institutions, norms, shared meanings\n");
    printf("  - Organization preserved through structural drift\n");
    printf("============================================================\n");

    sc_coupling_destroy(&person_A);
    sc_coupling_destroy(&person_B);
    return 0;
}
