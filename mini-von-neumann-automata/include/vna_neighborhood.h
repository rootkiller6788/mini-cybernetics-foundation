#ifndef VNA_NEIGHBORHOOD_H
#define VNA_NEIGHBORHOOD_H

#include "vna_core.h"

/* ============================================================================
 * Neighborhood Theory — Geometric and Topological Properties
 *
 * Covers: L1 (neighborhood definitions), L2 (neighborhood concepts),
 *         L3 (mathematical structures for neighborhoods)
 *
 * Reference curricula:
 *   MIT 6.045 — Automata, Computability, Complexity
 *   Caltech CS 151 — Complexity Theory
 *   Cambridge Part II — Complexity Theory
 * ============================================================================ */

/* --- L1: Neighborhood Creation and Configuration --- */

/* Create a fully-specified neighborhood descriptor.
 * For standard types, offset arrays are computed automatically.
 * For VNA_CUSTOM, the offset arrays must be populated manually. */
VnaNeighborhood* vna_neighborhood_create_standard(VnaNeighborhoodType type,
                                                   int radius);
VnaNeighborhood* vna_neighborhood_create_custom(int capacity);
void vna_neighborhood_add_offset(VnaNeighborhood* nb, int dx, int dy, int dz);
void vna_neighborhood_finalize(VnaNeighborhood* nb);

/* --- L2: Neighborhood Properties --- */

/* Returns the von Neumann neighborhood radius r:
 * N_r(x,y) = {(x',y') : |x-x'| + |y-y'| ≤ r} */
int vna_von_neumann_count(int radius);

/* Returns the Moore neighborhood count for radius r:
 * N_r(x,y) = {(x',y') : max(|x-x'|, |y-y'|) ≤ r} */
int vna_moore_count(int radius);

/* Returns the total number of distinct neighbor configurations
 * for a CA with given state count and neighborhood size.
 * This is |S|^|N| — the size of the rule space. */
int64_t vna_rule_space_size(int num_states, int num_neighbors);

/* --- L3: Geometric and Topological Operations --- */

/* Compute the minimal bounding box of the neighborhood offsets.
 * Useful for visualization and for determining ghost layer size. */
void vna_neighborhood_bbox(VnaNeighborhood* nb, int* min_dx, int* max_dx,
                            int* min_dy, int* max_dy, int* min_dz, int* max_dz);

/* Check whether a neighborhood is isotropic under 90° rotations.
 * Isotropy is important for physical modeling applications. */
bool vna_neighborhood_is_isotropic(VnaNeighborhood* nb);

/* Compute the neighborhood graph — the adjacency structure of the
 * neighborhood offsets (two offsets are adjacent if they share an edge).
 * This is used to understand local connectivity. */
typedef struct {
    int  num_vertices;           /* Number of neighbor offsets */
    int  num_edges;              /* Number of edges in neighborhood graph */
    int* adjacency_matrix;       /* Flattened num_vertices × num_vertices */
} VnaNeighborhoodGraph;

VnaNeighborhoodGraph* vna_neighborhood_graph(VnaNeighborhood* nb);
void vna_neighborhood_graph_free(VnaNeighborhoodGraph* ng);

/* --- L2: Neighborhood Transformations --- */

/* Reflect the neighborhood horizontally or vertically.
 * The resulting neighborhood is the mirror image. */
VnaNeighborhood* vna_neighborhood_reflect(VnaNeighborhood* nb,
                                           bool horizontal);

/* Rotate the neighborhood by 0, 90, 180, or 270 degrees. */
VnaNeighborhood* vna_neighborhood_rotate(VnaNeighborhood* nb,
                                          int quarter_turns);

/* Compute the symmetric closure: add all missing symmetry-equivalent
 * offsets. Used to ensure rotationally-invariant rules. */
void vna_neighborhood_symmetrize(VnaNeighborhood* nb);

/* --- L2: Neighborhood Comparison --- */

/* Check if two neighborhoods are equivalent up to rotation/reflection. */
bool vna_neighborhood_equivalent(VnaNeighborhood* a, VnaNeighborhood* b);

/* Compute the overlap between two neighborhoods (Jaccard similarity). */
double vna_neighborhood_overlap(VnaNeighborhood* a, VnaNeighborhood* b);

/* --- L5: Neighborhood Optimization --- */

/* Compute the lookup table that maps a neighbor's (dx,dy) to its
 * index in the offset arrays. O(1) access for neighbor lookup.
 * Format: lookup[dy * (2*radius+1) + dx] = index. */
int* vna_neighborhood_lookup_table(VnaNeighborhood* nb);

/* Generate the "neighbor states → rule index" mapping.
 * For a k-state CA with n neighbors, a neighbor state tuple
 * (s_0, ..., s_{n-1}) maps to a unique index in [0, k^n).
 * This is the fundamental basis for rule table encoding. */
int64_t vna_neighbor_tuple_to_index(const uint8_t* states, int num_states,
                                     int num_neighbors);
void vna_index_to_neighbor_tuple(int64_t index, int num_states,
                                  int num_neighbors, uint8_t* states_out);

/* Compute the adjacency distance between two neighbor tuples.
 * The Hamming distance in the neighborhood state space. */
int vna_neighbor_tuple_hamming(const uint8_t* a, const uint8_t* b,
                                int num_neighbors);

/* --- L8: Advanced Neighborhood Types --- */

/* Generate a circular neighborhood of given radius.
 * Includes all cells whose Euclidean distance from center ≤ radius.
 * This approximates continuous rotational symmetry. */
VnaNeighborhood* vna_neighborhood_circular(double radius, bool include_self);

/* Generate a weighted neighborhood where weights decay with distance.
 * w(d) = exp(-d^2 / (2*sigma^2)) — Gaussian-weighted neighborhood.
 * Used for continuous-state CA and reaction-diffusion models. */
VnaNeighborhood* vna_neighborhood_gaussian(double sigma, int truncate_radius);

#endif /* VNA_NEIGHBORHOOD_H */