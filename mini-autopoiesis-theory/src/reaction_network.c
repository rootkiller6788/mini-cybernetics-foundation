/**
 * @file reaction_network.c
 * @brief Reaction network construction, analysis, and manipulation.
 *
 * Implements the reaction network formalism for modeling autopoietic
 * organization. Each reaction transforms substrates into products,
 * potentially catalyzed by specific molecular species.
 *
 * Knowledge points implemented:
 *   L1 - rn_network_init / rn_add_species / rn_add_reaction: data structures
 *   L2 - rn_compute_reaction_rates: mass-action kinetics
 *   L2 - rn_compute_derivatives: ODE right-hand side
 *   L2 - rn_is_autocatalytic_species: autocatalysis detection
 *   L3 - rn_build_matrices: stoichiometric matrix construction
 *   L3 - rn_find_autocatalytic_cycles: graph cycle detection (DFS)
 *   L4 - rn_is_catalytically_closed: catalytic closure theorem
 *   L4 - rn_network_connectivity: network topology analysis
 *   L5 - rn_compute_production_set: iterative production reachability
 */

#include "reaction_network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ==========================================================================
 * Network Construction (L1)
 * ========================================================================== */

void rn_network_init(rn_network_t *net)
{
    if (!net) return;
    memset(net, 0, sizeof(rn_network_t));
}

int rn_add_species(rn_network_t *net, const char *name,
                   double initial_conc, double mol_weight)
{
    if (!net || !name) return -1;
    if (net->species_count >= RN_MAX_SPECIES) return -1;
    if (initial_conc < 0.0 || mol_weight <= 0.0) return -1;

    int idx = net->species_count;
    rn_species_t *sp = &net->species[idx];

    strncpy(sp->name, name, RN_MAX_NAME - 1);
    sp->name[RN_MAX_NAME - 1] = '\0';
    sp->initial_concentration = initial_conc;
    sp->concentration = initial_conc;
    sp->molecular_weight = mol_weight;
    sp->is_food = 0;
    sp->is_boundary = 0;
    sp->flags = 0;

    net->species_count++;
    return idx;
}

int rn_add_reaction(rn_network_t *net,
                    const int *reactants, const int *reactant_stoich,
                    int reactant_count,
                    const int *products, const int *product_stoich,
                    int product_count,
                    const int *catalysts, int catalyst_count,
                    double rate_constant)
{
    if (!net) return -1;
    if (net->reaction_count >= RN_MAX_REACTIONS) return -1;
    if (rate_constant <= 0.0) return -1;
    if (reactant_count > RN_MAX_REACTANTS || product_count > RN_MAX_PRODUCTS) return -1;
    if (catalyst_count > RN_MAX_CATALYSTS) return -1;

    int idx = net->reaction_count;
    rn_reaction_t *rxn = &net->reactions[idx];

    memset(rxn->label, 0, RN_MAX_NAME);
    snprintf(rxn->label, RN_MAX_NAME, "R%d", idx + 1);

    rxn->reactant_count = reactant_count;
    for (int i = 0; i < reactant_count; i++) {
        rxn->reactants[i] = (reactants) ? reactants[i] : -1;
        rxn->reactant_stoich[i] = (reactant_stoich) ? reactant_stoich[i] : 1;
    }

    rxn->product_count = product_count;
    for (int i = 0; i < product_count; i++) {
        rxn->products[i] = (products) ? products[i] : -1;
        rxn->product_stoich[i] = (product_stoich) ? product_stoich[i] : 1;
    }

    rxn->catalyst_count = catalyst_count;
    for (int i = 0; i < catalyst_count; i++) {
        rxn->catalysts[i] = (catalysts) ? catalysts[i] : -1;
    }

    rxn->rate_constant = rate_constant;
    rxn->is_reversible = 0;
    rxn->reverse_rate_constant = 0.0;

    net->reaction_count++;
    return idx;
}

void rn_build_matrices(rn_network_t *net)
{
    if (!net) return;

    /* Clear matrices */
    for (int i = 0; i < net->species_count; i++) {
        for (int j = 0; j < net->reaction_count; j++) {
            net->stoichiometry[i][j] = 0.0;
            net->catalysis[i][j] = 0;
        }
    }

    /* Populate stoichiometric matrix */
    for (int j = 0; j < net->reaction_count; j++) {
        const rn_reaction_t *rxn = &net->reactions[j];

        /* Reactants are consumed (negative stoichiometry) */
        for (int r = 0; r < rxn->reactant_count; r++) {
            int sp = rxn->reactants[r];
            if (sp >= 0 && sp < net->species_count) {
                net->stoichiometry[sp][j] -= (double)rxn->reactant_stoich[r];
            }
        }

        /* Products are produced (positive stoichiometry) */
        for (int p = 0; p < rxn->product_count; p++) {
            int sp = rxn->products[p];
            if (sp >= 0 && sp < net->species_count) {
                net->stoichiometry[sp][j] += (double)rxn->product_stoich[p];
            }
        }
    }

    /* Populate catalysis matrix */
    for (int j = 0; j < net->reaction_count; j++) {
        const rn_reaction_t *rxn = &net->reactions[j];
        for (int c = 0; c < rxn->catalyst_count; c++) {
            int cat = rxn->catalysts[c];
            if (cat >= 0 && cat < net->species_count) {
                net->catalysis[cat][j] = 1;
            }
        }
    }
}

/* ==========================================================================
 * Network Analysis (L2, L3)
 * ========================================================================== */

void rn_compute_reaction_rates(const rn_network_t *net, double *rates)
{
    if (!net || !rates) return;

    for (int j = 0; j < net->reaction_count; j++) {
        const rn_reaction_t *rxn = &net->reactions[j];
        double rate = rxn->rate_constant;

        /* Mass-action: rate = k * ∏ [reactant_i]^{stoich_i} */
        for (int r = 0; r < rxn->reactant_count; r++) {
            int sp = rxn->reactants[r];
            if (sp >= 0 && sp < net->species_count) {
                double conc = net->species[sp].concentration;
                int stoich = rxn->reactant_stoich[r];
                if (conc < 1e-15 && stoich > 0) {
                    rate = 0.0;
                    break;
                }
                for (int s = 0; s < stoich; s++) {
                    rate *= conc;
                }
            }
        }

        /* Catalyst enhancement: multiplicative factor */
        for (int c = 0; c < rxn->catalyst_count; c++) {
            int cat = rxn->catalysts[c];
            if (cat >= 0 && cat < net->species_count) {
                double cat_conc = net->species[cat].concentration;
                rate *= (1.0 + cat_conc);
            }
        }

        rates[j] = rate;
    }
}

void rn_compute_derivatives(const rn_network_t *net, double *derivatives)
{
    if (!net || !derivatives) return;

    double rates[RN_MAX_REACTIONS];
    rn_compute_reaction_rates(net, rates);

    /* dx/dt = S * v */
    for (int i = 0; i < net->species_count; i++) {
        derivatives[i] = 0.0;
        for (int j = 0; j < net->reaction_count; j++) {
            derivatives[i] += net->stoichiometry[i][j] * rates[j];
        }
        /* Add natural decay for non-food species */
        if (!net->species[i].is_food) {
            derivatives[i] -= 0.01 * net->species[i].concentration;
        }
    }
}

void rn_compute_production_set(const rn_network_t *net,
                                const uint64_t *available,
                                uint64_t *producible)
{
    if (!net || !available || !producible) return;

    uint64_t current_available = *available;
    int changed = 1;
    int max_iter = 1000;

    while (changed && max_iter-- > 0) {
        changed = 0;
        for (int j = 0; j < net->reaction_count; j++) {
            const rn_reaction_t *rxn = &net->reactions[j];

            /* Check all reactants are available */
            int all_available = 1;
            for (int r = 0; r < rxn->reactant_count; r++) {
                int sp = rxn->reactants[r];
                if (sp >= 0 && sp < 64) {
                    if (!(current_available & (1ULL << sp))) {
                        all_available = 0;
                        break;
                    }
                }
            }

            if (all_available) {
                /* Mark all products as available */
                for (int p = 0; p < rxn->product_count; p++) {
                    int sp = rxn->products[p];
                    if (sp >= 0 && sp < 64) {
                        uint64_t mask = 1ULL << sp;
                        if (!(current_available & mask)) {
                            current_available |= mask;
                            changed = 1;
                        }
                    }
                }
            }
        }
    }

    *producible = current_available;
}

int rn_is_autocatalytic_species(const rn_network_t *net, int species_id)
{
    if (!net || species_id < 0 || species_id >= net->species_count) return 0;

    /* Check if this species catalyzes a reaction that produces it */
    for (int j = 0; j < net->reaction_count; j++) {
        const rn_reaction_t *rxn = &net->reactions[j];

        /* Does this reaction produce species_id? */
        int produces_target = 0;
        for (int p = 0; p < rxn->product_count; p++) {
            if (rxn->products[p] == species_id) {
                produces_target = 1;
                break;
            }
        }

        if (!produces_target) continue;

        /* Does species_id catalyze this reaction? */
        for (int c = 0; c < rxn->catalyst_count; c++) {
            if (rxn->catalysts[c] == species_id) {
                return 1; /* Autocatalytic: catalyzes its own production */
            }
        }
    }

    return 0;
}

int rn_find_autocatalytic_cycles(const rn_network_t *net,
                                  int *cycles, int max_cycles)
{
    if (!net || !cycles || net->species_count < 2) return 0;

    /* Build catalysis graph adjacency: A[i][k] = 1 if species i catalyzes
     * the production of species k (via some reaction) */
    int n = net->species_count;
    int **adj = (int **)malloc(n * sizeof(int *));
    if (!adj) return 0;
    for (int i = 0; i < n; i++) {
        adj[i] = (int *)calloc(n, sizeof(int));
        if (!adj[i]) {
            for (int j = 0; j < i; j++) free(adj[j]);
            free(adj);
            return 0;
        }
    }

    /* Build adjacency: if species i is a catalyst of reaction j,
     * and reaction j produces species k, then i → k */
    for (int j = 0; j < net->reaction_count; j++) {
        const rn_reaction_t *rxn = &net->reactions[j];
        for (int c = 0; c < rxn->catalyst_count; c++) {
            int cat = rxn->catalysts[c];
            if (cat < 0 || cat >= n) continue;
            for (int p = 0; p < rxn->product_count; p++) {
                int prod = rxn->products[p];
                if (prod >= 0 && prod < n) {
                    adj[cat][prod] = 1;
                }
            }
        }
    }

    /* DFS-based cycle detection */
    int *visited = (int *)calloc(n, sizeof(int));
    int *on_stack = (int *)calloc(n, sizeof(int));
    int *path = (int *)malloc(n * sizeof(int));
    int path_len = 0;
    int cycle_count = 0;
    int *output_pos = cycles;

    if (!visited || !on_stack || !path) goto cleanup;

    /* Recursive DFS using explicit stack to avoid recursion limits */
    for (int start = 0; start < n && cycle_count < max_cycles; start++) {
        if (visited[start]) continue;

        int stack[256][2]; /* {node, next_neighbor_to_explore} */
        int stack_len = 0;

        stack[stack_len][0] = start;
        stack[stack_len][1] = 0;
        stack_len++;

        visited[start] = 1;
        on_stack[start] = 1;
        path[0] = start;
        path_len = 1;

        while (stack_len > 0 && cycle_count < max_cycles) {
            int node = stack[stack_len - 1][0];
            int next = stack[stack_len - 1][1];

            /* Find next unvisited neighbor */
            int found = 0;
            for (int k = next; k < n; k++) {
                if (adj[node][k]) {
                    if (!visited[k]) {
                        stack[stack_len - 1][1] = k + 1;
                        visited[k] = 1;
                        on_stack[k] = 1;
                        path[path_len++] = k;
                        stack[stack_len][0] = k;
                        stack[stack_len][1] = 0;
                        stack_len++;
                        found = 1;
                        break;
                    } else if (on_stack[k] && k == start) {
                        /* Found a cycle back to start */
                        for (int p = 0; p < path_len; p++) {
                            *output_pos++ = path[p];
                        }
                        *output_pos++ = -1; /* Terminator */
                        cycle_count++;
                        found = 1;
                        break;
                    }
                }
            }

            if (!found) {
                /* Backtrack */
                stack_len--;
                on_stack[node] = 0;
                path_len--;
            }
        }
    }

cleanup:
    for (int i = 0; i < n; i++) free(adj[i]);
    free(adj);
    free(visited);
    free(on_stack);
    free(path);

    return cycle_count;
}

/* ==========================================================================
 * Network Properties (L4)
 * ========================================================================== */

double rn_network_connectivity(const rn_network_t *net)
{
    if (!net || net->species_count == 0) return 0.0;

    int connected_pairs = 0;
    int total_pairs = net->species_count * net->species_count;

    for (int i = 0; i < net->species_count; i++) {
        for (int k = 0; k < net->species_count; k++) {
            if (i == k) { connected_pairs++; continue; }

            /* Check if species i and k are connected via any reaction */
            for (int j = 0; j < net->reaction_count; j++) {
                const rn_reaction_t *rxn = &net->reactions[j];
                int involves_i = 0, involves_k = 0;

                for (int r = 0; r < rxn->reactant_count; r++) {
                    if (rxn->reactants[r] == i) involves_i = 1;
                    if (rxn->reactants[r] == k) involves_k = 1;
                }
                for (int p = 0; p < rxn->product_count; p++) {
                    if (rxn->products[p] == i) involves_i = 1;
                    if (rxn->products[p] == k) involves_k = 1;
                }

                if (involves_i && involves_k) {
                    connected_pairs++;
                    break;
                }
            }
        }
    }

    return (double)connected_pairs / (double)total_pairs;
}

int rn_is_catalytically_closed(const rn_network_t *net)
{
    if (!net || net->reaction_count == 0) return 0;

    /* Catalytic closure: every reaction must be catalyzed by at least one
     * species that is producible within the network */

    /* First, identify all producible species (not in food set) */
    int *producible = (int *)calloc(net->species_count, sizeof(int));
    if (!producible) return 0;

    for (int j = 0; j < net->reaction_count; j++) {
        const rn_reaction_t *rxn = &net->reactions[j];
        for (int p = 0; p < rxn->product_count; p++) {
            int sp = rxn->products[p];
            if (sp >= 0 && sp < net->species_count) {
                producible[sp] = 1;
            }
        }
    }

    /* Check each reaction */
    for (int j = 0; j < net->reaction_count; j++) {
        const rn_reaction_t *rxn = &net->reactions[j];
        if (rxn->catalyst_count == 0) {
            /* Uncatalyzed reaction: allowed if it has non-zero rate */
            continue;
        }

        int has_internal_catalyst = 0;
        for (int c = 0; c < rxn->catalyst_count; c++) {
            int cat = rxn->catalysts[c];
            if (cat >= 0 && cat < net->species_count) {
                if (producible[cat] || !net->species[cat].is_food) {
                    has_internal_catalyst = 1;
                    break;
                }
            }
        }

        if (!has_internal_catalyst) {
            free(producible);
            return 0;
        }
    }

    free(producible);
    return 1;
}

double rn_autocatalytic_capacity(const rn_network_t *net)
{
    if (!net || net->species_count == 0) return 0.0;

    int autocat_count = 0;
    for (int i = 0; i < net->species_count; i++) {
        if (rn_is_autocatalytic_species(net, i)) {
            autocat_count++;
        }
    }

    return (double)autocat_count / (double)net->species_count;
}

/* ==========================================================================
 * Utility
 * ========================================================================== */

void rn_network_print(const rn_network_t *net)
{
    if (!net) { printf("Network: NULL\n"); return; }

    printf("=== Reaction Network ===\n");
    printf("Species: %d  |  Reactions: %d  |  Food: %d\n",
           net->species_count, net->reaction_count, net->food_count);

    printf("\nSpecies:\n");
    for (int i = 0; i < net->species_count; i++) {
        printf("  [%d] %-12s conc=%.4f mw=%.1f food=%d\n",
               i, net->species[i].name, net->species[i].concentration,
               net->species[i].molecular_weight, net->species[i].is_food);
    }

    printf("\nReactions:\n");
    for (int j = 0; j < net->reaction_count; j++) {
        const rn_reaction_t *rxn = &net->reactions[j];
        printf("  %s: k=%.3f  ", rxn->label, rxn->rate_constant);

        printf("reactants=[");
        for (int r = 0; r < rxn->reactant_count; r++) {
            printf("%s%d", r > 0 ? "," : "", rxn->reactants[r]);
        }
        printf("] → products=[");
        for (int p = 0; p < rxn->product_count; p++) {
            printf("%s%d", p > 0 ? "," : "", rxn->products[p]);
        }
        printf("]");

        if (rxn->catalyst_count > 0) {
            printf(" cat=[");
            for (int c = 0; c < rxn->catalyst_count; c++) {
                printf("%s%d", c > 0 ? "," : "", rxn->catalysts[c]);
            }
            printf("]");
        }
        printf("\n");
    }
    printf("========================\n");
}

void rn_network_destroy(rn_network_t *net)
{
    if (!net) return;
    memset(net, 0, sizeof(rn_network_t));
}
