#include "vna_replication.h"
#include "vna_core.h"
#include "vna_cellular.h"
#include "vna_rule.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Langton Loop Pattern Generation
 * ============================================================================ */

VnaPattern* vna_langton_loop_pattern(int side_length, int x0, int y0) {
    /* Langton's self-replicating loop has a characteristic structure:
     * - Outer sheath (state 2) forming a square
     * - Inner data path (state 3,4) just inside the sheath
     * - The loop circulates signals (state 4) counterclockwise.
     *
     * Simplified encoding: we create the geometric loop structure.
     * Full loop dynamics require Codd's 8-state transition rule. */
    if (side_length < 6) side_length = 10; /* Minimum viable size */
    int s = side_length;

    VnaPattern* pattern = vna_pattern_create(s, s, NULL, "Langton Loop");
    if (!pattern) return NULL;

    /* Fill with empty (0) */
    /* Outer sheath: state 2 around the perimeter */
    for (int x = 0; x < s; x++) {
        pattern->data[x] = LANGTON_SHEATH;                    /* Top */
        pattern->data[(s-1) * s + x] = LANGTON_SHEATH;        /* Bottom */
    }
    for (int y = 0; y < s; y++) {
        pattern->data[y * s] = LANGTON_SHEATH;                /* Left */
        pattern->data[y * s + (s-1)] = LANGTON_SHEATH;        /* Right */
    }

    /* Inner data path: one cell inside the sheath */
    for (int x = 1; x < s - 1; x++) {
        pattern->data[1 * s + x] = LANGTON_DATA_PATH;         /* Just below top */
        pattern->data[(s-2) * s + x] = LANGTON_DATA_PATH;     /* Just above bottom */
    }
    for (int y = 1; y < s - 1; y++) {
        pattern->data[y * s + 1] = LANGTON_DATA_PATH;         /* Just right of left */
        pattern->data[y * s + (s-2)] = LANGTON_DATA_PATH;     /* Just left of right */
    }

    /* Signal markers in the data path (top-left region) */
    pattern->data[1 * s + 3] = LANGTON_SIGNAL_0;
    pattern->data[1 * s + 5] = LANGTON_SIGNAL_1;
    pattern->data[2 * s + 1] = LANGTON_SIGNAL_2;
    pattern->data[4 * s + 1] = LANGTON_SIGNAL_3;

    /* Core at center */
    pattern->data[(s/2) * s + (s/2)] = LANGTON_CORE;

    pattern->period = 0;
    pattern->is_glider = false;
    return pattern;
}

VnaLangtonLoop* vna_langton_loop_extract(VnaLattice* lattice, int x0, int y0) {
    if (!lattice) return NULL;
    VnaLangtonLoop* loop = (VnaLangtonLoop*)calloc(1, sizeof(VnaLangtonLoop));
    if (!loop) return NULL;

    /* Try to detect loop dimensions by scanning for sheath cells */
    int max_scan = 50;
    int right = x0;
    while (right < lattice->width && right < x0 + max_scan) {
        uint8_t s = vna_lattice_get_cell(lattice, right, y0, 0);
        if (s == LANGTON_SHEATH || s == CODD_SHEATH) right++;
        else break;
    }
    loop->side_length = right - x0;

    /* Count signals inside the loop */
    int signal_count = 0;
    for (int y = y0 + 1; y < y0 + loop->side_length - 1; y++) {
        for (int x = x0 + 1; x < x0 + loop->side_length - 1; x++) {
            uint8_t s = vna_lattice_get_cell(lattice, x, y, 0);
            if (s >= LANGTON_SIGNAL_0 && s <= LANGTON_SIGNAL_3) signal_count++;
        }
    }
    loop->signal_count = signal_count;
    loop->x0 = x0;
    loop->y0 = y0;
    return loop;
}

void vna_langton_loop_free(VnaLangtonLoop* loop) {
    if (!loop) return;
    free(loop->encoded_data);
    free(loop);
}

/* ============================================================================
 * L5: Replication Detection
 * ============================================================================ */

int vna_detect_replication_events(VnaLattice* before, VnaLattice* after,
                                   int min_size, int* event_x, int* event_y,
                                   int max_events) {
    if (!before || !after) return 0;
    if (before->width != after->width || before->height != after->height)
        return 0;

    int found = 0;
    int half_w = before->width / 2;
    int half_h = before->height / 2;

    /* Simple approach: scan for regions in `after` that match patterns
     * from `before` at displaced positions.
     * This detects approximate self-replication. */
    for (int by = min_size; by < half_h - min_size && found < max_events; by += min_size) {
        for (int bx = min_size; bx < half_w - min_size && found < max_events; bx += min_size) {
            /* Check if a pattern centered at (bx, by) in `before`
             * appears elsewhere in `after`. */
            VnaPattern* template = vna_pattern_extract(before, bx, by,
                                                        min_size, min_size);
            if (!template) continue;

            /* Search for this template in `after`, excluding original location */
            int best_x = -1, best_y = -1;
            double score = vna_pattern_match(after, template, &best_x, &best_y);

            if (score > 0.85 &&
                (abs(best_x - bx) > min_size || abs(best_y - by) > min_size)) {
                if (event_x) event_x[found] = best_x;
                if (event_y) event_y[found] = best_y;
                found++;
            }
            vna_pattern_free(template);
        }
    }
    return found;
}

bool vna_track_lineage(VnaLattice* history[], int num_timesteps,
                        int final_x, int final_y,
                        int* ancestor_x, int* ancestor_y, int* ancestor_t) {
    if (!history || num_timesteps < 2) return false;

    /* Lineage tracking: for a cell at time t, find the cell at time t-1
     * that most influenced its state (in von Neumann automata, the
     * center cell and its neighbors). Simplified: track the majority
     * parent contribution. */
    int cx = final_x, cy = final_y;
    for (int t = num_timesteps - 1; t > 0; t--) {
        VnaLattice* prev = history[t - 1];
        VnaLattice* curr = history[t];
        if (!prev || !curr) return false;

        uint8_t target = vna_lattice_get_cell(curr, cx, cy, 0);
        /* Find the cell in prev that most likely produced this state.
         * Simplified: search local neighborhood. */
        bool found = false;
        for (int dy = -2; dy <= 2 && !found; dy++) {
            for (int dx = -2; dx <= 2 && !found; dx++) {
                int px = cx + dx, py = cy + dy;
                uint8_t ps = vna_lattice_get_cell(prev, px, py, 0);
                if (ps == target) {
                    cx = px; cy = py;
                    found = true;
                }
            }
        }
        if (!found) {
            /* Try center cell */
            uint8_t ps = vna_lattice_get_cell(prev, cx, cy, 0);
            /* Keep cx, cy if the state matches reasonably */
            (void)ps;
        }
    }

    if (ancestor_x) *ancestor_x = cx;
    if (ancestor_y) *ancestor_y = cy;
    if (ancestor_t) *ancestor_t = 0;
    return true;
}

/* ============================================================================
 * L4: Self-Replication Feasibility Analysis
 * ============================================================================ */

double vna_rule_replication_score(VnaTransitionRule* rule, int num_states,
                                   VnaNeighborhood* nb) {
    /* Score a rule's potential for supporting self-replication.
     * Based on heuristic criteria from von Neumann's analysis:
     * 1. Quiescent state must exist
     * 2. Signals must be able to propagate
     * 3. Signals must be able to cross without interference
     * 4. The rule must allow construction (arbitrary pattern formation) */
    if (!rule || !nb) return 0.0;
    if (num_states < 4) return 0.1; /* Too few states for complex replication */

    double score = 0.0;

    /* Criterion 1: Quiescent state (20%) */
    uint8_t quiescent = 0;
    uint8_t* all_quiet = (uint8_t*)calloc(nb->num_neighbors, sizeof(uint8_t));
    bool has_quiescent = false;
    for (uint8_t s = 0; s < num_states; s++) {
        for (int i = 0; i < nb->num_neighbors; i++) all_quiet[i] = s;
        if (vna_rule_lookup(rule, all_quiet) == s) {
            has_quiescent = true;
            quiescent = s;
            break;
        }
    }
    if (has_quiescent) score += 0.2;

    /* Criterion 2: Signal propagation (30%)
     * Check that a non-quiescent state adjacent to quiescent cells
     * can propagate its state. */
    bool propagates = false;
    for (int i = 0; i < nb->num_neighbors; i++) {
        for (int s = 0; s < num_states && !propagates; s++) {
            if (s == quiescent) continue;
            for (int j = 0; j < nb->num_neighbors; j++) all_quiet[j] = quiescent;
            all_quiet[i] = (uint8_t)s;
            uint8_t result = vna_rule_lookup(rule, all_quiet);
            if (result != quiescent) propagates = true;
        }
    }
    if (propagates) score += 0.3;

    /* Criterion 3: Sufficient state diversity (20%) */
    score += 0.2 * (num_states >= 8 ? 1.0 : (double)num_states / 8.0);

    /* Criterion 4: Complex behavior potential (30%)
     * Use Langton lambda and Z-parameter as indicators. */
    double lambda = vna_rule_langton_lambda(rule, quiescent);
    /* λ near the "edge of chaos" (0.2-0.4) is most promising */
    if (lambda > 0.15 && lambda < 0.45) score += 0.15;
    if (lambda > 0.2 && lambda < 0.35) score += 0.15;

    free(all_quiet);
    return score > 1.0 ? 1.0 : score;
}

/* ============================================================================
 * L3: Template Replication
 * ============================================================================ */

int vna_template_replication_rule(VnaPattern* template_pattern,
                                   int num_states, VnaNeighborhood* nb,
                                   VnaTransitionRule** candidate_rules,
                                   int max_candidates) {
    /* Template replication: find a rule where the template produces
     * a copy of itself in an adjacent region.
     * This is a search problem over rule space. We generate candidate
     * rules that are consistent with the template being a still life,
     * and then check if an adjacent empty region becomes the template. */
    if (!template_pattern || max_candidates <= 0) return 0;

    int found = 0;
    /* Use the synthesis function to get a base rule */
    found = vna_synthesize_rule_for_pattern(template_pattern, num_states,
                                             nb->type, nb->radius,
                                             candidate_rules, max_candidates);

    /* Modify the rule to make adjacent empty cells become the pattern.
     * For each cell in the empty region adjacent to the template,
     * set the rule to produce the matching cell from the template. */
    if (found > 0 && candidate_rules[0]) {
        VnaTransitionRule* rule = candidate_rules[0];
        /* For demo purposes: set the rule so that if a neighborhood
         * contains the right half of the template on the left side,
         * output the left half of the template.
         * This is a simplified demonstration. */
        (void)rule; /* Full template replication is complex */
    }

    return found;
}

/* ============================================================================
 * L5: Complexity of Self-Replication
 * ============================================================================ */

int64_t vna_replication_complexity_bound(VnaLattice* lattice) {
    /* Kolmogorov complexity lower bound:
     * K(C) ≥ |C| - O(log|C|) for a self-replicating configuration.
     * A self-replicator must contain its own description.
     * We estimate the minimum description length as the number of
     * non-quiescent cells times the bits per cell. */
    if (!lattice) return 0;
    int non_quiescent = 0;
    int total = lattice->width * lattice->height;
    for (int i = 0; i < total; i++)
        if (lattice->cells[i] != 0) non_quiescent++;

    /* Bits per cell = log2(num_states) */
    double bits_per_cell = log2(lattice->num_states);
    return (int64_t)(non_quiescent * bits_per_cell);
}

/* ============================================================================
 * L7: Population Dynamics for Artificial Life
 * ============================================================================ */

VnaPopulation* vna_population_create(void) {
    VnaPopulation* pop = (VnaPopulation*)calloc(1, sizeof(VnaPopulation));
    if (!pop) return NULL;
    pop->generation = 0;
    return pop;
}

void vna_population_free(VnaPopulation* pop) {
    free(pop);
}

void vna_population_update(VnaPopulation* pop, VnaLattice* before,
                             VnaLattice* after) {
    if (!pop || !before || !after) return;
    /* Count living cells */
    int total = before->width * before->height;
    int live_before = 0, live_after = 0;
    for (int i = 0; i < total; i++) {
        if (before->cells[i] > 0) live_before++;
        if (after->cells[i] > 0) live_after++;
    }

    pop->total_live_cells = live_after;

    /* Estimate population: count connected components of live cells */
    /* Simplified: use density as proxy */
    double density = (double)live_after / total;
    pop->population = (int64_t)(density * 100);

    /* Replication rate */
    if (live_before > 0) {
        pop->replication_rate = (double)(live_after - live_before) / live_before;
    }

    pop->generation++;
}

void vna_population_print(VnaPopulation* pop) {
    if (!pop) return;
    printf("ALife Population:\n");
    printf("  Generation:        %lld\n", (long long)pop->generation);
    printf("  Population est:    %lld\n", (long long)pop->population);
    printf("  Total live cells:  %lld\n", (long long)pop->total_live_cells);
    printf("  Replication rate:  %.4f\n", pop->replication_rate);
    printf("  Mutation rate:     %.6f\n", pop->mutation_rate);
    printf("  Mean lifespan:     %.2f\n", pop->mean_lifespan);
}
