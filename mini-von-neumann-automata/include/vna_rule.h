#ifndef VNA_RULE_H
#define VNA_RULE_H

#include "vna_core.h"

/* ============================================================================
 * Transition Rule Theory — Encoding, Analysis, and Classification
 *
 * Covers: L1 (rule definitions), L2 (rule concepts), L3 (mathematical structure),
 *         L4 (fundamental theorems about rule spaces), L5 (rule analysis algorithms)
 *
 * Key references:
 *   Wolfram (1983) — Statistical mechanics of CA, Rev. Mod. Phys.
 *   Wolfram (2002) — A New Kind of Science
 *   Li & Packard (1990) — Structure of elementary CA rule space
 *   Langton (1990) — Computation at the edge of chaos
 *   Culik & Yu (1988) — Undecidability of CA classification
 * ============================================================================ */

/* --- L1: Rule Classification (Wolfram Classes) --- */

/* Wolfram's phenomenological classification of CA behavior:
 *   Class I  — Fixed point (homogeneous state)
 *   Class II — Periodic (simple oscillators)
 *   Class III — Chaotic (aperiodic, sensitive to initial conditions)
 *   Class IV — Complex (long-lived transients, computational) */
typedef enum {
    VNA_WOLFRAM_I   = 0,    /* Evolution leads to homogeneous state */
    VNA_WOLFRAM_II  = 1,    /* Evolution leads to periodic structures */
    VNA_WOLFRAM_III = 2,    /* Evolution leads to chaotic patterns */
    VNA_WOLFRAM_IV  = 3,    /* Evolution leads to complex structures */
    VNA_UNCLASSIFIED = 4    /* Classification not yet determined */
} VnaWolframClass;

/* --- L2: Totalistic and Outer-Totalistic Rules --- */

/* Totalistic rule: next state depends only on the sum of neighbor states.
 * f(s_0, ..., s_{n-1}) = g(Σ s_i)
 * The rule table is much smaller: (k-1)*n + 1 entries instead of k^n. */
void vna_rule_set_totalistic_sum(VnaTransitionRule* rule, int max_sum,
                                  const uint8_t* sum_mapping);
bool vna_rule_is_totalistic(VnaTransitionRule* rule);

/* Outer-totalistic rule: depends on sum of neighbors + center separately.
 * f(center, s_1, ..., s_{n-1}) = g(center, Σ_{i≠0} s_i)
 * Conway's Game of Life is outer-totalistic. */
void vna_rule_set_outer_totalistic(VnaTransitionRule* rule, int num_states,
                                    int max_neighbor_sum,
                                    const uint8_t* sum_mapping);
bool vna_rule_is_outer_totalistic(VnaTransitionRule* rule);

/* --- L1: Specific Named Rules --- */

/* Conway's Game of Life (B3/S23):
 * Born if exactly 3 neighbors are alive.
 * Survives if 2 or 3 neighbors are alive.
 * This is an outer-totalistic rule on the Moore neighborhood. */
VnaTransitionRule* vna_rule_game_of_life(void);

/* HighLife (B36/S23): Game of Life variant with additional birth condition. */
VnaTransitionRule* vna_rule_highlife(void);

/* Brian's Brain: 3-state CA with excitable dynamics. */
VnaTransitionRule* vna_rule_brians_brain(void);

/* WireWorld: 4-state CA for simulating digital circuits. */
VnaTransitionRule* vna_rule_wireworld(void);

/* Seeds: all cells die; birth on exactly 2 neighbors. Chaotic class III. */
VnaTransitionRule* vna_rule_seeds(void);

/* Day & Night (B3678/S34678): symmetric under state inversion. */
VnaTransitionRule* vna_rule_day_and_night(void);

/* --- L3: Rule Space Structure --- */

/* Compute the Langton parameter λ of a rule.
 * λ = fraction of neighborhood configurations that map to non-quiescent state.
 * Langton (1990) showed λ ≈ 0.273–0.5 corresponds to Class IV (complex). */
double vna_rule_langton_lambda(VnaTransitionRule* rule, uint8_t quiescent);

/* Compute the Z-parameter (Wuensche, 1999) from the mean-field curve.
 * Z = derivative of the mean-field map at ρ = 0.
 * Z > 1 implies unstable fixed point at ρ = 0 (potential Class III/IV). */
double vna_rule_z_parameter(VnaTransitionRule* rule, int num_neighbors);

/* Compute Hamming distance between two rules.
 * Fraction of neighborhood configurations where rules differ.
 * Measures similarity in rule space. */
double vna_rule_hamming_distance(VnaTransitionRule* r1, VnaTransitionRule* r2);

/* --- L4: Rule Space Properties (Theorems) --- */

/* Check if a rule is reversible (bijective global map).
 * For 1D CA: a rule is reversible iff the de Bruijn diagram has no
 * branching. For 2D, reversibility is undecidable in general (Kari, 1990). */
bool vna_rule_is_reversible_1d(VnaTransitionRule* rule, int num_states,
                                int radius);

/* Check if a rule is number-conserving.
 * A rule conserves particle number if Σ s_i(x,t) = Σ s_i(x,t+1) for all x.
 * (Boccara & Fukś, 1998; Durand-Lose, 2001) */
bool vna_rule_is_number_conserving(VnaTransitionRule* rule, int num_states,
                                    int num_neighbors);

/* Check if a rule is additive (linear over GF(k)).
 * f(s_0,...,s_{n-1}) = Σ a_i s_i mod k for some a_i.
 * Additive rules have closed-form solutions via generating functions. */
bool vna_rule_is_additive(VnaTransitionRule* rule, int num_states,
                           int num_neighbors);

/* --- L5: Rule Analysis Algorithms --- */

/* Attempt to classify a rule into Wolfram's classes I-IV.
 * Uses multiple heuristics: Langton λ, Z-parameter, entropy evolution,
 * and transient length analysis from random initial conditions. */
VnaWolframClass vna_rule_classify_wolfram(VnaTransitionRule* rule,
                                           int num_states, int num_neighbors,
                                           int sample_width, int samples,
                                           int max_steps);

/* Compute the rule's de Bruijn diagram.
 * Nodes: (s_{-r}, ..., s_{r-1}) — length 2r tuples.
 * Edges: (s_{-r},...,s_{r-1}) → (s_{-r+1},...,s_r) if f produces s_r.
 * Format: adjacency matrix of size k^(2r) × k^(2r). */
typedef struct {
    int    num_nodes;           /* k^(2r) */
    int    num_edges;           /* k^(2r+1) edges, k per node */
    int*   adjacency;           /* adj[node * k + branch] = target node */
    uint8_t* outputs;           /* output state for each edge */
} VnaDeBruijnDiagram;

VnaDeBruijnDiagram* vna_rule_debruijn_diagram(VnaTransitionRule* rule,
                                                int num_states, int radius);
void vna_debruijn_diagram_free(VnaDeBruijnDiagram* db);

/* Compute the subset diagram of the de Bruijn diagram.
 * Nodes are subsets of de Bruijn nodes, representing sets of possible
 * predecessor partial configurations. Used for Garden of Eden detection. */
typedef struct {
    int    num_nodes;
    int    num_edges;
    int*   adjacency;           /* adj[subset * k] = target subset */
} VnaSubsetDiagram;

VnaSubsetDiagram* vna_rule_subset_diagram(VnaDeBruijnDiagram* db,
                                           int num_states);
void vna_subset_diagram_free(VnaSubsetDiagram* sd);

/* --- L8: Advanced Rule Operations --- */

/* Compute the 2-block transformation of a 1D rule.
 * This maps the rule to an equivalent rule on a coarse-grained lattice.
 * Used in renormalization group analysis of CA. */
VnaTransitionRule* vna_rule_block_renormalize(VnaTransitionRule* rule,
                                               int num_states, int radius,
                                               int block_size);

/* Generate a stochastic version of a deterministic rule.
 * Each output state is assigned a probability based on the original
 * rule plus noise. The noise_level controls the probability of
 * randomly perturbing the output. */
VnaTransitionRule* vna_rule_stochastic_perturb(VnaTransitionRule* rule,
                                                double noise_level);

/* --- Utility --- */

/* Print the rule table in a human-readable format. */
void vna_rule_print(VnaTransitionRule* rule);

/* Export rule as Wolfram code string (for 1D rules). */
char* vna_rule_to_wolfram_string(VnaTransitionRule* rule, int radius);

#endif /* VNA_RULE_H */