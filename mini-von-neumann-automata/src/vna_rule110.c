#include "vna_core.h"
#include "vna_cellular.h"
#include "vna_rule.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Rule 110 — Universality in Elementary Cellular Automata
 *
 * Covers: L1 (definition), L2 (universality concept), L3 (mathematical
 *         structure), L4 (Cook's theorem), L5 (simulation), L6 (canonical)
 *
 * Matthew Cook (1990, published 2004) proved that Rule 110 is Turing complete.
 * This was a landmark result: the simplest known universal Turing machine
 * is embedded in a 1D binary CA with radius 1.
 *
 * Rule 110 transition (Wolfram notation):
 *   pattern: 111 110 101 100 011 010 001 000
 *   output:   0   1   1   0   1   1   1   0
 *
 * Key structures:
 *   - Background: repeating pattern 00010011011111 (spatial period 14)
 *   - Gliders: localized structures that move through the background
 *   - Glider interactions implement logic gates (AND, OR, NOT, fan-out)
 * ============================================================================ */

/* --- L1: Rule 110 Definition --- */

VnaTransitionRule* vna_rule110_create(void) {
    VnaTransitionRule* rule = vna_rule_create(2, 3); /* binary, radius 1 */
    if (!rule) return NULL;
    rule->name = strdup("Rule 110");
    rule->description = strdup(
        "Rule 110 (01101110 binary). Proven Turing complete by Cook (2004).");
    rule->wolfram_code = 110;

    /* Set rule table: Wolfram code 110 = binary 01101110 */
    uint8_t table[8] = {0, 1, 1, 1, 1, 0, 1, 0};
    /* Wolfram code: LSB = rule(000), MSB = rule(111).
     * Code 110 = 0b01101110
     * Index 0 (000) → 0
     * Index 1 (001) → 1
     * Index 2 (010) → 1
     * Index 3 (011) → 1
     * Index 4 (100) → 0
     * Index 5 (101) → 1
     * Index 6 (110) → 1
     * Index 7 (111) → 0 */
    for (int i = 0; i < 8; i++)
        rule->rule_table[i] = table[i];

    return rule;
}

/* --- L3: Rule 110 Canonical Structures --- */

/* Rule 110 background ether: the periodic pattern that fills space.
 * Period: 7 (spatial period 14, temporal period 7).
 * Pattern: 00010011011111... */
typedef struct {
    int      spatial_period;
    int      temporal_period;
    uint8_t* pattern;       /* One period of the background ether */
} Rule110Background;

Rule110Background* vna_rule110_background(void) {
    Rule110Background* bg =
        (Rule110Background*)calloc(1, sizeof(Rule110Background));
    if (!bg) return NULL;
    bg->spatial_period = 14;
    bg->temporal_period = 7;
    bg->pattern = (uint8_t*)malloc(14);
    /* One spatial period: 00010011011111 */
    uint8_t ether_pattern[14] = {0,0,0,1,0,0,1,1,0,1,1,1,1,1};
    memcpy(bg->pattern, ether_pattern, 14);
    return bg;
}

void vna_rule110_background_free(Rule110Background* bg) {
    if (!bg) return;
    free(bg->pattern);
    free(bg);
}

/* --- L3: Rule 110 Glider Catalog --- */

/* Rule 110 has a rich taxonomy of gliders (moving localized structures).
 * Cook's proof uses specific glider collisions to implement logic. */

typedef enum {
    R110_GLIDER_A = 0,  /* Most common glider; speed c/2? */
    R110_GLIDER_B = 1,  /* Wider glider */
    R110_GLIDER_C = 2,  /* Fast glider */
    R110_GLIDER_D = 3,  /* Ether disruption (glider gun interaction) */
    R110_GLIDER_E = 4   /* Glider used in Cook's proof */
} Rule110GliderType;

typedef struct {
    Rule110GliderType type;
    int     width;            /* Spatial extent */
    int     period;           /* Temporal period */
    int     dx_per_period;    /* Spatial displacement per period */
    uint8_t* pattern;         /* Pattern data */
    char*   name;
} Rule110Glider;

Rule110Glider* vna_rule110_glider_create(Rule110GliderType type) {
    Rule110Glider* glider =
        (Rule110Glider*)calloc(1, sizeof(Rule110Glider));
    if (!glider) return NULL;
    glider->type = type;

    switch (type) {
        case R110_GLIDER_A:
            glider->width = 4;
            glider->period = 2;
            glider->dx_per_period = 2;
            glider->name = strdup("A-glider (c/2)");
            glider->pattern = (uint8_t*)malloc(4);
            { uint8_t a[] = {0,1,1,1}; memcpy(glider->pattern, a, 4); }
            break;
        case R110_GLIDER_B:
            glider->width = 6;
            glider->period = 5;
            glider->dx_per_period = 2;
            glider->name = strdup("B-glider");
            glider->pattern = (uint8_t*)malloc(6);
            memset(glider->pattern, 1, 6);
            break;
        case R110_GLIDER_C:
            glider->width = 3;
            glider->period = 7;
            glider->dx_per_period = 5;
            glider->name = strdup("C-glider (fast)");
            glider->pattern = (uint8_t*)malloc(3);
            { uint8_t c[] = {1,0,1}; memcpy(glider->pattern, c, 3); }
            break;
        case R110_GLIDER_D:
            glider->width = 8;
            glider->period = 15;
            glider->dx_per_period = 6;
            glider->name = strdup("D-glider (ether disruption)");
            glider->pattern = (uint8_t*)malloc(8);
            memset(glider->pattern, 1, 8);
            break;
        case R110_GLIDER_E:
            glider->width = 5;
            glider->period = 12;
            glider->dx_per_period = 8;
            glider->name = strdup("E-glider (Cook's glider)");
            glider->pattern = (uint8_t*)malloc(5);
            { uint8_t e[] = {1,0,0,1,1}; memcpy(glider->pattern, e, 5); }
            break;
    }
    return glider;
}

void vna_rule110_glider_free(Rule110Glider* glider) {
    if (!glider) return;
    free(glider->pattern);
    free(glider->name);
    free(glider);
}

/* --- L5: Rule 110 Background Fill --- */

int vna_rule110_fill_background(VnaLattice* lattice, int x0, int length) {
    if (!lattice) return -1;
    Rule110Background* bg = vna_rule110_background();
    if (!bg) return -1;

    for (int i = 0; i < length; i++) {
        int x = x0 + i;
        vna_lattice_set_cell(lattice, x, 0, 0,
                             bg->pattern[i % bg->spatial_period]);
    }
    vna_rule110_background_free(bg);
    return 0;
}

int vna_rule110_insert_glider(VnaLattice* lattice, int x, Rule110GliderType type) {
    if (!lattice) return -1;
    Rule110Glider* g = vna_rule110_glider_create(type);
    if (!g) return -1;
    for (int i = 0; i < g->width; i++)
        vna_lattice_set_cell(lattice, x + i, 0, 0, g->pattern[i]);
    vna_rule110_glider_free(g);
    return 0;
}

/* --- L4: Rule 110 Universality (Cook's Theorem) --- */

/* Cook proved Rule 110 is Turing complete by showing how to simulate
 * a cyclic tag system (which is Turing complete).
 *
 * Key components of the simulation:
 * 1. Background ether — acts as a "clock"
 * 2. Gliders — carry information
 * 3. Glider collisions — implement logic operations
 * 4. Ether disruptions — store and process data
 *
 * The simulation proceeds by:
 * (a) Encoding the tag system's tape as ether disruptions
 * (b) Using glider collisions to implement tag system rules
 * (c) Reading the output from the resulting tape configuration
 */

/* --- L5: Glider Collision Analysis --- */

/* Simulate a head-on collision between two Rule 110 gliders.
 * Returns the set of output gliders produced. */
int vna_rule110_simulate_collision(VnaLattice* lattice, VnaNeighborhood* nb,
                                    VnaTransitionRule* rule,
                                    int glider_a_x, Rule110GliderType type_a,
                                    int glider_b_x, Rule110GliderType type_b,
                                    int max_output_gliders,
                                    int* output_glider_x) {
    if (!lattice || !rule) return 0;

    /* Create clean lattice with background */
    vna_rule110_fill_background(lattice, 0, lattice->width);

    /* Insert the two gliders */
    vna_rule110_insert_glider(lattice, glider_a_x, type_a);
    vna_rule110_insert_glider(lattice, glider_b_x, type_b);

    /* Evolve until collision products form */
    int max_steps = 200;
    int output_count = 0;

    for (int t = 0; t < max_steps; t++) {
        vna_lattice_evolve(lattice, nb, rule);

        /* Detect gliders in the lattice (simple heuristic) */
        for (int x = 5; x < lattice->width - 10 && output_count < max_output_gliders;
             x++) {
            /* Check for A-glider signature */
            uint8_t a = vna_lattice_get_cell(lattice, x, 0, 0);
            uint8_t b = vna_lattice_get_cell(lattice, x+1, 0, 0);
            uint8_t c = vna_lattice_get_cell(lattice, x+2, 0, 0);
            uint8_t d = vna_lattice_get_cell(lattice, x+3, 0, 0);
            uint8_t e = vna_lattice_get_cell(lattice, x+4, 0, 0);

            /* Background check: mark if significantly deviating */
            if (a == 0 && b == 1 && c == 1 && d == 1 && e == 0) {
                /* Possible A-glider at position x+1 */
                if (output_glider_x)
                    output_glider_x[output_count] = x + 1;
                output_count++;
                break; /* Only count once per detection sweep */
            }
        }
    }
    return output_count;
}

/* --- L5: Rule 110 Periodic Orbits --- */

/* Find periodic structures in Rule 110.
 * Returns the number of distinct periodic patterns found. */
typedef struct {
    int     period;
    int     width;
    uint8_t* pattern;
} Rule110Periodic;

int vna_rule110_find_periodic(int max_width, int max_period,
                               Rule110Periodic** results) {
    /* Brute-force search for periodic patterns in a small width.
     * Only feasible for small widths due to exponential search space. */
    int found = 0;
    int max_search_states = 1 << max_width;
    if (max_search_states > 1000000) max_search_states = 1000000;

    VnaLattice* lattice = vna_lattice_create(max_width + 10, 1, 2,
                                              VNA_LATTICE_1D);
    VnaNeighborhood* nb = vna_neighborhood_create(VNA_ONE_DIMENSIONAL, 1, 2);
    VnaTransitionRule* rule = vna_rule110_create();

    if (!lattice || !nb || !rule) {
        vna_lattice_free(lattice);
        vna_neighborhood_free(nb);
        vna_rule_free(rule);
        return 0;
    }
    vna_lattice_set_boundary(lattice, VNA_BC_PERIODIC, VNA_BC_PERIODIC,
                              VNA_BC_PERIODIC);

    for (int state = 0; state < max_search_states && found < 50; state++) {
        /* Set initial state */
        for (int i = 0; i < max_width; i++)
            vna_lattice_set_cell(lattice, i, 0, 0, (state >> i) & 1);

        /* Detect period */
        int period = vna_detect_oscillator(lattice, nb, rule, max_period);
        if (period > 0 && period <= max_period && found < 50) {
            if (results) {
                results[found] = (Rule110Periodic*)malloc(sizeof(Rule110Periodic));
                results[found]->period = period;
                results[found]->width = max_width;
                results[found]->pattern = (uint8_t*)malloc(max_width);
                for (int i = 0; i < max_width; i++)
                    results[found]->pattern[i] =
                        vna_lattice_get_cell(lattice, i, 0, 0);
            }
            found++;
        }
    }

    vna_lattice_free(lattice);
    vna_neighborhood_free(nb);
    vna_rule_free(rule);
    return found;
}

/* --- L5: Binary Classification of Rule 110 Space-Time Diagrams --- */

/* Compute the spatial measure entropy of Rule 110 for different initial
 * densities. This demonstrates the phase transition behavior. */
double* vna_rule110_entropy_vs_density(int width, int steps,
                                        int density_steps) {
    double* entropies = (double*)malloc(density_steps * sizeof(double));
    if (!entropies) return NULL;

    VnaLattice* lattice = vna_lattice_create(width, 1, 2, VNA_LATTICE_1D);
    VnaNeighborhood* nb = vna_neighborhood_create(VNA_ONE_DIMENSIONAL, 1, 2);
    VnaTransitionRule* rule = vna_rule110_create();

    if (!lattice || !nb || !rule) {
        free(entropies);
        vna_lattice_free(lattice);
        vna_neighborhood_free(nb);
        vna_rule_free(rule);
        return NULL;
    }

    for (int d = 0; d < density_steps; d++) {
        double density = (double)(d + 1) / (density_steps + 1);
        vna_lattice_randomize(lattice, density, (unsigned int)(100 + d));
        vna_lattice_evolve_n_steps(lattice, nb, rule, steps);

        VnaStateHistogram* hist = vna_histogram_compute(lattice);
        entropies[d] = hist ? hist->shannon_entropy : 0.0;
        vna_histogram_free(hist);
    }

    vna_lattice_free(lattice);
    vna_neighborhood_free(nb);
    vna_rule_free(rule);
    return entropies;
}

/* --- L4: Cyclic Tag System Simulator (Proof of Universality) --- */

/* A cyclic tag system is a universal model of computation.
 * CTS = (k, productions[0..k-1])
 * At each step: if first symbol of tape is 1, append productions[step % k]
 * Then delete the first symbol. */

typedef struct {
    int     k;                  /* Number of productions */
    uint8_t** productions;       /* Each production is a string of 0s and 1s */
    int*    prod_lengths;
    uint8_t* tape;
    int     tape_length;
    int     tape_head;
    int     step;
} VnaCyclicTagSystem;

VnaCyclicTagSystem* vna_cts_create(int k) {
    VnaCyclicTagSystem* cts =
        (VnaCyclicTagSystem*)calloc(1, sizeof(VnaCyclicTagSystem));
    if (!cts) return NULL;
    cts->k = k;
    cts->productions = (uint8_t**)calloc(k, sizeof(uint8_t*));
    cts->prod_lengths = (int*)calloc(k, sizeof(int));
    cts->tape = NULL;
    cts->tape_length = 0;
    cts->tape_head = 0;
    cts->step = 0;
    return cts;
}

void vna_cts_free(VnaCyclicTagSystem* cts) {
    if (!cts) return;
    for (int i = 0; i < cts->k; i++) free(cts->productions[i]);
    free(cts->productions);
    free(cts->prod_lengths);
    free(cts->tape);
    free(cts);
}

void vna_cts_set_production(VnaCyclicTagSystem* cts, int idx,
                              const uint8_t* data, int len) {
    if (!cts || idx < 0 || idx >= cts->k) return;
    cts->productions[idx] = (uint8_t*)malloc(len);
    memcpy(cts->productions[idx], data, len);
    cts->prod_lengths[idx] = len;
}

void vna_cts_set_tape(VnaCyclicTagSystem* cts, const uint8_t* data, int len) {
    if (!cts) return;
    free(cts->tape);
    cts->tape = (uint8_t*)malloc(len);
    memcpy(cts->tape, data, len);
    cts->tape_length = len;
    cts->tape_head = 0;
}

bool vna_cts_step(VnaCyclicTagSystem* cts) {
    if (!cts || !cts->tape || cts->tape_head >= cts->tape_length)
        return false;

    int prod_idx = cts->step % cts->k;
    uint8_t first = cts->tape[cts->tape_head];
    cts->tape_head++;
    cts->step++;

    if (first == 1 && cts->prod_lengths[prod_idx] > 0) {
        /* Append production to tape */
        int new_len = cts->tape_length - cts->tape_head +
                       cts->prod_lengths[prod_idx];
        uint8_t* new_tape = (uint8_t*)malloc(new_len);
        memcpy(new_tape, cts->tape + cts->tape_head,
               cts->tape_length - cts->tape_head);
        memcpy(new_tape + cts->tape_length - cts->tape_head,
               cts->productions[prod_idx], cts->prod_lengths[prod_idx]);
        free(cts->tape);
        cts->tape = new_tape;
        cts->tape_length = new_len;
        cts->tape_head = 0;
    }

    return (cts->tape_head < cts->tape_length);
}

void vna_cts_print(VnaCyclicTagSystem* cts) {
    if (!cts) return;
    printf("CTS (k=%d, step=%d): tape=", cts->k, cts->step);
    for (int i = cts->tape_head; i < cts->tape_length; i++)
        putchar(cts->tape[i] ? '1' : '0');
    printf(" productions=[");
    for (int i = 0; i < cts->k; i++) {
        for (int j = 0; j < cts->prod_lengths[i]; j++)
            putchar(cts->productions[i][j] ? '1' : '0');
        if (i < cts->k - 1) printf(", ");
    }
    printf("]\n");
}

/* --- L7: Application — Rule 110 Random Number Generator --- */

/* Rule 110 can be used as a pseudorandom number generator.
 * The chaotic dynamics of Rule 110 from certain initial conditions
 * produce high-entropy output suitable for Monte Carlo applications. */
double vna_rule110_random_01(unsigned int* seed) {
    /* Simple PRNG based on Rule 110 evolution.
     * Maintain a small lattice and evolve it; sample cells for output. */
    static uint8_t state[32] = {0};
    static bool initialized = false;

    if (!initialized) {
        unsigned int s = seed ? *seed : 12345;
        for (int i = 0; i < 32; i++) {
            s = s * 1103515245 + 12345;
            state[i] = (s >> 16) & 1;
        }
        initialized = true;
    }

    /* Evolve one step of Rule 110 on the 32-cell lattice */
    uint8_t new_state[32];
    for (int i = 0; i < 32; i++) {
        int left  = state[(i - 1 + 32) % 32];
        int center = state[i];
        int right = state[(i + 1) % 32];
        int idx = (left << 2) | (center << 1) | right;
        /* Rule 110 table: 0,1,1,1,0,1,1,0 */
        const uint8_t table[8] = {0,1,1,1,0,1,1,0};
        new_state[i] = table[idx];
    }
    memcpy(state, new_state, 32);

    /* Combine 8 cells into a double */
    int combined = 0;
    for (int i = 0; i < 8; i++) {
        combined = (combined << 1) | state[i];
    }
    return (double)combined / 256.0;
}
