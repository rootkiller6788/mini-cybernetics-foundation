#ifndef SOS_CELLULAR_H
#define SOS_CELLULAR_H

#include "sos_core.h"

/* ============================================================================
 * Cellular Automata Models of Self-Organization
 *
 * Conway's Game of Life (1970), Wolfram's elementary CA (1983, 2002),
 * Bak-Tang-Wiesenfeld sandpile (1987), Langton's lambda parameter (1990).
 * ============================================================================ */

/* --- Wolfram's Four Universality Classes (Wolfram 1983) --- */
typedef enum {
    CA_CLASS_I   = 0,  /* Fixed point: evolution to homogeneous state */
    CA_CLASS_II  = 1,  /* Periodic: evolution to simple separated periodic structures */
    CA_CLASS_III = 2,  /* Chaotic: evolution to chaotic aperiodic patterns */
    CA_CLASS_IV  = 3   /* Complex: evolution to complex localized structures (edge of chaos) */
} CAClass;

/* --- Cell State for Game of Life --- */
typedef enum {
    CELL_DEAD  = 0,
    CELL_ALIVE = 1
} CellState;

/* --- Boundary Condition Types --- */
typedef enum {
    CA_BOUNDARY_PERIODIC   = 0,  /* Toroidal wrap-around */
    CA_BOUNDARY_REFLECTIVE = 1,  /* Mirror at edges */
    CA_BOUNDARY_FIXED      = 2,  /* Constant boundary value */
    CA_BOUNDARY_ABSORBING  = 3   /* Edges act as sinks */
} CABoundaryType;

/* --- Neighborhood Types --- */
typedef enum {
    CA_NEIGHBOR_MOORE     = 0,  /* 8 neighbors (square grid) */
    CA_NEIGHBOR_VON_NEUMANN = 1,  /* 4 neighbors (cross) */
    CA_NEIGHBOR_HEXAGONAL = 2   /* 6 neighbors (hex grid) */
} CANeighborhoodType;

/* --- 2D Cellular Automaton --- */
typedef struct {
    int               width;
    int               height;
    CellState**       grid;       /* Current generation */
    CellState**       next_grid;  /* Next generation buffer */
    CABoundaryType    boundary;
    CANeighborhoodType neighborhood;
    CAClass           ca_class;   /* Wolfram classification */
    int               generation;
    double            lambda;     /* Langton's lambda: fraction of rules leading to non-quiescent */
    char*             rule_name;  /* e.g., "B3/S23" for Game of Life */
    int               survive_counts[9];  /* Survival condition bitmask */
    int               birth_counts[9];    /* Birth condition bitmask */
} CellularAutomaton;

/* --- 1D Elementary Cellular Automaton (Wolfram 1983) --- */
typedef struct {
    int               width;
    int*              cells;
    int*              next_cells;
    int               rule_number;  /* Wolfram rule 0-255 */
    int               generation;
    CABoundaryType    boundary;
} ElementaryCA;

/* --- Sandpile Model (Bak, Tang, Wiesenfeld 1987) --- */
typedef struct {
    int               width;
    int               height;
    int**             pile;        /* Number of grains at each site */
    int               threshold;   /* Toppling threshold (typically 4 for square) */
    long long         total_grains;
    long long         total_avalanches;
    long long*        avalanche_sizes;  /* History of avalanche sizes */
    int               aval_history_len;
    int               aval_history_cap;
    double*           aval_size_dist;   /* Size distribution P(s) */
    double*           aval_time_dist;   /* Lifetime distribution P(t) */
} SandpileModel;

/* ===== Cellular Automaton Lifecycle ===== */

CellularAutomaton* ca_create(int width, int height, const char* rule_name);
void ca_free(CellularAutomaton* ca);
void ca_set_boundary(CellularAutomaton* ca, CABoundaryType btype);
void ca_set_neighborhood(CellularAutomaton* ca, CANeighborhoodType ntype);

/* Set a specific cell state. */
void ca_set_cell(CellularAutomaton* ca, int x, int y, CellState state);
CellState ca_get_cell(const CellularAutomaton* ca, int x, int y);

/* Randomize the grid with given density of live cells. */
void ca_randomize(CellularAutomaton* ca, double density);

/* Evolve one generation according to the birth/survival rule. */
void ca_evolve(CellularAutomaton* ca);

/* ===== Pattern Analysis ===== */

/* Count live neighbor cells at (x,y) according to neighborhood type. */
int ca_count_neighbors(const CellularAutomaton* ca, int x, int y);

/* Count total live cells in the grid. */
int ca_live_count(const CellularAutomaton* ca);

/* Compute the density of living cells. */
double ca_density(const CellularAutomaton* ca);

/* Detect still life patterns (period-1 oscillators). Returns count. */
int ca_detect_still_life(const CellularAutomaton* ca);

/* Detect oscillators of period p. Returns number found. */
int ca_detect_oscillators(CellularAutomaton* ca, int period);

/* Compute Hamming distance between two CA states. */
int ca_hamming_distance(const CellularAutomaton* ca1, const CellularAutomaton* ca2);

/* Compute Lyapunov exponent for CA damage spreading (Bagnoli et al. 1992). */
double ca_damage_spreading(CellularAutomaton* ca, int steps);

/* Estimate Langton's lambda parameter for a given rule. */
double ca_lambda_parameter(const CellularAutomaton* ca);

/* Classify CA into Wolfram's four universality classes by
 * analyzing density evolution and entropy rate. */
CAClass ca_classify_wolfram(const CellularAutomaton* ca, int max_steps);

/* ===== Elementary 1D CA (Wolfram) ===== */

ElementaryCA* eca_create(int width, int rule_number);
void eca_free(ElementaryCA* eca);
void eca_set_cell(ElementaryCA* eca, int x, int value);
int eca_get_cell(const ElementaryCA* eca, int x);
void eca_randomize(ElementaryCA* eca, double density);
void eca_evolve(ElementaryCA* eca);

/* Encode the current ECA row as a Wolfram rule number for the next step. */
int eca_compute_rule(const ElementaryCA* eca, int x);

/* Compute the entropy of the current spatial pattern. */
double eca_spatial_entropy(const ElementaryCA* eca);

/* ===== Sandpile Model (Self-Organized Criticality) ===== */

SandpileModel* sp_create(int width, int height, int threshold);
void sp_free(SandpileModel* sp);

/* Add a grain at position (x,y). If the site exceeds threshold,
 * topple and propagate avalanche. Returns size of triggered avalanche. */
int sp_add_grain(SandpileModel* sp, int x, int y);

/* Add N grains at random positions. Returns total avalanche size. */
int sp_add_random_grains(SandpileModel* sp, int n_grains);

/* Let the sandpile relax fully: topple all over-threshold sites
 * until the pile is stable. Returns number of topplings. */
int sp_relax(SandpileModel* sp);

/* Compute the avalanche size distribution P(s) by sampling many avalanches.
 * Fills sp->aval_size_dist with log-binned histogram. */
void sp_compute_size_distribution(SandpileModel* sp, int n_samples, int n_bins);

/* Get total grains in the pile. */
long long sp_total_grains(const SandpileModel* sp);

/* Get average slope / roughness of the pile. */
double sp_average_slope(const SandpileModel* sp);

#endif /* SOS_CELLULAR_H */
