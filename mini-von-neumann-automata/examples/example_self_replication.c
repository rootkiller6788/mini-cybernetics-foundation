#include "vna_core.h"
#include "vna_cellular.h"
#include "vna_replication.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Example: Self-Replication in Cellular Automata
 *
 * Demonstrates:
 *   - Langton loop pattern generation
 *   - Self-replication detection
 *   - Replication feasibility analysis
 *   - Universal constructor simulation
 *   - Population dynamics of self-replicating structures
 * ============================================================================ */

int main(void) {
    printf("=== Self-Replication in Cellular Automata ===\n\n");

    /* --- Demonstrate Langton Loop Pattern --- */
    printf("1. Langton Self-Replicating Loop\n");
    printf("   Generating a 10x10 Langton loop...\n");
    VnaPattern* loop = vna_langton_loop_pattern(10, 0, 0);
    if (loop) {
        printf("   Loop dimensions: %dx%d\n", loop->width, loop->height);
        printf("   Loop name: %s\n", loop->name);
        printf("   Sheath: state %d   Data path: state %d   Core: state %d\n",
               LANGTON_SHEATH, LANGTON_DATA_PATH, LANGTON_CORE);
        printf("   Signals: states %d-%d\n",
               LANGTON_SIGNAL_0, LANGTON_SIGNAL_3);
    }

    /* --- Universal Constructor Simulation --- */
    printf("\n2. Von Neumann Universal Constructor\n");
    VnaUniversalConstructor* uc = vna_constructor_create();
    if (!uc) {
        fprintf(stderr, "Failed to create constructor\n");
        vna_pattern_free(loop);
        return 1;
    }

    /* Add construction instructions (simplified) */
    for (int i = 0; i < 20; i++) {
        uint8_t op = (uint8_t)(i % 4);   /* EXTEND, INJECT, RETRACT, TURN */
        uint8_t arg = (uint8_t)(i % 8);   /* State to inject */
        vna_constructor_add_instruction(uc, op, arg, 1);
    }
    printf("   Added %d construction instructions\n", uc->tape.length);

    /* Run construction on a target lattice */
    VnaLattice* target = vna_lattice_create(50, 50, 8, VNA_LATTICE_2D_SQUARE);
    if (target) {
        printf("   Running construction steps...\n");
        int steps_run = 0;
        bool active = true;
        while (active && steps_run < 50) {
            active = vna_constructor_step(uc, target);
            steps_run++;
        }
        printf("   Construction phases completed: %d steps\n", steps_run);
        printf("   Arm position: (%d, %d), direction: %d\n",
               uc->arm_x, uc->arm_y, uc->arm_direction);
        printf("   Tape position: %d/%d\n",
               uc->tape.position, uc->tape.length);

        VnaReplicationPhase phase = vna_constructor_get_phase(uc);
        printf("   Phase: %d (0=IDLE, 1=READ, 2=BUILD_ARM, 3=BUILD_BODY, "
               "4=COPY, 5=SEPARATE, 6=DONE)\n", phase);
    }

    /* --- Replication Detection --- */
    printf("\n3. Replication Detection\n");
    VnaLattice* before_lat = vna_lattice_create(30, 30, 8, VNA_LATTICE_2D_SQUARE);
    VnaLattice* after_lat = vna_lattice_create(30, 30, 8, VNA_LATTICE_2D_SQUARE);

    if (before_lat && after_lat) {
        /* Insert a pattern in before_lat */
        if (loop) {
            vna_pattern_insert(before_lat, loop, 2, 2);
            /* "Replicate" — insert a similar pattern in a different location */
            vna_pattern_insert(after_lat, loop, 2, 2);
            vna_pattern_insert(after_lat, loop, 15, 15);
        }

        int event_x[10], event_y[10];
        int events = vna_detect_replication_events(before_lat, after_lat,
                                                     3, event_x, event_y, 10);
        printf("   Detected %d replication-like events\n", events);
        for (int i = 0; i < events; i++)
            printf("   Event %d at (%d, %d)\n", i, event_x[i], event_y[i]);
    }

    /* --- Replication Complexity --- */
    printf("\n4. Replication Complexity Bound\n");
    if (target) {
        int64_t k_bound = vna_replication_complexity_bound(target);
        printf("   Estimated Kolmogorov complexity bound: %lld bits\n",
               (long long)k_bound);
    }

    /* --- Population Simulation --- */
    printf("\n5. Artificial Life Population\n");
    VnaPopulation* pop = vna_population_create();
    if (pop && before_lat) {
        /* Simulate a few generations of artificial life */
        for (int t = 0; t < 5; t++) {
            VnaLattice* gen_before = vna_lattice_create(30, 30, 8,
                                                         VNA_LATTICE_2D_SQUARE);
            VnaLattice* gen_after = vna_lattice_create(30, 30, 8,
                                                        VNA_LATTICE_2D_SQUARE);
            if (gen_before && gen_after) {
                vna_lattice_randomize(gen_before, 0.2 + t * 0.05,
                                       (unsigned int)(100 + t));
                vna_lattice_randomize(gen_after, 0.2 + (t+1) * 0.05,
                                       (unsigned int)(200 + t));
                vna_population_update(pop, gen_before, gen_after);
            }
            vna_lattice_free(gen_before);
            vna_lattice_free(gen_after);
        }
        vna_population_print(pop);
    }

    /* Cleanup */
    vna_pattern_free(loop);
    vna_constructor_free(uc);
    vna_lattice_free(target);
    vna_lattice_free(before_lat);
    vna_lattice_free(after_lat);
    vna_population_free(pop);

    printf("\n[Self-replication demonstration complete]\n");
    return 0;
}
