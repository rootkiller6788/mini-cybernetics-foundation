/* ============================================================================
 * Example 3: Pask's Conversation Theory — Understanding Through Dialogue
 *
 * Demonstrates how two observers (Alice and Bob) with different initial
 * beliefs converge to shared understanding through recursive conversation
 * (teachback). This is Pask's Conversation Theory (1975).
 *
 * Key result: Agreement in a conversation converges if participants
 * share an entailment structure — even with different initial states.
 * ============================================================================ */

#include <stdio.h>
#include <math.h>
#include "../include/soc_core.h"

int main(void) {
    printf("============================================\n");
    printf("  Example 3: Conversation Theory (Pask)\n");
    printf("  Understanding through recursive dialogue\n");
    printf("============================================\n\n");

    /* Create two observers with different belief states */
    SOCObserver* alice = soc_observer_create("Alice", SOC_OBSERVER_PARTICIPANT, 4, 4);
    SOCObserver* bob = soc_observer_create("Bob", SOC_OBSERVER_PARTICIPANT, 4, 4);

    if (!alice || !bob) {
        printf("Failed to create observers.\n");
        return 1;
    }

    /* Alice initially believes in [1.0, 0.2, 0.1, 0.1] */
    alice->belief_state[0] = 1.0;
    alice->belief_state[1] = 0.2;

    /* Bob initially believes in [0.1, 0.9, 0.8, 0.8] */
    bob->belief_state[0] = 0.1;
    bob->belief_state[1] = 0.9;
    bob->belief_state[2] = 0.8;
    bob->belief_state[3] = 0.8;

    printf("Initial beliefs:\n");
    printf("  Alice: [%.3f, %.3f, %.3f, %.3f]\n",
           alice->belief_state[0], alice->belief_state[1],
           alice->belief_state[2], alice->belief_state[3]);
    printf("  Bob:   [%.3f, %.3f, %.3f, %.3f]\n",
           bob->belief_state[0], bob->belief_state[1],
           bob->belief_state[2], bob->belief_state[3]);

    /* Initial agreement */
    double init_agree = soc_cosine_similarity(
        alice->belief_state, bob->belief_state, 4);
    printf("  Initial agreement (cosine similarity): %.4f\n\n", init_agree);

    /* Create conversation */
    SOCConversation* conv = soc_conversation_create(
        "epistemology", 4, alice, bob);
    if (!conv) {
        printf("Failed to create conversation.\n");
        return 1;
    }

    /* Run conversation for 30 turns */
    printf("Running conversation for 30 turns...\n");
    printf(" Turn | Agreement | Teachback | Stabilized\n");
    printf("------|-----------|-----------|----------\n");

    for (int t = 0; t < 30; t++) {
        soc_conversation_turn(conv);

        if (t < 10 || t % 5 == 0 || t == 29) {
            printf(" %3d  |  %.4f   |  %.4f   |  %s\n",
                   t + 1, conv->agreement_level,
                   conv->teachback_fidelity,
                   conv->stabilized ? "YES" : "no");
        }
        if (conv->stabilized && t < 15) {
            printf("  ... (converged early)\n");
            break;
        }
    }

    printf("\nFinal state:\n");
    printf("  Alice: [%.4f, %.4f, %.4f, %.4f]\n",
           alice->belief_state[0], alice->belief_state[1],
           alice->belief_state[2], alice->belief_state[3]);
    printf("  Bob:   [%.4f, %.4f, %.4f, %.4f]\n",
           bob->belief_state[0], bob->belief_state[1],
           bob->belief_state[2], bob->belief_state[3]);
    printf("  Shared: [%.4f, %.4f, %.4f, %.4f]\n",
           conv->shared_model[0], conv->shared_model[1],
           conv->shared_model[2], conv->shared_model[3]);

    double final_agree = soc_cosine_similarity(
        alice->belief_state, bob->belief_state, 4);
    printf("\n  Final agreement: %.4f\n", final_agree);
    printf("  Convergence: %s\n", conv->stabilized ? "YES" : "no");
    printf("  Teachback fidelity: %.4f\n\n", conv->teachback_fidelity);

    /* Second conversation: paradoxical topic */
    printf("--- Paradox Detection ---\n");
    const char* liar = "This statement is false";
    bool is_paradox = soc_paradox_detect(liar);
    printf("  \"%s\"\n", liar);
    printf("  Self-referential paradox detected: %s\n",
           is_paradox ? "YES" : "no");

    SOCParadox* p = soc_paradox_create(SOC_PARADOX_LIAR, liar);
    soc_paradox_resolve(p);
    printf("  Resolution: %s\n", p->resolution_strategy);

    soc_paradox_free(p);
    soc_conversation_free(conv);
    soc_observer_free(alice);
    soc_observer_free(bob);

    printf("\n=== Key Insight ===\n");
    printf("Pask's Conversation Theory shows that understanding is not\n");
    printf("transmitted but converged upon through recursive interaction.\n");
    printf("Agreement is an eigenform: the fixed point of dialogue.\n");

    return 0;
}

