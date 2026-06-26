#include "cp_core.h"
#include "cp_hierarchy.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Example 3: Hierarchical Control ? Multi-Layer Regulation
 *
 * Demonstrates hierarchical control decomposition inspired by:
 *   Mesarovic, Macko, Takahara (1970) "Theory of Hierarchical, Multilevel Systems"
 *   Beer, S. (1972) "Brain of the Firm" ? Viable System Model
 *   Simon, H.A. (1962) "The Architecture of Complexity"
 *
 * A manufacturing plant with three control layers:
 *   Strategic  (slow, abstract)    ? Sets production targets, adapts to market
 *   Tactical   (medium, coordinated)? Allocates resources, coordinates units
 *   Operational (fast, execution)  ? Controls individual machines
 *
 * Key philosophical insights:
 *   1. Higher layers set goals for lower layers (top-down causation)
 *   2. Lower layers report status upward (bottom-up feedback)
 *   3. Each layer operates at its own time scale (near-decomposability)
 *   4. System behavior exceeds sum of parts (emergence)
 */

int main(void) {
    printf("=== Example 3: Hierarchical Control Architecture ===\n");
    printf("Mesarovic et al. (1970): Multilevel Hierarchical Organization\n\n");

    /* --- Build the hierarchy --- */
    HierarchicalControl* plant = cp_hierarchical_create("Manufacturing Plant", 3);

    /* Strategic Layer: sets global production goals, plans months ahead */
    ControlLayer* strategic = cp_layer_create("Strategic (VSM System 5)", CP_STRATEGIC_LAYER,
                                               2, 30.0, 365.0);
    double strat_goals[2] = {1000.0, 500.0};  /* target: 1000 units product A, 500 product B */
    double strat_constraints[2] = {2000.0, 1000.0};  /* capacity limits */
    cp_layer_set_goals(strategic, strat_goals, 2);
    cp_layer_set_constraints(strategic, strat_constraints, 2);

    /* Tactical Layer: allocates resources, coordinates, plans days ahead */
    ControlLayer* tactical = cp_layer_create("Tactical (VSM System 3)", CP_COORDINATION_LAYER,
                                              2, 7.0, 30.0);

    /* Operational Layer: fast control loop, seconds to minutes */
    ControlLayer* operational = cp_layer_create("Operational (VSM System 1)", CP_EXECUTION_LAYER,
                                                 2, 0.1, 1.0);

    /* Wire up hierarchy */
    int idx_s = cp_hierarchical_add_layer(plant, strategic);
    int idx_t = cp_hierarchical_add_layer(plant, tactical);
    int idx_o = cp_hierarchical_add_layer(plant, operational);

    tactical->parent_idx = idx_s;
    operational->parent_idx = idx_t;
    cp_layer_add_child(strategic, idx_t);
    cp_layer_add_child(tactical, idx_o);

    printf("Hierarchy structure:\n");
    printf("  [%d] Strategic  (?=30d, H=365d) ? %d goals, %d constraints\n",
           idx_s, strategic->n_goals, strategic->n_constraints);
    printf("  [%d] Tactical   (?=7d,  H=30d)  ? parent: %d, children: %d\n",
           idx_t, tactical->parent_idx, tactical->n_children);
    printf("  [%d] Operational(?=0.1d, H=1d)  ? parent: %d\n",
           idx_o, operational->parent_idx);

    /* --- Simulate --- */
    printf("\nSimulating hierarchical control (20 steps, dt=1.0 day):\n");
    printf("Step\tStrategic_y\tTactical_y\tOperational_y\tCoordCost\tEmergence\n");
    printf("----\t-----------\t----------\t-------------\t---------\t---------\n");

    for (int step = 0; step < 20; step++) {
        cp_hierarchical_step(plant, 1.0);
        double em = cp_hierarchical_emergence(plant);
        plant->emergence_measure = em;

        printf("%4d\t%10.3f\t%10.3f\t%12.3f\t%9.3f\t%9.4f\n",
               step,
               strategic->state ? strategic->state[0] : 0.0,
               tactical->state ? tactical->state[0] : 0.0,
               operational->state ? operational->state[0] : 0.0,
               plant->coordination_overhead,
               em);
    }

    /* --- Emergence Analysis --- */
    printf("\n=== Emergence Analysis ===\n");
    EmergenceAnalysis* ea = cp_emergence_analyze(plant);
    printf("Weak emergence:     %.4f  (arises from interaction of parts)\n", ea->weak_emergence);
    printf("Strong emergence:   %.4f  (irreducible to parts alone)\n", ea->strong_emergence);
    printf("Downward causation: %.4f  (higher layers constrain lower)\n", ea->downward_causation);
    printf("Self-organization:  %.4f  (spontaneous order)\n", ea->self_organization);
    printf("Complexity gap:     %.4f  (C(system) - Sum(C(components)))\n", ea->complexity_gap);
    printf("Emergent behavior:  %s\n", ea->has_emergent_behavior ? "DETECTED" : "None");

    /* --- Viable System Model --- */
    printf("\n=== Viable System Model (Stafford Beer) ===\n");
    ViableSystemModel* vsm = cp_vsm_create("PlantCo");
    cp_vsm_set_operational_units(vsm, 2);
    cp_vsm_set_autonomy(vsm, 0, 0.7);  /* Unit A: moderately autonomous */
    cp_vsm_set_autonomy(vsm, 1, 0.9);  /* Unit B: highly autonomous */
    cp_vsm_step(vsm, 10.0);

    printf("Identity: %s\n", vsm->identity);
    printf("Cohesion: %.4f\n", vsm->cohesion_measure);
    printf("Viable: %s\n", cp_vsm_is_viable(vsm) ? "YES (all viability criteria met)" : "NO");
    printf("Oscillation amplitude: %.6f\n", vsm->oscillation_amplitude);
    cp_vsm_print(vsm);

    /* --- Near-Decomposability (Simon, 1962) --- */
    printf("\n=== Near-Decomposability Analysis ===\n");
    /* Simple interaction matrix: 2 subsystems of size 2 */
    double inter[16] = {
        1.0, 0.8, 0.05, 0.02,   /* Block 1 */
        0.8, 1.0, 0.01, 0.03,
        0.05, 0.01, 1.0, 0.9,   /* Block 2 */
        0.02, 0.03, 0.9, 1.0
    };
    int partition[4] = {0, 0, 1, 1};
    double nd = cp_near_decomposability(inter, 4, partition, 1.5);
    printf("Interaction matrix (4x4, 2 subsystems):\n");
    for (int i = 0; i < 4; i++) {
        printf("  ");
        for (int j = 0; j < 4; j++) printf("%.2f ", inter[i * 4 + j]);
        printf("\n");
    }
    printf("Near-decomposability index: %.4f\n", nd);
    printf("(> 0.7 ? highly decomposable, Simon''s architecture of complexity)\n");

    /* Optimal span of control */
    double b_opt = cp_optimal_span_of_control(100, 0.5, 0.2);
    printf("\nOptimal span of control for 100 leaves: %.1f\n", b_opt);

    /* --- Goal Tree --- */
    printf("\n=== Goal Decomposition Tree ===\n");
    GoalTree* gt = cp_goal_tree_create(10);
    int root = cp_goal_tree_add_goal(gt, "Maximize Profit", strat_goals, 2, 1.0, -1);
    cp_goal_tree_decompose(gt, root);
    printf("Coherence after decomposition: %.4f\n", cp_goal_tree_coherence(gt));
    cp_goal_tree_print(gt);

    /* Cleanup */
    cp_emergence_free(ea);
    cp_goal_tree_free(gt);
    cp_layer_free(strategic);
    cp_layer_free(tactical);
    cp_layer_free(operational);
    cp_hierarchical_free(plant);
    cp_vsm_free(vsm);

    printf("\nExample 3 complete.\n");
    return 0;
}
