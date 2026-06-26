/**
 * @file organizational_closure.c
 * @brief Algorithms for computing organizational closure — the defining
 *        property of autopoietic systems.
 *
 * Organizational closure means all components of the system are produced
 * within the network of productions they themselves constitute. This file
 * implements the algorithms that verify and quantify this property.
 *
 * Knowledge points implemented:
 *   L4 - oc_production_closure: iterative production closure (fixed-point)
 *   L4 - oc_catalytic_closure: catalysis-driven closure
 *   L4 - oc_organizational_closure: simultaneous production + catalytic closure
 *   L4 - oc_find_minimal_organizational_set: minimal closure generator
 *   L4 - oc_boundary_is_self_produced: boundary self-production verification
 *   L5 - oc_dependency_depths: production chain depth analysis
 *   L5 - oc_identify_autopoietic_core: core component identification
 *   L5 - oc_closure_index: closure quantification
 */

#include "organizational_closure.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==========================================================================
 * System Management
 * ========================================================================== */

void oc_system_init(oc_system_t *sys, int n_components, int n_reactions)
{
    if (!sys) return;
    memset(sys, 0, sizeof(oc_system_t));

    if (n_components <= 0 || n_components > OC_MAX_COMPONENTS) return;
    if (n_reactions <= 0 || n_reactions > OC_MAX_REACTIONS) return;

    sys->n_components = n_components;
    sys->n_reactions = n_reactions;

    /* Allocate reaction data arrays (calloc for zero-init, safe to free later) */
    sys->reaction_substrates = (int **)calloc(n_reactions, sizeof(int *));
    sys->reaction_substrate_counts = (int *)calloc(n_reactions, sizeof(int));
    sys->reaction_products = (int **)calloc(n_reactions, sizeof(int *));
    sys->reaction_product_counts = (int *)calloc(n_reactions, sizeof(int));

    /* Allocate catalysis matrix */
    sys->catalysis = (int **)malloc(n_components * sizeof(int *));
    for (int i = 0; i < n_components; i++) {
        sys->catalysis[i] = (int *)calloc(n_reactions, sizeof(int));
    }

    /* Allocate component property arrays */
    sys->is_produced = (int *)calloc(n_components, sizeof(int));
    sys->is_boundary = (int *)calloc(n_components, sizeof(int));
    sys->reachable = (int *)calloc(n_components, sizeof(int));
    sys->closure_set = (int *)calloc(n_components, sizeof(int));
}

int oc_add_reaction(oc_system_t *sys, int reaction_id,
                     const int *substrates, int substrate_count,
                     const int *products, int product_count)
{
    if (!sys || reaction_id < 0 || reaction_id >= sys->n_reactions) return -1;
    if (substrate_count > 8 || product_count > 8) return -1;

    /* Copy substrates */
    sys->reaction_substrate_counts[reaction_id] = substrate_count;
    if (substrate_count > 0 && substrates) {
        sys->reaction_substrates[reaction_id] = (int *)malloc(substrate_count * sizeof(int));
        memcpy(sys->reaction_substrates[reaction_id], substrates,
               substrate_count * sizeof(int));
    }

    /* Copy products and mark them as produced */
    sys->reaction_product_counts[reaction_id] = product_count;
    if (product_count > 0 && products) {
        sys->reaction_products[reaction_id] = (int *)malloc(product_count * sizeof(int));
        memcpy(sys->reaction_products[reaction_id], products,
               product_count * sizeof(int));
        /* Mark products as produced */
        for (int p = 0; p < product_count; p++) {
            int comp = products[p];
            if (comp >= 0 && comp < sys->n_components) {
                sys->is_produced[comp] = 1;
            }
        }
    }

    return 0;
}

void oc_set_catalysis(oc_system_t *sys, int component_id, int reaction_id, int value)
{
    if (!sys || component_id < 0 || component_id >= sys->n_components) return;
    if (reaction_id < 0 || reaction_id >= sys->n_reactions) return;

    sys->catalysis[component_id][reaction_id] = value ? 1 : 0;
}

void oc_set_boundary(oc_system_t *sys, int component_id)
{
    if (!sys || component_id < 0 || component_id >= sys->n_components) return;
    if (!sys->is_boundary[component_id]) {
        sys->is_boundary[component_id] = 1;
        sys->boundary_count++;
    }
}

void oc_system_destroy(oc_system_t *sys)
{
    if (!sys) return;

    for (int j = 0; j < sys->n_reactions; j++) {
        free(sys->reaction_substrates[j]);
        free(sys->reaction_products[j]);
    }
    free(sys->reaction_substrates);
    free(sys->reaction_substrate_counts);
    free(sys->reaction_products);
    free(sys->reaction_product_counts);

    for (int i = 0; i < sys->n_components; i++) {
        free(sys->catalysis[i]);
    }
    free(sys->catalysis);

    free(sys->is_produced);
    free(sys->is_boundary);
    free(sys->reachable);
    free(sys->closure_set);

    memset(sys, 0, sizeof(oc_system_t));
}

/* ==========================================================================
 * Closure Operations — Core Theorems (L4)
 * ========================================================================== */

void oc_production_closure(const oc_system_t *sys,
                            const int *food_set, int food_count,
                            int *closure, int *closure_size)
{
    if (!sys || !food_set || !closure || !closure_size) return;

    int n = sys->n_components;
    int *available = (int *)calloc(n, sizeof(int));
    if (!available) { *closure_size = 0; return; }

    /* Initialize with food set */
    for (int f = 0; f < food_count; f++) {
        int comp = food_set[f];
        if (comp >= 0 && comp < n) {
            available[comp] = 1;
        }
    }

    /* Iterative fixed-point: repeatedly apply all reactions whose
     * substrates are available, adding their products. Continue until
     * no new components become available. */
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int j = 0; j < sys->n_reactions; j++) {
            /* Check if all substrates are available */
            int all_available = 1;
            int sc = sys->reaction_substrate_counts[j];
            for (int s = 0; s < sc; s++) {
                int sub = sys->reaction_substrates[j][s];
                if (sub < 0 || sub >= n || !available[sub]) {
                    all_available = 0;
                    break;
                }
            }

            if (all_available) {
                /* Add all products to available set */
                int pc = sys->reaction_product_counts[j];
                for (int p = 0; p < pc; p++) {
                    int prod = sys->reaction_products[j][p];
                    if (prod >= 0 && prod < n && !available[prod]) {
                        available[prod] = 1;
                        changed = 1;
                    }
                }
            }
        }
    }

    /* Collect closure set */
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (available[i]) {
            closure[count++] = i;
        }
    }
    *closure_size = count;

    /* Note: sys is const, cannot cache results here */

    free(available);
}

void oc_catalytic_closure(const oc_system_t *sys,
                           const int *component_set, int component_count,
                           int *catalyzed_reactions, int *cat_count)
{
    if (!sys || !component_set || !catalyzed_reactions || !cat_count) return;

    /* Build a quick-lookup mask for the component set */
    int *in_set = (int *)calloc(sys->n_components, sizeof(int));
    if (!in_set) { *cat_count = 0; return; }

    for (int i = 0; i < component_count; i++) {
        int comp = component_set[i];
        if (comp >= 0 && comp < sys->n_components) {
            in_set[comp] = 1;
        }
    }

    /* Find all reactions catalyzed by at least one component in the set */
    int count = 0;
    for (int j = 0; j < sys->n_reactions; j++) {
        for (int c = 0; c < sys->n_components; c++) {
            if (in_set[c] && sys->catalysis[c][j]) {
                catalyzed_reactions[count++] = j;
                break;
            }
        }
    }
    *cat_count = count;

    free(in_set);
}

void oc_organizational_closure(const oc_system_t *sys,
                                const int *initial_set, int initial_count,
                                int *org_closure, int *org_size,
                                int *converged)
{
    if (!sys || !initial_set || !org_closure || !org_size || !converged) return;

    int n = sys->n_components;
    int m = sys->n_reactions;
    int *current_set = (int *)calloc(n, sizeof(int));
    int *prev_set = (int *)calloc(n, sizeof(int));
    int *prod_closure = (int *)malloc(n * sizeof(int));
    int *cat_reactions = (int *)malloc(m * sizeof(int));

    if (!current_set || !prev_set || !prod_closure || !cat_reactions) {
        *org_size = 0;
        *converged = 0;
        free(current_set); free(prev_set);
        free(prod_closure); free(cat_reactions);
        return;
    }

    /* Initialize */
    for (int i = 0; i < initial_count; i++) {
        int comp = initial_set[i];
        if (comp >= 0 && comp < n) {
            current_set[comp] = 1;
        }
    }

    int iterations = 0;
    int max_iterations = 100;

    do {
        /* Save previous set */
        memcpy(prev_set, current_set, n * sizeof(int));

        /* Step 1: Production closure from current set */
        int init_arr[OC_MAX_COMPONENTS];
        int init_sz = 0;
        for (int i = 0; i < n; i++) {
            if (current_set[i]) init_arr[init_sz++] = i;
        }

        int prod_sz;
        oc_production_closure(sys, init_arr, init_sz, prod_closure, &prod_sz);

        /* Update current set with production closure */
        for (int i = 0; i < prod_sz; i++) {
            current_set[prod_closure[i]] = 1;
        }

        /* Step 2: Catalytic closure — find reactions catalyzed by current set */
        int cat_sz;
        oc_catalytic_closure(sys, prod_closure, prod_sz, cat_reactions, &cat_sz);

        /* Step 3: Add products of catalyzed reactions that have available substrates */
        for (int r = 0; r < cat_sz; r++) {
            int j = cat_reactions[r];
            int all_substrates_available = 1;
            int sc = sys->reaction_substrate_counts[j];
            for (int s = 0; s < sc; s++) {
                int sub = sys->reaction_substrates[j][s];
                if (sub < 0 || sub >= n || !current_set[sub]) {
                    all_substrates_available = 0;
                    break;
                }
            }
            if (all_substrates_available) {
                int pc = sys->reaction_product_counts[j];
                for (int p = 0; p < pc; p++) {
                    int prod = sys->reaction_products[j][p];
                    if (prod >= 0 && prod < n) {
                        current_set[prod] = 1;
                    }
                }
            }
        }

        /* Check convergence: has the set changed? */
        int set_size = 0;
        for (int i = 0; i < n; i++) if (current_set[i]) set_size++;

        iterations++;
    } while (memcmp(current_set, prev_set, n * sizeof(int)) != 0 && iterations < max_iterations);

    *converged = (iterations < max_iterations) ? 1 : 0;

    /* Collect result */
    int sz = 0;
    for (int i = 0; i < n; i++) {
        if (current_set[i]) {
            org_closure[sz++] = i;
        }
    }
    *org_size = sz;

    free(current_set);
    free(prev_set);
    free(prod_closure);
    free(cat_reactions);
}

int oc_find_minimal_organizational_set(const oc_system_t *sys,
                                        int *minimal_set, int *min_size)
{
    if (!sys || !minimal_set || !min_size) return -1;

    int n = sys->n_components;

    /* Start with all producible components as the candidate set */
    int *candidate = (int *)calloc(n, sizeof(int));
    int candidate_sz = 0;
    for (int i = 0; i < n; i++) {
        if (sys->is_produced[i]) {
            candidate[i] = 1;
            candidate_sz++;
        }
    }

    /* Greedy reduction: try removing each component and check if
     * the organizational closure still covers all producible components */
    for (int i = 0; i < n; i++) {
        if (!candidate[i]) continue;

        /* Try removing i */
        candidate[i] = 0;

        /* Build initial set for closure from remaining candidates */
        int init_arr[OC_MAX_COMPONENTS];
        int init_sz = 0;
        for (int k = 0; k < n; k++) {
            if (candidate[k]) init_arr[init_sz++] = k;
        }

        int closure[OC_MAX_COMPONENTS];
        int closure_sz;
        int conv;
        oc_organizational_closure(sys, init_arr, init_sz, closure, &closure_sz, &conv);

        /* Check if all producible components are still in closure */
        int all_covered = 1;
        for (int k = 0; k < n; k++) {
            if (sys->is_produced[k]) {
                int found = 0;
                for (int c = 0; c < closure_sz; c++) {
                    if (closure[c] == k) { found = 1; break; }
                }
                if (!found) { all_covered = 0; break; }
            }
        }

        if (!all_covered) {
            /* Need to keep i */
            candidate[i] = 1;
        }
    }

    /* Collect minimal set */
    int sz = 0;
    for (int i = 0; i < n; i++) {
        if (candidate[i]) {
            minimal_set[sz++] = i;
        }
    }
    *min_size = sz;

    free(candidate);
    return 0;
}

/* ==========================================================================
 * Closure Metrics (L5)
 * ========================================================================== */

double oc_closure_index(const oc_system_t *sys)
{
    if (!sys || sys->n_components == 0) return 0.0;

    /* Compute production closure from all initially available species */
    int food_set[OC_MAX_COMPONENTS];
    memset(food_set, 0, sizeof(food_set));
    int food_count = 0;

    /* Food set: all components that are not produced (imported/basal) */
    for (int i = 0; i < sys->n_components; i++) {
        if (!sys->is_produced[i]) {
            food_set[food_count++] = i;
        }
    }

    int closure[OC_MAX_COMPONENTS];
    int closure_size;
    oc_production_closure(sys, food_set, food_count, closure, &closure_size);

    int producible_count = 0;
    for (int i = 0; i < sys->n_components; i++) {
        if (sys->is_produced[i]) producible_count++;
    }

    if (producible_count == 0) return 1.0;

    /* Count how many producible components are in the closure */
    int produced_in_closure = 0;
    for (int c = 0; c < closure_size; c++) {
        if (sys->is_produced[closure[c]]) produced_in_closure++;
    }

    return (double)produced_in_closure / (double)producible_count;
}

void oc_dependency_depths(const oc_system_t *sys,
                           const int *food_set, int food_count,
                           int *depths)
{
    if (!sys || !food_set || !depths) return;

    int n = sys->n_components;

    /* Initialize depths to -1 (unreachable) */
    for (int i = 0; i < n; i++) depths[i] = -1;

    /* Food set has depth 0 */
    for (int f = 0; f < food_count; f++) {
        int comp = food_set[f];
        if (comp >= 0 && comp < n) {
            depths[comp] = 0;
        }
    }

    /* BFS-like iterative deepening: for each pass, assign depth = 1 + max(substrate depth) */
    int changed = 1;
    int current_depth = 1;
    while (changed) {
        changed = 0;
        for (int j = 0; j < sys->n_reactions; j++) {
            int sc = sys->reaction_substrate_counts[j];
            if (sc == 0) continue;

            /* Find max depth among substrates */
            int max_depth = -1;
            int all_assigned = 1;
            for (int s = 0; s < sc; s++) {
                int sub = sys->reaction_substrates[j][s];
                if (sub < 0 || sub >= n) continue;
                if (depths[sub] < 0) { all_assigned = 0; break; }
                if (depths[sub] > max_depth) max_depth = depths[sub];
            }

            if (all_assigned && max_depth >= 0) {
                int new_depth = max_depth + 1;
                int pc = sys->reaction_product_counts[j];
                for (int p = 0; p < pc; p++) {
                    int prod = sys->reaction_products[j][p];
                    if (prod >= 0 && prod < n && depths[prod] < 0) {
                        depths[prod] = new_depth;
                        changed = 1;
                    }
                }
            }
        }
        current_depth++;
    }
}

int oc_boundary_is_self_produced(const oc_system_t *sys)
{
    if (!sys || sys->boundary_count == 0) {
        /* No boundary defined: vacuously self-produced (or undefined) */
        return 1;
    }

    /* Find the food set: components not produced by any reaction */
    int food_set[OC_MAX_COMPONENTS];
    memset(food_set, 0, sizeof(food_set));
    int food_count = 0;
    for (int i = 0; i < sys->n_components; i++) {
        if (!sys->is_produced[i] && !sys->is_boundary[i]) {
            food_set[food_count++] = i;
        }
    }

    /* Compute production closure from food set */
    int closure[OC_MAX_COMPONENTS];
    int closure_size;
    oc_production_closure(sys, food_set, food_count, closure, &closure_size);

    /* Build quick lookup for closure */
    int *in_closure = (int *)calloc((size_t)sys->n_components, sizeof(int));
    for (int c = 0; c < closure_size; c++) {
        in_closure[closure[c]] = 1;
    }

    /* Check that every boundary component is in the closure */
    int all_self_produced = 1;
    for (int i = 0; i < sys->n_components; i++) {
        if (sys->is_boundary[i] && !in_closure[i]) {
            all_self_produced = 0;
            break;
        }
    }

    free(in_closure);
    return all_self_produced;
}

void oc_identify_autopoietic_core(const oc_system_t *sys,
                                   int *core, int *core_size)
{
    if (!sys || !core || !core_size) return;

    int n = sys->n_components;
    int *in_core = (int *)calloc(n, sizeof(int));
    if (!in_core) { *core_size = 0; return; }

    /* A component is in the autopoietic core if it is:
     * 1. Produced within the system (is_produced)
     * 2. Consumed as a substrate in at least one reaction (not terminal)
     * 3. Part of the organizational closure */

    /* Determine which components are consumed */
    int *is_consumed = (int *)calloc(n, sizeof(int));
    for (int j = 0; j < sys->n_reactions; j++) {
        int sc = sys->reaction_substrate_counts[j];
        for (int s = 0; s < sc; s++) {
            int sub = sys->reaction_substrates[j][s];
            if (sub >= 0 && sub < n) {
                is_consumed[sub] = 1;
            }
        }
    }

    /* Compute organizational closure to identify self-sustaining set */
    int food_set[OC_MAX_COMPONENTS];
    memset(food_set, 0, sizeof(food_set));
    int food_count = 0;
    for (int i = 0; i < n; i++) {
        if (!sys->is_produced[i]) {
            food_set[food_count++] = i;
        }
    }

    int closure[OC_MAX_COMPONENTS];
    int closure_size;
    oc_production_closure(sys, food_set, food_count, closure, &closure_size);

    int *in_closure = (int *)calloc(n, sizeof(int));
    for (int c = 0; c < closure_size; c++) {
        in_closure[closure[c]] = 1;
    }

    /* Core = produced + consumed + in closure + not boundary */
    int sz = 0;
    for (int i = 0; i < n; i++) {
        if (sys->is_produced[i] && is_consumed[i] &&
            in_closure[i] && !sys->is_boundary[i]) {
            in_core[i] = 1;
            core[sz++] = i;
        }
    }
    *core_size = sz;

    free(in_core);
    free(is_consumed);
    free(in_closure);
}

void oc_system_print_analysis(const oc_system_t *sys)
{
    if (!sys) { printf("OC System: NULL\n"); return; }

    printf("=== Organizational Closure Analysis ===\n");
    printf("Components: %d  |  Reactions: %d  |  Boundary: %d\n",
           sys->n_components, sys->n_reactions, sys->boundary_count);

    double ci = oc_closure_index(sys);
    printf("Closure Index: %.4f\n", ci);

    int is_closed = (ci > 0.99);
    printf("Organizational Closure: %s\n", is_closed ? "ACHIEVED" : "INCOMPLETE");

    int bp = oc_boundary_is_self_produced(sys);
    printf("Self-Produced Boundary: %s\n", bp ? "YES" : "NO");

    int food_set[OC_MAX_COMPONENTS];
    memset(food_set, 0, sizeof(food_set));
    int food_count = 0;
    for (int i = 0; i < sys->n_components; i++) {
        if (!sys->is_produced[i]) food_set[food_count++] = i;
    }
    int depths[OC_MAX_COMPONENTS];
    oc_dependency_depths(sys, food_set, food_count, depths);

    int max_depth = 0;
    for (int i = 0; i < sys->n_components; i++) {
        if (depths[i] > max_depth) max_depth = depths[i];
    }
    printf("Maximum Dependency Depth: %d\n", max_depth);

    int core[OC_MAX_COMPONENTS];
    int core_size;
    oc_identify_autopoietic_core(sys, core, &core_size);
    printf("Autopoietic Core Size: %d components\n", core_size);
    printf("========================================\n");
}
