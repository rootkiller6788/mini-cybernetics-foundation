#ifndef VNA_CELLULAR_H
#define VNA_CELLULAR_H

#include "vna_core.h"

/* ============================================================================
 * Cellular Automaton Operations — Advanced Cell-Level Primitives
 *
 * Covers: L2 (core concepts), L3 (mathematical structures), L5 (algorithms)
 *
 * Key concepts:
 *   - Quiescent state identification
 *   - Configuration space operations
 *   - Sub-lattice and block operations
 *   - Pattern enumeration and counting
 *   - Damage spreading analysis
 *   - Entropy and complexity measures
 * ============================================================================ */

/* --- L2: Quiescent State and Configuration Space --- */

/* The quiescent state is the "rest" state from which the automaton
 * can be activated. In most CA, state 0 is quiescent. A configuration
 * is "finite" if only finitely many cells are non-quiescent.
 * Von Neumann relied on this concept for his construction. */
uint8_t vna_find_quiescent_state(VnaLattice* lattice, VnaTransitionRule* rule,
                                  VnaNeighborhood* nb);
bool vna_is_quiescent_config(VnaLattice* lattice, uint8_t quiescent);
bool vna_configuration_is_finite(VnaLattice* lattice, uint8_t quiescent);

/* --- L3: Lattice Subset Operations --- */

/* Extract a sub-lattice (window) from the main lattice.
 * Useful for studying local dynamics and for pattern detection. */
VnaLattice* vna_sublattice_extract(VnaLattice* lattice, int x0, int y0,
                                    int w, int h);
void vna_sublattice_insert(VnaLattice* dest, VnaLattice* src,
                            int dest_x, int dest_y);

/* Block-based operations for Margolus neighborhood.
 * The lattice is partitioned into 2x2 blocks; blocks are rotated
 * or permuted according to the Margolus rule. */
int vna_block_partition_count(VnaLattice* lattice, int block_w, int block_h);
void vna_block_get(VnaLattice* lattice, int block_idx, int block_w, int block_h,
                    uint8_t* block_out);
void vna_block_set(VnaLattice* lattice, int block_idx, int block_w, int block_h,
                    const uint8_t* block_in);

/* --- L3: Advanced Lattice Access --- */

/* Periodic index wraparound (torus topology) */
int vna_wrap_x(VnaLattice* lattice, int x);
int vna_wrap_y(VnaLattice* lattice, int y);
int vna_wrap_z(VnaLattice* lattice, int z);

/* Neighbor state collection into a buffer.
 * For a given cell, collect the states of all neighbors according
 * to the neighborhood specification into the provided buffer. */
void vna_collect_neighbors(VnaLattice* lattice, int x, int y, int z,
                            VnaNeighborhood* nb, uint8_t* neighbor_buf);

/* Convolution-like operation: apply a kernel over the lattice.
 * For each cell, the kernel-weighted sum of neighbor states is computed.
 * This generalizes both totalistic rules and image processing on CA. */
double vna_lattice_convolve(VnaLattice* lattice, int x, int y, int z,
                             double* kernel, int kernel_w, int kernel_h);

/* --- L5: Pattern Analysis Algorithms --- */

/* Enumerate all distinct n×n block patterns in the lattice.
 * Returns the number of distinct patterns found. The patterns
 * are stored in the provided buffer (must be pre-allocated). */
int vna_enumerate_blocks(VnaLattice* lattice, int block_w, int block_h,
                          uint8_t* pattern_buffer, int* counts, int max_patterns);

/* Compute the block entropy — the Shannon entropy of the distribution
 * of block patterns. This measures the structural complexity. */
double vna_block_entropy(VnaLattice* lattice, int block_w, int block_h);

/* --- L5: Damage Spreading Analysis --- */

/* Damage spreading: create a copy, flip one cell, evolve both,
 * and measure the Hamming distance over time. This estimates the
 * Lyapunov exponent of the CA dynamics.
 *
 * Reference: Bagnoli, Rechtman, Ruffo (1992), Phys. Lett. A */
typedef struct {
    double* hamming_distance;     /* H(t) at each time step */
    int     max_steps;            /* Number of steps recorded */
    double  lyapunov_estimate;    /* λ ≈ (1/t) * log(H(t)/H(0)) */
    double  damage_saturation;    /* Saturation level (0 to 1) */
    bool    is_chaotic;           /* True if damage grows exponentially */
} VnaDamageSpread;

VnaDamageSpread* vna_damage_spreading(VnaLattice* lattice, VnaNeighborhood* nb,
                                       VnaTransitionRule* rule, int steps,
                                       int perturb_x, int perturb_y);
void vna_damage_spread_free(VnaDamageSpread* ds);

/* --- L5: CA Inverse Problem / Synthesis --- */

/* Given a target pattern, attempt to find transition rules that
 * produce it. This is a search over rule space — NP-hard in general.
 * Returns the number of candidate rules found. */
int vna_synthesize_rule_for_pattern(VnaPattern* target, int num_states,
                                     VnaNeighborhoodType nb_type, int radius,
                                     VnaTransitionRule** candidates, int max_candidates);

/* --- L5: Higher-Order CA Operations --- */

/* Mean-field theory approximation: estimate the density after one
 * time step, assuming states are uncorrelated. This gives the
 * mean-field map ρ(t+1) = f(ρ(t)). */
double vna_mean_field_density(VnaLattice* lattice, VnaTransitionRule* rule,
                               VnaNeighborhood* nb);

/* Compute the pair correlation function C(r): the probability that
 * two cells at distance r have the same state, beyond random chance. */
double* vna_pair_correlation(VnaLattice* lattice, int max_distance,
                              int* num_pairs_per_distance);

/* Mutual information between a cell and its neighborhood.
 * I(cell; N(cell)) = H(cell) + H(N(cell)) - H(cell, N(cell)).
 * This measures how much information the neighborhood provides
 * about the cell's next state. */
double vna_cell_neighborhood_mi(VnaLattice* lattice, VnaNeighborhood* nb,
                                 VnaTransitionRule* rule);

/* Detect if the lattice is in a Garden of Eden configuration
 * (a configuration with no predecessor). Uses the subset diagram.
 * Returns the number of cells identified as potential GoE seeds. */
int vna_detect_garden_of_eden(VnaLattice* lattice, VnaNeighborhood* nb,
                               VnaTransitionRule* rule);

/* --- L6: Canonical Patterns in CA --- */

/* Still life detection: a pattern that does not change under the rule.
 * Returns true if the lattice is at a fixed point. */
bool vna_is_still_life(VnaLattice* lattice, VnaNeighborhood* nb,
                        VnaTransitionRule* rule);

/* Oscillator detection: determine if the lattice is in a periodic cycle.
 * Returns the period length. 0 means no period detected within max_period. */
int vna_detect_oscillator(VnaLattice* lattice, VnaNeighborhood* nb,
                           VnaTransitionRule* rule, int max_period);

/* Glider detection: moving localized patterns. Uses cross-correlation
 * between successive timesteps to detect displacement. */
int vna_detect_gliders(VnaLattice* lattice, VnaNeighborhood* nb,
                        VnaTransitionRule* rule, int* glider_positions,
                        int max_gliders);

#endif /* VNA_CELLULAR_H */