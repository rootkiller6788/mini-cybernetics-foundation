/**
 * @file autopoiesis.c
 * @brief Core autopoiesis theory implementation.
 *
 * Implements the central data structures and algorithms for representing
 * and analyzing autopoietic systems as defined by Maturana & Varela.
 *
 * Knowledge points implemented (one per function):
 *   L1 - ap_system_init: system initialization
 *   L1 - ap_add_component: component registration
 *   L1 - ap_add_production: production relation creation
 *   L2 - ap_check_autopoiesis: autopoiesis criteria verification
 *   L2 - ap_compute_organizational_closure: closure quantification
 *   L2 - ap_evolve_step: discrete-time system evolution
 *   L2 - ap_classify_state: system state classification
 *   L2 - ap_apply_perturbation: perturbation response
 *   L2 - ap_structural_coupling_measure: coupling quantification
 *   L3 - ap_build_production_adjacency: adjacency matrix construction
 *   L3 - ap_production_transitive_closure: transitive closure (Warshall)
 *   L3 - ap_find_minimal_generators: minimal generating set
 *   L4 - ap_verify_self_produced_boundary: boundary self-production theorem
 */

#include "autopoiesis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ==========================================================================
 * L1: System Initialization and Component Management
 * ========================================================================== */

void ap_system_init(ap_system_t *sys)
{
    if (!sys) return;
    memset(sys, 0, sizeof(ap_system_t));
    sys->state = AP_STATE_INACTIVE;
    sys->time = 0.0;
    sys->total_mass = 0.0;
    sys->entropy = 0.0;
    sys->organization_measure = 0.0;
    sys->perturbation_count = 0;
    sys->adaptation_count = 0;

    /* Initialize boundary to default values */
    sys->boundary.component_count = 0;
    sys->boundary.permeability = 0.5;
    sys->boundary.surface_area = 1.0;
    sys->boundary.thickness = 0.1;
    sys->boundary.topology_size = 0;
}

int ap_add_component(ap_system_t *sys, const char *label,
                     ap_component_role_t role,
                     double concentration, double decay_rate)
{
    if (!sys || !label) return -1;
    if (sys->component_count >= AP_MAX_COMPONENTS) return -1;
    if (concentration < 0.0 || decay_rate < 0.0) return -1;

    int idx = sys->component_count;
    ap_component_t *comp = &sys->components[idx];

    strncpy(comp->label, label, AP_MAX_LABEL - 1);
    comp->label[AP_MAX_LABEL - 1] = '\0';
    comp->role = role;
    comp->concentration = concentration;
    comp->decay_rate = decay_rate;
    comp->flags = 0;
    comp->production_count = 0;
    comp->consumption_count = 0;

    /* Set boundary flag */
    if (role == AP_COMPONENT_BOUNDARY) {
        comp->flags |= AP_FLAG_BOUNDARY;
    }

    sys->total_mass += concentration;
    sys->component_count++;

    /* Update boundary topology if this is a boundary component */
    if (role == AP_COMPONENT_BOUNDARY && sys->boundary.component_count < AP_MAX_NEIGHBORS) {
        sys->boundary.component_ids[sys->boundary.component_count] = idx;
        sys->boundary.component_count++;
        sys->boundary.topology_size = sys->boundary.component_count;
    }

    return idx;
}

int ap_add_production(ap_system_t *sys, int producer_id, int product_id,
                      double rate, int catalyst_id,
                      const int *substrate_ids, int substrate_count)
{
    if (!sys) return -1;
    if (sys->production_count >= AP_MAX_REACTIONS) return -1;
    if (producer_id < 0 || producer_id >= sys->component_count) return -1;
    if (product_id < 0 || product_id >= sys->component_count) return -1;
    if (rate <= 0.0) return -1;
    if (substrate_count > 8) return -1;

    int idx = sys->production_count;
    ap_production_t *prod = &sys->productions[idx];

    prod->producer_id = producer_id;
    prod->product_id = product_id;
    prod->rate = rate;
    prod->catalyst_id = catalyst_id;
    prod->substrate_count = substrate_count;

    if (substrate_ids && substrate_count > 0) {
        for (int i = 0; i < substrate_count && i < 8; i++) {
            if (substrate_ids[i] >= 0 && substrate_ids[i] < sys->component_count) {
                prod->substrate_ids[i] = substrate_ids[i];
            } else {
                prod->substrate_ids[i] = -1;
            }
        }
    }

    /* Update component statistics */
    sys->components[product_id].production_count++;
    sys->components[product_id].flags |= AP_FLAG_SELF_PRODUCED;

    if (producer_id != product_id) {
        sys->components[producer_id].consumption_count++;
    }

    /* Mark products as essential if they are boundary components */
    if (sys->components[product_id].role == AP_COMPONENT_BOUNDARY) {
        sys->components[product_id].flags |= AP_FLAG_ESSENTIAL;
    }

    sys->production_count++;
    return idx;
}

/* ==========================================================================
 * L2: Core Autopoiesis Functions
 * ========================================================================== */

int ap_check_autopoiesis(const ap_system_t *sys)
{
    if (!sys || sys->component_count == 0) return 0;

    /* Criterion 1: Operational closure — all components must be produced
     * by reactions within the system. Check if every component has at
     * least one production reaction. */
    for (int i = 0; i < sys->component_count; i++) {
        /* Skip externally imported components */
        if (sys->components[i].flags & AP_FLAG_IMPORTED) continue;

        if (sys->components[i].production_count == 0) {
            return 0; /* Component i is not produced internally */
        }
    }

    /* Criterion 2: Boundary self-production — boundary components must
     * be producible from internal components only. */
    if (!ap_verify_self_produced_boundary(sys)) {
        return 0;
    }

    /* Criterion 3: Organization invariance — the network of productions
     * must be a connected whole (every component reachable from every other
     * via production relations). */
    if (sys->component_count > 1) {
        /* Build reachability via production graph */
        int n = sys->component_count;
        int *reachable = (int *)calloc(n, sizeof(int));
        if (!reachable) return 0;

        /* Start from first component, do BFS over production graph */
        int *queue = (int *)malloc(n * sizeof(int));
        if (!queue) { free(reachable); return 0; }

        int head = 0, tail = 0;
        queue[tail++] = 0;
        reachable[0] = 1;

        while (head < tail) {
            int current = queue[head++];
            for (int p = 0; p < sys->production_count; p++) {
                if (sys->productions[p].producer_id == current) {
                    int target = sys->productions[p].product_id;
                    if (!reachable[target] && target < n) {
                        reachable[target] = 1;
                        queue[tail++] = target;
                    }
                }
            }
        }

        /* Check all components are reachable */
        int all_reachable = 1;
        for (int i = 0; i < n; i++) {
            if (!reachable[i] && !(sys->components[i].flags & AP_FLAG_IMPORTED)) {
                all_reachable = 0;
                break;
            }
        }

        free(queue);
        free(reachable);
        if (!all_reachable) return 0;
    }

    return 1; /* All criteria satisfied */
}

double ap_compute_organizational_closure(const ap_system_t *sys)
{
    if (!sys || sys->component_count == 0) return 0.0;

    int self_produced = 0;
    int total = 0;

    for (int i = 0; i < sys->component_count; i++) {
        if (sys->components[i].flags & AP_FLAG_IMPORTED) continue;
        total++;
        if (sys->components[i].flags & AP_FLAG_SELF_PRODUCED) {
            self_produced++;
        }
    }

    if (total == 0) return 0.0;
    return (double)self_produced / (double)total;
}

void ap_evolve_step(ap_system_t *sys, double dt)
{
    if (!sys || dt <= 0.0) return;

    /* Compute concentration changes from production and decay */
    double delta[AP_MAX_COMPONENTS];
    memset(delta, 0, sizeof(delta));

    /* Production contributions */
    for (int p = 0; p < sys->production_count; p++) {
        const ap_production_t *prod = &sys->productions[p];
        int prod_id = prod->product_id;

        /* Mass-action: rate * producer_concentration * product(catalyst) */
        double rate = prod->rate;
        double producer_conc = sys->components[prod->producer_id].concentration;
        double reaction_rate = rate * producer_conc;

        /* Catalyst enhancement */
        if (prod->catalyst_id >= 0 && prod->catalyst_id < sys->component_count) {
            reaction_rate *= (1.0 + sys->components[prod->catalyst_id].concentration);
        }

        /* Substrate limitation: if any substrate is depleted, rate drops */
        int substrates_depleted = 0;
        for (int s = 0; s < prod->substrate_count; s++) {
            int sub_id = prod->substrate_ids[s];
            if (sub_id >= 0 && sub_id < sys->component_count) {
                if (sys->components[sub_id].concentration < 1e-10) {
                    substrates_depleted = 1;
                    break;
                }
            }
        }
        if (substrates_depleted) {
            reaction_rate *= 0.01; /* Severely reduced */
        }

        delta[prod_id] += reaction_rate * dt;
    }

    /* Decay contributions */
    for (int i = 0; i < sys->component_count; i++) {
        delta[i] -= sys->components[i].decay_rate * sys->components[i].concentration * dt;
    }

    /* Apply changes with non-negativity constraint */
    sys->total_mass = 0.0;
    for (int i = 0; i < sys->component_count; i++) {
        sys->components[i].concentration += delta[i];
        if (sys->components[i].concentration < 0.0) {
            sys->components[i].concentration = 0.0;
        }
        sys->total_mass += sys->components[i].concentration;
    }

    /* Update entropy (simplified: based on concentration distribution) */
    sys->entropy = 0.0;
    if (sys->total_mass > 1e-15) {
        for (int i = 0; i < sys->component_count; i++) {
            double p = sys->components[i].concentration / sys->total_mass;
            if (p > 1e-15) {
                sys->entropy -= p * log(p);
            }
        }
    }

    /* Update organization measure */
    sys->organization_measure = ap_compute_organizational_closure(sys);

    /* Classify state */
    sys->state = ap_classify_state(sys);

    sys->time += dt;
}

ap_system_state_t ap_classify_state(const ap_system_t *sys)
{
    if (!sys || sys->component_count == 0) return AP_STATE_INACTIVE;

    double total_conc = 0.0;
    double total_decay = 0.0;
    double total_production = 0.0;

    for (int i = 0; i < sys->component_count; i++) {
        total_conc += sys->components[i].concentration;
        total_decay += sys->components[i].decay_rate * sys->components[i].concentration;
    }

    /* Estimate total production from production relations */
    for (int p = 0; p < sys->production_count; p++) {
        total_production += sys->productions[p].rate *
            sys->components[sys->productions[p].producer_id].concentration;
    }

    if (total_conc < 1e-12) return AP_STATE_DISINTEGRATING;

    double net_change = total_production - total_decay;

    if (fabs(net_change) < 1e-10 * total_conc) {
        return AP_STATE_MAINTAINING;
    } else if (net_change > 0) {
        if (sys->organization_measure > 0.9) {
            return AP_STATE_SELF_PRODUCING;
        } else {
            return AP_STATE_ADAPTING;
        }
    } else {
        if (sys->organization_measure > 0.5) {
            return AP_STATE_DEGRADING;
        } else {
            return AP_STATE_DISINTEGRATING;
        }
    }
}

ap_system_state_t ap_apply_perturbation(ap_system_t *sys, int component_id,
                                         double delta)
{
    if (!sys || component_id < 0 || component_id >= sys->component_count) {
        return sys ? sys->state : AP_STATE_INACTIVE;
    }

    double old_conc = sys->components[component_id].concentration;
    sys->components[component_id].concentration += delta;
    if (sys->components[component_id].concentration < 0.0) {
        sys->components[component_id].concentration = 0.0;
    }

    /* Update total mass */
    sys->total_mass += (sys->components[component_id].concentration - old_conc);

    sys->perturbation_count++;

    /* Recompute organization measure after perturbation */
    sys->organization_measure = ap_compute_organizational_closure(sys);

    /* Classify new state */
    sys->state = ap_classify_state(sys);

    /* Track adaptation: if system returned to maintaining after perturbation */
    if (sys->state == AP_STATE_MAINTAINING || sys->state == AP_STATE_ADAPTING) {
        sys->adaptation_count++;
    }

    return sys->state;
}

double ap_structural_coupling_measure(const ap_system_t *sys1,
                                       const ap_system_t *sys2)
{
    if (!sys1 || !sys2) return 0.0;

    /* Structural coupling is measured by the degree of overlap in
     * component profiles and the history of mutual perturbations. */

    /* Component similarity (Jaccard index over active components) */
    int overlap = 0;
    int union_size = 0;

    /* Count components that are present in both systems */
    for (int i = 0; i < sys1->component_count && i < sys2->component_count; i++) {
        int present1 = (sys1->components[i].concentration > 1e-10);
        int present2 = (sys2->components[i].concentration > 1e-10);
        if (present1 || present2) union_size++;
        if (present1 && present2) overlap++;
    }

    double component_similarity = 0.0;
    if (union_size > 0) {
        component_similarity = (double)overlap / (double)union_size;
    }

    /* Organizational similarity */
    double org_diff = fabs(sys1->organization_measure - sys2->organization_measure);
    double org_similarity = 1.0 - org_diff;

    /* Interaction history (normalized perturbation counts) */
    double interaction_factor = 0.0;
    if (sys1->perturbation_count + sys2->perturbation_count > 0) {
        interaction_factor = (double)(sys1->adaptation_count + sys2->adaptation_count) /
            (double)(sys1->perturbation_count + sys2->perturbation_count + 1);
    }

    /* Combined coupling measure */
    return 0.4 * component_similarity + 0.4 * org_similarity + 0.2 * interaction_factor;
}

void ap_system_destroy(ap_system_t *sys)
{
    if (!sys) return;
    /* Currently using stack-allocated arrays, nothing to free.
     * If dynamic allocation is added in the future, clean up here. */
    memset(sys, 0, sizeof(ap_system_t));
}

/* ==========================================================================
 * L3: Mathematical Structure Functions
 * ========================================================================== */

void ap_build_production_adjacency(const ap_system_t *sys,
                                    int *matrix, int n)
{
    if (!sys || !matrix || n <= 0) return;

    /* Initialize to zeros */
    for (int i = 0; i < n * n; i++) matrix[i] = 0;

    for (int p = 0; p < sys->production_count; p++) {
        int from = sys->productions[p].producer_id;
        int to = sys->productions[p].product_id;
        if (from >= 0 && from < n && to >= 0 && to < n) {
            matrix[from * n + to] = 1;
        }
    }
}

void ap_production_transitive_closure(const ap_system_t *sys,
                                       const int *initial_set,
                                       int initial_count,
                                       int *closure_set,
                                       int *closure_count)
{
    if (!sys || !initial_set || !closure_set || !closure_count) return;

    int n = sys->component_count;
    int *producible = (int *)calloc(n, sizeof(int));
    if (!producible) { *closure_count = 0; return; }

    /* Mark initial set as producible */
    for (int i = 0; i < initial_count; i++) {
        int idx = initial_set[i];
        if (idx >= 0 && idx < n) {
            producible[idx] = 1;
        }
    }

    /* Iteratively apply productions until fixed point */
    int changed = 1;
    int max_iter = n * n; /* Safety bound */

    while (changed && max_iter-- > 0) {
        changed = 0;
        for (int p = 0; p < sys->production_count; p++) {
            const ap_production_t *prod = &sys->productions[p];

            /* Check that all substrates are producible */
            int all_substrates_available = 1;
            for (int s = 0; s < prod->substrate_count; s++) {
                int sub_id = prod->substrate_ids[s];
                if (sub_id >= 0 && sub_id < n && !producible[sub_id]) {
                    all_substrates_available = 0;
                    break;
                }
            }

            if (all_substrates_available) {
                int product_id = prod->product_id;
                if (product_id >= 0 && product_id < n && !producible[product_id]) {
                    producible[product_id] = 1;
                    changed = 1;
                }
            }
        }
    }

    /* Collect results */
    int count = 0;
    for (int i = 0; i < n && count < AP_MAX_COMPONENTS; i++) {
        if (producible[i]) {
            closure_set[count++] = i;
        }
    }
    *closure_count = count;

    free(producible);
}

int ap_find_minimal_generators(const ap_system_t *sys,
                                int *generators, int *gen_count)
{
    if (!sys || !generators || !gen_count) return -1;
    if (sys->component_count == 0) { *gen_count = 0; return 0; }

    int n = sys->component_count;

    /* Greedy heuristic: start with all components, iteratively remove
     * those that are not needed for producing the rest. */
    int *is_generator = (int *)calloc(n, sizeof(int));
    if (!is_generator) return -1;

    /* Start by marking all as generators */
    for (int i = 0; i < n; i++) is_generator[i] = 1;

    /* For each component, check if removing it still allows production
     * of all other components */
    for (int i = 0; i < n; i++) {
        /* Temporarily remove i from generator set */
        is_generator[i] = 0;

        /* Build set of remaining generators */
        int init_set[AP_MAX_COMPONENTS];
        int init_count = 0;
        for (int j = 0; j < n; j++) {
            if (is_generator[j]) {
                init_set[init_count++] = j;
            }
        }

        /* Compute what can be produced from remaining generators */
        int closure_idx[AP_MAX_COMPONENTS];
        int closure_size;
        ap_production_transitive_closure(sys, init_set, init_count,
                                          closure_idx, &closure_size);

        /* Check if all components are still producible */
        int all_covered = 1;
        for (int j = 0; j < n; j++) {
            int found = 0;
            for (int k = 0; k < closure_size; k++) {
                if (closure_idx[k] == j) { found = 1; break; }
            }
            if (!found) { all_covered = 0; break; }
        }

        if (!all_covered) {
            /* Need i as a generator */
            is_generator[i] = 1;
        }
    }

    /* Collect results */
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (is_generator[i]) {
            generators[count++] = i;
        }
    }
    *gen_count = count;

    free(is_generator);
    return 0;
}

int ap_verify_self_produced_boundary(const ap_system_t *sys)
{
    if (!sys) return 0;
    if (sys->boundary.component_count == 0) {
        /* No boundary defined: cannot verify */
        /* For a system with no explicit boundary, consider it vacuously true */
        return 1;
    }

    /* Check that every boundary component is produced by some production
     * using only internal components as substrates. */
    for (int b = 0; b < sys->boundary.component_count; b++) {
        int boundary_comp = sys->boundary.component_ids[b];
        if (boundary_comp < 0 || boundary_comp >= sys->component_count) continue;

        int is_boundary_produced = 0;

        for (int p = 0; p < sys->production_count; p++) {
            const ap_production_t *prod = &sys->productions[p];
            if (prod->product_id != boundary_comp) continue;

            /* Check that all substrates are internal (not imported) */
            int all_internal_substrates = 1;
            for (int s = 0; s < prod->substrate_count; s++) {
                int sub_id = prod->substrate_ids[s];
                if (sub_id >= 0 && sub_id < sys->component_count) {
                    if (sys->components[sub_id].flags & AP_FLAG_IMPORTED) {
                        all_internal_substrates = 0;
                        break;
                    }
                }
            }

            if (all_internal_substrates) {
                is_boundary_produced = 1;
                break;
            }
        }

        if (!is_boundary_produced) {
            return 0; /* Boundary component not self-produced */
        }
    }

    return 1;
}

/* ==========================================================================
 * Utility / Introspection
 * ========================================================================== */

void ap_system_print_summary(const ap_system_t *sys)
{
    if (!sys) { printf("System: NULL\n"); return; }

    printf("=== Autopoietic System Summary ===\n");
    printf("Components: %d  |  Productions: %d\n",
           sys->component_count, sys->production_count);
    printf("State: ");
    switch (sys->state) {
        case AP_STATE_INACTIVE:        printf("INACTIVE\n"); break;
        case AP_STATE_SELF_PRODUCING:  printf("SELF-PRODUCING\n"); break;
        case AP_STATE_MAINTAINING:     printf("MAINTAINING\n"); break;
        case AP_STATE_DEGRADING:       printf("DEGRADING\n"); break;
        case AP_STATE_ADAPTING:        printf("ADAPTING\n"); break;
        case AP_STATE_DISINTEGRATING:  printf("DISINTEGRATING\n"); break;
        default:                       printf("UNKNOWN\n"); break;
    }
    printf("Time: %.3f  |  Total Mass: %.6f  |  Entropy: %.6f\n",
           sys->time, sys->total_mass, sys->entropy);
    printf("Organization: %.4f  |  Boundary: %d components (perm=%.2f)\n",
           sys->organization_measure, sys->boundary.component_count,
           sys->boundary.permeability);
    printf("Perturbations: %d  |  Adaptations: %d\n",
           sys->perturbation_count, sys->adaptation_count);
    printf("Is Autopoietic: %s\n",
           ap_check_autopoiesis(sys) ? "YES" : "NO");
    printf("==================================\n");
}

int ap_count_essential_components(const ap_system_t *sys)
{
    if (!sys) return 0;
    int count = 0;
    for (int i = 0; i < sys->component_count; i++) {
        if (sys->components[i].flags & AP_FLAG_ESSENTIAL) count++;
    }
    return count;
}

double ap_self_production_fraction(const ap_system_t *sys)
{
    if (!sys || sys->component_count == 0) return 0.0;
    int sp = 0, total = 0;
    for (int i = 0; i < sys->component_count; i++) {
        if (!(sys->components[i].flags & AP_FLAG_IMPORTED)) {
            total++;
            if (sys->components[i].flags & AP_FLAG_SELF_PRODUCED) sp++;
        }
    }
    if (total == 0) return 1.0;
    return (double)sp / (double)total;
}
