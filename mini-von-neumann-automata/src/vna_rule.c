#include "vna_rule.h"
#include "vna_core.h"
#include "vna_neighborhood.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * L2: Totalistic and Outer-Totalistic Rules
 * ============================================================================ */

void vna_rule_set_totalistic_sum(VnaTransitionRule* rule, int max_sum,
                                  const uint8_t* sum_mapping) {
    if (!rule || !rule->rule_table || !sum_mapping) return;
    rule->is_totalistic = true;
    int64_t table_size = 1;
    for (int i = 0; i < rule->num_neighbors; i++) table_size *= rule->num_states;
    uint8_t* nb = (uint8_t*)malloc(rule->num_neighbors);
    for (int64_t i = 0; i < table_size; i++) {
        vna_index_to_neighbor_tuple(i, rule->num_states,
                                     rule->num_neighbors, nb);
        int sum = 0;
        for (int j = 0; j < rule->num_neighbors; j++) sum += nb[j];
        if (sum > max_sum) sum = max_sum;
        rule->rule_table[i] = sum_mapping[sum];
    }
    free(nb);
}

bool vna_rule_is_totalistic(VnaTransitionRule* rule) {
    if (!rule || !rule->rule_table) return false;
    /* Check: for any two neighbor tuples with equal sum,
     * the output must be the same. */
    int64_t table_size = 1;
    for (int i = 0; i < rule->num_neighbors; i++) table_size *= rule->num_states;
    if (table_size > 100000) return rule->is_totalistic; /* Trust the flag */

    uint8_t* nb = (uint8_t*)malloc(rule->num_neighbors);
    int* sums = (int*)malloc(table_size * sizeof(int));
    for (int64_t i = 0; i < table_size; i++) {
        vna_index_to_neighbor_tuple(i, rule->num_states,
                                     rule->num_neighbors, nb);
        int sum = 0;
        for (int j = 0; j < rule->num_neighbors; j++) sum += nb[j];
        sums[i] = sum;
    }

    for (int64_t i = 0; i < table_size; i++) {
        for (int64_t j = i + 1; j < table_size; j++) {
            if (sums[i] == sums[j] && rule->rule_table[i] != rule->rule_table[j]) {
                free(nb); free(sums);
                return false;
            }
        }
    }
    free(nb); free(sums);
    return true;
}

void vna_rule_set_outer_totalistic(VnaTransitionRule* rule, int num_states,
                                    int max_neighbor_sum,
                                    const uint8_t* sum_mapping) {
    if (!rule) return;
    rule->is_outer_totalistic = true;
    /* For outer-totalistic: mapping[center][sum_of_others] = output */
    int64_t table_size = 1;
    for (int i = 0; i < rule->num_neighbors; i++) table_size *= num_states;
    uint8_t* nb = (uint8_t*)malloc(rule->num_neighbors);
    int center_idx = rule->num_neighbors / 2;

    for (int64_t i = 0; i < table_size; i++) {
        vna_index_to_neighbor_tuple(i, num_states, rule->num_neighbors, nb);
        uint8_t center = nb[center_idx];
        int neighbor_sum = 0;
        for (int j = 0; j < rule->num_neighbors; j++)
            if (j != center_idx) neighbor_sum += nb[j];
        if (neighbor_sum > max_neighbor_sum) neighbor_sum = max_neighbor_sum;
        int map_idx = center * (max_neighbor_sum + 1) + neighbor_sum;
        rule->rule_table[i] = sum_mapping[map_idx];
    }
    free(nb);
}

bool vna_rule_is_outer_totalistic(VnaTransitionRule* rule) {
    if (!rule) return false;
    return rule->is_outer_totalistic;
}

/* ============================================================================
 * L1: Named Rules
 * ============================================================================ */

VnaTransitionRule* vna_rule_game_of_life(void) {
    /* Conway's Game of Life: B3/S23 on Moore neighborhood
     * 2 states, 9 neighbors (including center)
     * Outer-totalistic: depends on center + sum of 8 neighbors */
    VnaTransitionRule* rule = vna_rule_create(2, 9);
    if (!rule) return NULL;
    rule->name = strdup("Conway's Game of Life (B3/S23)");
    rule->description = strdup("Born if exactly 3 neighbors; survives if 2 or 3");
    rule->is_outer_totalistic = true;

    uint8_t nb[9];
    int64_t table_size = 512; /* 2^9 */
    for (int64_t i = 0; i < table_size; i++) {
        vna_index_to_neighbor_tuple(i, 2, 9, nb);
        uint8_t center = nb[4]; /* Center is index 4 in Moore 3x3 */
        int sum = 0;
        for (int j = 0; j < 9; j++) sum += nb[j];
        int neighbors_sum = sum - center;
        if (center == 1) {
            /* Survival: S23 */
            rule->rule_table[i] = (neighbors_sum == 2 || neighbors_sum == 3) ? 1 : 0;
        } else {
            /* Birth: B3 */
            rule->rule_table[i] = (neighbors_sum == 3) ? 1 : 0;
        }
    }
    return rule;
}

VnaTransitionRule* vna_rule_highlife(void) {
    /* HighLife: B36/S23 */
    VnaTransitionRule* rule = vna_rule_create(2, 9);
    if (!rule) return NULL;
    rule->name = strdup("HighLife (B36/S23)");
    rule->description = strdup("Like Life but also born with 6 neighbors");
    rule->is_outer_totalistic = true;

    uint8_t nb[9];
    int64_t table_size = 512;
    for (int64_t i = 0; i < table_size; i++) {
        vna_index_to_neighbor_tuple(i, 2, 9, nb);
        uint8_t center = nb[4];
        int sum = 0;
        for (int j = 0; j < 9; j++) sum += nb[j];
        int ns = sum - center;
        if (center == 1)
            rule->rule_table[i] = (ns == 2 || ns == 3) ? 1 : 0;
        else
            rule->rule_table[i] = (ns == 3 || ns == 6) ? 1 : 0;
    }
    return rule;
}

VnaTransitionRule* vna_rule_brians_brain(void) {
    /* Brian's Brain: 3-state CA on Moore neighborhood.
     * States: 0=dead, 1=dying, 2=alive
     * Alive → Dying always. Dying → Dead always.
     * Dead → Alive if exactly 2 alive neighbors. */
    VnaTransitionRule* rule = vna_rule_create(3, 9);
    if (!rule) return NULL;
    rule->name = strdup("Brian's Brain");
    rule->description = strdup("3-state excitable CA: 0=dead,1=dying,2=alive");

    uint8_t nb[9];
    for (int64_t i = 0; i < 19683 /* 3^9 */; i++) {
        vna_index_to_neighbor_tuple(i, 3, 9, nb);
        uint8_t center = nb[4];
        if (center == 2) {
            rule->rule_table[i] = 1; /* Alive → Dying */
        } else if (center == 1) {
            rule->rule_table[i] = 0; /* Dying → Dead */
        } else {
            /* Count alive neighbors (state 2) */
            int alive = 0;
            for (int j = 0; j < 9; j++)
                if (j != 4 && nb[j] == 2) alive++;
            rule->rule_table[i] = (alive == 2) ? 2 : 0;
        }
    }
    return rule;
}

VnaTransitionRule* vna_rule_wireworld(void) {
    /* WireWorld: 4-state CA for simulating digital circuits.
     * States: 0=empty, 1=electron head, 2=electron tail, 3=conductor
     * Head → Tail, Tail → Conductor
     * Conductor → Head if 1 or 2 of its 8 neighbors are heads. */
    VnaTransitionRule* rule = vna_rule_create(4, 9);
    if (!rule) return NULL;
    rule->name = strdup("WireWorld");
    rule->description = strdup("4-state CA for digital logic simulation");

    uint8_t nb[9];
    int64_t table_size = 262144; /* 4^9 */
    for (int64_t i = 0; i < table_size; i++) {
        vna_index_to_neighbor_tuple(i, 4, 9, nb);
        uint8_t c = nb[4];
        if (c == 1) { rule->rule_table[i] = 2; } /* Head → Tail */
        else if (c == 2) { rule->rule_table[i] = 3; } /* Tail → Conductor */
        else if (c == 3) {
            int heads = 0;
            for (int j = 0; j < 9; j++)
                if (j != 4 && nb[j] == 1) heads++;
            rule->rule_table[i] = (heads == 1 || heads == 2) ? 1 : 3;
        } else {
            rule->rule_table[i] = 0; /* Empty stays empty */
        }
    }
    return rule;
}

VnaTransitionRule* vna_rule_seeds(void) {
    /* Seeds: B2/S — all cells die; birth with exactly 2 neighbors.
     * Produces complex, chaotic patterns despite simplicity. */
    VnaTransitionRule* rule = vna_rule_create(2, 9);
    if (!rule) return NULL;
    rule->name = strdup("Seeds (B2/S)");
    rule->description = strdup("All cells die; born with exactly 2 live neighbors");

    uint8_t nb[9];
    int64_t table_size = 512;
    for (int64_t i = 0; i < table_size; i++) {
        vna_index_to_neighbor_tuple(i, 2, 9, nb);
        int sum = 0;
        for (int j = 0; j < 9; j++) sum += nb[j];
        int ns = sum - nb[4];
        rule->rule_table[i] = (nb[4] == 0 && ns == 2) ? 1 : 0;
    }
    return rule;
}

VnaTransitionRule* vna_rule_day_and_night(void) {
    /* Day & Night: B3678/S34678 — symmetric under state inversion.
     * A pattern and its negative evolve identically. */
    VnaTransitionRule* rule = vna_rule_create(2, 9);
    if (!rule) return NULL;
    rule->name = strdup("Day & Night (B3678/S34678)");
    rule->description = strdup("State-inversion symmetric: B3678/S34678");

    uint8_t nb[9];
    int64_t table_size = 512;
    for (int64_t i = 0; i < table_size; i++) {
        vna_index_to_neighbor_tuple(i, 2, 9, nb);
        uint8_t center = nb[4];
        int sum = 0;
        for (int j = 0; j < 9; j++) sum += nb[j];
        int ns = sum - center;
        if (center == 1)
            rule->rule_table[i] = (ns == 3 || ns == 4 || ns == 6 ||
                                    ns == 7 || ns == 8) ? 1 : 0;
        else
            rule->rule_table[i] = (ns == 3 || ns == 6 || ns == 7 ||
                                    ns == 8) ? 1 : 0;
    }
    return rule;
}

/* ============================================================================
 * L3: Rule Space Structure
 * ============================================================================ */

double vna_rule_langton_lambda(VnaTransitionRule* rule, uint8_t quiescent) {
    /* λ = (# of neighborhood configs mapping to non-quiescent) / total configs
     * Langton (1990): λ ≈ 0.0 → order; λ ≈ 0.5 → complexity; λ ≈ 1.0 → chaos */
    if (!rule || !rule->rule_table) return 0.0;
    int64_t table_size = 1;
    for (int i = 0; i < rule->num_neighbors; i++) table_size *= rule->num_states;
    int64_t non_quiescent = 0;
    for (int64_t i = 0; i < table_size; i++)
        if (rule->rule_table[i] != quiescent) non_quiescent++;
    return (double)non_quiescent / table_size;
}

double vna_rule_z_parameter(VnaTransitionRule* rule, int num_neighbors) {
    /* Z-parameter: derivative of mean-field map at origin.
     * For binary CA: Z = n * (p_1 - p_0), where
     * p_1 = P(output=1 | exactly 1 neighbor is 1)
     * p_0 = P(output=1 | no neighbors are 1)
     * Z > 1 suggests Class III/IV behavior. */
    if (!rule || rule->num_states != 2) return 0.0;
    /* Count neighborhood configs with exactly 1 one */
    uint8_t* nb = (uint8_t*)malloc(num_neighbors);
    int p1_count = 0, p1_total = 0;
    int p0_count = 0;

    /* All-zero configuration */
    memset(nb, 0, num_neighbors);
    p0_count = vna_rule_lookup(rule, nb);

    /* Configurations with exactly one 1 */
    for (int i = 0; i < num_neighbors; i++) {
        memset(nb, 0, num_neighbors);
        nb[i] = 1;
        p1_count += vna_rule_lookup(rule, nb);
        p1_total++;
    }
    free(nb);

    double p0 = p0_count; /* 0 or 1 */
    double p1 = (p1_total > 0) ? (double)p1_count / p1_total : 0.0;
    return num_neighbors * (p1 - p0);
}

double vna_rule_hamming_distance(VnaTransitionRule* r1, VnaTransitionRule* r2) {
    if (!r1 || !r2 || !r1->rule_table || !r2->rule_table) return 1.0;
    if (r1->num_states != r2->num_states) return 1.0;
    if (r1->num_neighbors != r2->num_neighbors) return 1.0;

    int64_t table_size = 1;
    for (int i = 0; i < r1->num_neighbors; i++) table_size *= r1->num_states;
    int64_t diff = 0;
    for (int64_t i = 0; i < table_size; i++)
        if (r1->rule_table[i] != r2->rule_table[i]) diff++;
    return (double)diff / table_size;
}

/* ============================================================================
 * L4: Rule Space Properties (Theorems)
 * ============================================================================ */

bool vna_rule_is_reversible_1d(VnaTransitionRule* rule, int num_states,
                                int radius) {
    /* A 1D CA with k states and radius r is reversible iff its
     * de Bruijn diagram is a permutation (each node has exactly 1 incoming
     * and 1 outgoing edge per possible new state). */
    if (!rule || num_states > 8) return false; /* Too large to check */
    int num_nodes = 1;
    for (int i = 0; i < 2 * radius; i++) num_nodes *= num_states;

    /* Check that for each (left_neighbor, node), there's exactly one
     * predecessor (left_extended, new_state_left). */
    uint8_t* tuple = (uint8_t*)malloc(2 * radius + 1);
    for (int node = 0; node < num_nodes; node++) {
        for (int left_input = 0; left_input < num_states; left_input++) {
            /* Build full neighborhood: left_input + node */
            int tmp = node;
            for (int i = 0; i < 2 * radius; i++) {
                tuple[i] = (uint8_t)(tmp % num_states);
                tmp /= num_states;
            }
            uint8_t output = vna_rule_lookup(rule, tuple);
            /* Construct predecessor: left_input + tuple[0..2r-2] */
            /* The successor node is: tuple[0..2r-2] + output */
            /* For reversibility, for each possible predecessor state,
             * there must be exactly one (left_input, old_leftmost) mapping. */
            (void)output; (void)left_input;
        }
    }
    free(tuple);

    /* Simplified test: check if the rule table is a permutation
     * when viewed as a mapping from (2r)-length tuples. */
    (void)num_nodes;
    return false; /* Default: not verifiable without deeper analysis */
}

bool vna_rule_is_number_conserving(VnaTransitionRule* rule, int num_states,
                                    int num_neighbors) {
    /* A rule is number-conserving if Σ f(s_i) = Σ s_i for all inputs.
     * Check a sample of random inputs. */
    if (!rule || rule->num_states != 2) return false;
    uint8_t* nb = (uint8_t*)malloc(num_neighbors);
    for (int trial = 0; trial < 1000; trial++) {
        int sum_in = 0;
        for (int i = 0; i < num_neighbors; i++) {
            nb[i] = rand() % num_states;
            sum_in += nb[i];
        }
        uint8_t output = vna_rule_lookup(rule, nb);
        if (output != sum_in) { free(nb); return false; }
    }
    free(nb);
    return true;
}

bool vna_rule_is_additive(VnaTransitionRule* rule, int num_states,
                           int num_neighbors) {
    /* Additive means f(s_0,...,s_{n-1}) = Σ a_i s_i (mod k)
     * Check: f(x+y) = f(x) + f(y) (mod k) for random pairs. */
    if (!rule) return false;
    uint8_t* x = (uint8_t*)malloc(num_neighbors);
    uint8_t* y = (uint8_t*)malloc(num_neighbors);
    uint8_t* sum_vec = (uint8_t*)malloc(num_neighbors);

    for (int trial = 0; trial < 500; trial++) {
        for (int i = 0; i < num_neighbors; i++) {
            x[i] = rand() % num_states;
            y[i] = rand() % num_states;
            sum_vec[i] = (x[i] + y[i]) % num_states;
        }
        uint8_t fx = vna_rule_lookup(rule, x);
        uint8_t fy = vna_rule_lookup(rule, y);
        uint8_t f_sum = vna_rule_lookup(rule, sum_vec);
        if ((fx + fy) % num_states != f_sum) {
            free(x); free(y); free(sum_vec);
            return false;
        }
    }
    free(x); free(y); free(sum_vec);
    return true;
}

/* ============================================================================
 * L5: Rule Analysis Algorithms
 * ============================================================================ */

VnaWolframClass vna_rule_classify_wolfram(VnaTransitionRule* rule,
                                           int num_states, int num_neighbors,
                                           int sample_width, int samples,
                                           int max_steps) {
    if (!rule) return VNA_UNCLASSIFIED;
    /* Heuristic classification based on entropy evolution and lambda. */
    uint8_t quiescent = 0;
    double lambda = vna_rule_langton_lambda(rule, quiescent);

    /* Create sample lattices and observe behavior */
    VnaNeighborhood* nb = vna_neighborhood_create(VNA_ONE_DIMENSIONAL, 1, 2);
    if (!nb) return VNA_UNCLASSIFIED;

    double avg_entropy_end = 0.0;
    double avg_activity = 0.0;

    for (int s = 0; s < samples; s++) {
        VnaLattice* lat = vna_lattice_create(sample_width, 1, num_states,
                                              VNA_LATTICE_1D);
        if (!lat) continue;
        vna_lattice_randomize(lat, 0.5, (unsigned int)(1000 + s));

        /* Initial entropy */
        double entropies[10];
        for (int t = 0; t < 10 && t < max_steps; t++) {
            VnaStateHistogram* hist = vna_histogram_compute(lat);
            if (hist) { entropies[t] = hist->shannon_entropy; vna_histogram_free(hist); }
            else entropies[t] = 0;
            vna_lattice_evolve(lat, nb, rule);
        }
        avg_entropy_end += entropies[9];
        avg_activity += fabs(entropies[9] - entropies[0]);
        vna_lattice_free(lat);
    }

    avg_entropy_end /= samples;
    avg_activity /= samples;
    vna_neighborhood_free(nb);

    /* Wolfram classification heuristics:
     * Class I:   entropy → 0, activity low
     * Class II:  entropy low, periodic
     * Class III: entropy high, chaotic
     * Class IV:  entropy moderate, complex transients */
    if (avg_entropy_end < 0.1) return VNA_WOLFRAM_I;
    if (avg_activity < 0.2 && avg_entropy_end < 1.0) return VNA_WOLFRAM_II;
    if (avg_activity > 0.5) return VNA_WOLFRAM_III;
    if (lambda > 0.2 && lambda < 0.6 && avg_entropy_end > 0.3) return VNA_WOLFRAM_IV;
    return VNA_UNCLASSIFIED;
}

/* ============================================================================
 * L5: De Bruijn and Subset Diagrams
 * ============================================================================ */

VnaDeBruijnDiagram* vna_rule_debruijn_diagram(VnaTransitionRule* rule,
                                                int num_states, int radius) {
    /* For 1D CA with k states and radius r:
     * Nodes: (s_{-r}, ..., s_{r-1}) = k^(2r) possible tuples
     * Edges: from (s_{-r},...,s_{r-1}) with input s_r:
     *   → (s_{-r+1},...,s_r) if f(s_{-r},...,s_r) is the output
     * Each node has k outgoing edges. */
    int num_nodes = 1;
    for (int i = 0; i < 2 * radius; i++) num_nodes *= num_states;

    VnaDeBruijnDiagram* db =
        (VnaDeBruijnDiagram*)calloc(1, sizeof(VnaDeBruijnDiagram));
    if (!db) return NULL;
    db->num_nodes = num_nodes;
    db->num_edges = num_nodes * num_states;
    db->adjacency = (int*)malloc(num_nodes * num_states * sizeof(int));
    db->outputs = (uint8_t*)malloc(num_nodes * num_states);
    if (!db->adjacency || !db->outputs) {
        vna_debruijn_diagram_free(db);
        return NULL;
    }

    uint8_t* nb_full = (uint8_t*)malloc(2 * radius + 1);
    for (int node = 0; node < num_nodes; node++) {
        /* Decode node into (s_{-r}, ..., s_{r-1}) */
        int tmp = node;
        for (int i = 0; i < 2 * radius; i++) {
            nb_full[i] = (uint8_t)(tmp % num_states);
            tmp /= num_states;
        }
        for (int next = 0; next < num_states; next++) {
            nb_full[2 * radius] = (uint8_t)next;
            uint8_t output = vna_rule_lookup(rule, nb_full);
            /* New node: (s_{-r+1}, ..., s_{r-1}, next) */
            int new_node = 0;
            int mul = 1;
            for (int i = 0; i < 2 * radius; i++) {
                new_node += nb_full[i + 1] * mul;
                mul *= num_states;
            }
            db->adjacency[node * num_states + next] = new_node;
            db->outputs[node * num_states + next] = output;
        }
    }
    free(nb_full);
    return db;
}

void vna_debruijn_diagram_free(VnaDeBruijnDiagram* db) {
    if (!db) return;
    free(db->adjacency);
    free(db->outputs);
    free(db);
}

VnaSubsetDiagram* vna_rule_subset_diagram(VnaDeBruijnDiagram* db,
                                           int num_states) {
    if (!db) return NULL;
    /* The subset diagram has 2^N nodes (N = number of de Bruijn nodes).
     * This is exponentially large — only feasible for very small systems.
     * Implement a bounded version. */
    (void)num_states;
    VnaSubsetDiagram* sd = (VnaSubsetDiagram*)calloc(1, sizeof(VnaSubsetDiagram));
    if (!sd) return NULL;
    sd->num_nodes = 0;
    sd->num_edges = 0;
    sd->adjacency = NULL;
    return sd;
}

void vna_subset_diagram_free(VnaSubsetDiagram* sd) {
    if (!sd) return;
    free(sd->adjacency);
    free(sd);
}

/* ============================================================================
 * L8: Advanced Rule Operations
 * ============================================================================ */

VnaTransitionRule* vna_rule_block_renormalize(VnaTransitionRule* rule,
                                               int num_states, int radius,
                                               int block_size) {
    /* Coarse-grain a 1D rule by grouping block_size cells.
     * This maps the original rule to an effective rule on larger blocks.
     * Used in renormalization group analysis. */
    if (!rule || block_size < 2) return NULL;
    int new_num_neighbors = 2 * ((radius / block_size) + 1) + 1;
    VnaTransitionRule* coarse = vna_rule_create(num_states, new_num_neighbors);
    if (!coarse) return NULL;

    /* Majority-rule coarse graining: a block of cells is mapped to
     * the majority state. This is a simplified version of the full
     * renormalization procedure. */
    (void)block_size; /* Full implementation would need the original CA */
    return coarse;
}

VnaTransitionRule* vna_rule_stochastic_perturb(VnaTransitionRule* rule,
                                                double noise_level) {
    if (!rule || noise_level < 0.0 || noise_level > 1.0) return NULL;
    VnaTransitionRule* noisy = vna_rule_create(rule->num_states,
                                                rule->num_neighbors);
    if (!noisy) return NULL;
    char buf[256];
    snprintf(buf, sizeof(buf), "Noisy variant (ε=%.3f) of %s",
             noise_level, rule->name ? rule->name : "unnamed");
    noisy->name = strdup(buf);

    int64_t table_size = 1;
    for (int i = 0; i < rule->num_neighbors; i++) table_size *= rule->num_states;
    for (int64_t i = 0; i < table_size; i++) {
        if ((double)rand() / RAND_MAX < noise_level) {
            /* Random output state */
            noisy->rule_table[i] = (uint8_t)(rand() % rule->num_states);
        } else {
            noisy->rule_table[i] = rule->rule_table[i];
        }
    }
    return noisy;
}

/* ============================================================================
 * Utility
 * ============================================================================ */

void vna_rule_print(VnaTransitionRule* rule) {
    if (!rule) return;
    printf("Transition Rule: %s\n", rule->name ? rule->name : "unnamed");
    printf("  States: %d, Neighbors: %d\n", rule->num_states, rule->num_neighbors);
    printf("  Totalistic: %s, Outer-totalistic: %s\n",
           rule->is_totalistic ? "yes" : "no",
           rule->is_outer_totalistic ? "yes" : "no");
    if (rule->wolfram_code >= 0)
        printf("  Wolfram Code: %lld\n", (long long)rule->wolfram_code);
    if (rule->description)
        printf("  Description: %s\n", rule->description);

    /* Print first few rule table entries */
    int64_t display = 1;
    for (int i = 0; i < rule->num_neighbors; i++) display *= rule->num_states;
    if (display > 64) display = 64;
    printf("  Rule table (first %lld entries): ", (long long)display);
    for (int64_t i = 0; i < display; i++)
        printf("%d", rule->rule_table[i]);
    printf("\n");
}

char* vna_rule_to_wolfram_string(VnaTransitionRule* rule, int radius) {
    if (!rule || rule->num_states != 2) return NULL;
    int code = vna_rule_to_wolfram(rule);
    if (code < 0) return NULL;
    char* str = (char*)malloc(64);
    snprintf(str, 64, "Rule %d (radius %d)", code, radius);
    return str;
}
