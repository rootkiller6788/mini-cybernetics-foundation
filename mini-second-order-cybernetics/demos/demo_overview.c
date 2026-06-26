/* ============================================================================
 * demo_overview.c — Second-Order Cybernetics Demo
 *
 * Demonstrates key concepts: NTM, observer, eigenform, conversation.
 * ============================================================================ */

#include <stdio.h>
#include <math.h>
#include "../include/soc_core.h"

int main(void) {
    printf("=============================================\n");
    printf("  Second-Order Cybernetics — Demo Overview\n");
    printf("  \"Cybernetics of cybernetics\"\n");
    printf("=============================================\n\n");

    /* 1. Non-Trivial Machine */
    printf("1. Non-Trivial Machine (von Foerster)\n");
    printf("   Creating a 2-state NTM...\n");
    SOCNonTrivialMachine* ntm = soc_ntm_create(2, 1, 1, "demo_ntm");
    if (ntm) {
        double A[] = {0.7, -0.2, 0.1, 0.9};
        double B[] = {1.0, 0.5};
        double C[] = {1.0, 0.0};
        double D[] = {0.1};
        soc_ntm_set_linear(ntm, A, B, C, D);

        double in[] = {1.0};
        double out[1];
        for (int t = 0; t < 5; t++) {
            soc_ntm_step(ntm, in, out);
            printf("   t=%d: output=%.4f, state=[%.3f, %.3f]\n",
                   t, out[0], ntm->state[0], ntm->state[1]);
        }
        printf("   This NTM is %s\n\n",
               soc_ntm_is_trivial(ntm) ? "trivial" : "non-trivial");
        soc_ntm_free(ntm);
    }

    /* 2. Observer with Blind Spot */
    printf("2. Observer (Maturana & Varela)\n");
    SOCObserver* obs = soc_observer_create("demo_obs", SOC_OBSERVER_PARTICIPANT, 4, 3);
    if (obs) {
        printf("   World dim: %d, Obs dim: %d\n", obs->world_dim, obs->obs_dim);
        printf("   Blind spot: %.1f%%\n",
               soc_observer_blind_spot_size(obs) * 100.0);
        printf("   Key insight: Every observer has a blind spot.\n\n");
        soc_observer_free(obs);
    }

    /* 3. Conversation */
    printf("3. Conversation Theory (Pask)\n");
    SOCObserver* alice = soc_observer_create("Alice", SOC_OBSERVER_PARTICIPANT, 4, 4);
    SOCObserver* bob = soc_observer_create("Bob", SOC_OBSERVER_PARTICIPANT, 4, 4);
    if (alice && bob) {
        alice->belief_state[0] = 0.9;
        bob->belief_state[0] = 0.1;
        printf("   Initial agreement: %.4f\n",
               soc_cosine_similarity(alice->belief_state, bob->belief_state, 4));

        SOCConversation* conv = soc_conversation_create("demo", 4, alice, bob);
        for (int t = 0; t < 10; t++) soc_conversation_turn(conv);
        printf("   After 10 turns: agreement = %.4f\n", conv->agreement_level);
        printf("   Converged: %s\n", conv->stabilized ? "yes" : "no");
        printf("   Key insight: Understanding emerges through dialogue.\n\n");
        soc_conversation_free(conv);
    }
    soc_observer_free(alice);
    soc_observer_free(bob);

    printf("=============================================\n");
    printf("  \"Act always so as to increase the number\n");
    printf("   of choices.\" — Heinz von Foerster\n");
    printf("=============================================\n");
    return 0;
}

