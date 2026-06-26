#ifndef VNA_CORE_H
#define VNA_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * Von Neumann Automata — Core Types and Definitions
 *
 * Based on the foundational works of:
 *   John von Neumann  "Theory of Self-Reproducing Automata" (1966, posthumous)
 *   Arthur W. Burks   "Essays on Cellular Automata" (1970, editor)
 *   E. F. Codd        "Cellular Automata" (1968)
 *   Chris Langton     "Self-Reproduction in Cellular Automata" (1984)
 *   Stephen Wolfram   "A New Kind of Science" (2002)
 *   John Conway       "The Game of Life" (1970)
 *   Matthew Cook      "Universality in Elementary Cellular Automata" (2004)
 * ============================================================================ */

/* --- L1: Fundamental Enumerations --- */

/* Neighborhood type — determines which cells influence a cell's next state.
 * von Neumann's original automaton used the VNA_VON_NEUMANN neighborhood.
 * Moore (1962) generalized this to include diagonal neighbors.
 * Margolus (1984) introduced block-based neighborhoods for reversible CA. */
typedef enum {
    VNA_VON_NEUMANN = 0,   /* 4 orthogonal neighbors (N,S,E,W) + center */
    VNA_MOORE       = 1,   /* 8 surrounding neighbors + center */
    VNA_EXTENDED_MOORE = 2,/* Moore with radius r > 1 */
    VNA_MARGOLUS    = 3,   /* 2x2 block neighborhood (reversible CA) */
    VNA_HEXAGONAL   = 4,   /* 6 neighbors on hexagonal lattice */
    VNA_ONE_DIMENSIONAL = 5,/* 1D CA with radius r */
    VNA_CUSTOM      = 6    /* User-defined neighborhood function */
} VnaNeighborhoodType;

/* Boundary condition — how cells at lattice edges behave.
 * Periodic boundaries are standard for theoretical analysis (no edge effects).
 * Fixed boundaries represent physical constraints.
 * Adiabatic boundaries ensure no flow across the boundary. */
typedef enum {
    VNA_BC_PERIODIC  = 0,   /* Wrap-around (torus topology) */
    VNA_BC_FIXED     = 1,   /* Constant edge value */
    VNA_BC_REFLECTIVE = 2,  /* Mirror at boundary */
    VNA_BC_ADIABATIC = 3,   /* Zero-gradient (copy neighbor) */
    VNA_BC_ZERO      = 4    /* Zero padding outside lattice */
} VnaBoundaryCondition;

/* Update mode — synchronous vs asynchronous update.
 * Classical CA uses synchronous update (all cells simultaneously).
 * Asynchronous update can model physical systems more realistically.
 * Stochastic asynchronous uses random cell selection order. */
typedef enum {
    VNA_UPDATE_SYNC      = 0, /* All cells update simultaneously */
    VNA_UPDATE_ASYNC_FIXED = 1, /* Fixed sequential order */
    VNA_UPDATE_ASYNC_RANDOM = 2, /* Random cell selection each step */
    VNA_UPDATE_BLOCK_SYNC  = 3, /* Block-synchronous (Margolus) */
    VNA_UPDATE_CLOCKED     = 4  /* Clocked blocks (Toffoli-Margolus) */
} VnaUpdateMode;

/* CA state type — a cell can be in one of finitely many states.
 * Von Neumann's original automaton used 29 states.
 * Conway's Game of Life uses 2 states (dead/alive).
 * Langton's loops used 8 states. */
typedef enum {
    VNA_STATE_BINARY  = 0,  /* 2 states: {0,1} */
    VNA_STATE_TERNARY = 1,  /* 3 states: {0,1,2} */
    VNA_STATE_CODD    = 2,  /* 8 states (Codd's self-replicating CA) */
    VNA_STATE_LANGTON = 3,  /* 8 states (Langton's loops) */
    VNA_STATE_VON_NEUMANN = 4, /* 29 states (original von Neumann CA) */
    VNA_STATE_CUSTOM  = 5   /* User-defined state count */
} VnaStateType;

/* Lattice topology — the geometric arrangement of cells.
 * Square lattice is the most common (Z^2).
 * Hexagonal lattice has better isotropy for physical modeling.
 * Triangular lattice has higher coordination number. */
typedef enum {
    VNA_LATTICE_1D = 0,
    VNA_LATTICE_2D_SQUARE = 1,
    VNA_LATTICE_2D_HEXAGONAL = 2,
    VNA_LATTICE_2D_TRIANGULAR = 3,
    VNA_LATTICE_3D_CUBIC = 4
} VnaLatticeTopology;

/* --- L3: Core Mathematical Structures --- */

/* A single configuration of the cellular automaton lattice.
 * Represents the function c: Z^d → S at a given time step.
 * Internally stored as a flat array with row-major ordering.
 * The ghost layer stores boundary-adjacent cells for efficient updates. */
typedef struct {
    int           width;          /* Lattice width (x-dimension) */
    int           height;         /* Lattice height (y-dimension) */
    int           depth;          /* Lattice depth (z-dimension, 1 for 2D) */
    int           num_states;     /* |S| — cardinality of state set */
    uint8_t*      cells;          /* Cell states, size = width*height*depth */
    uint8_t*      next_cells;     /* Double buffer for synchronous update */
    uint8_t*      ghost_layer;    /* Boundary ghost cells for BC handling */
    int           ghost_width;    /* Width of ghost layer (typically 1) */
    VnaBoundaryCondition bc_x;    /* Boundary condition in x direction */
    VnaBoundaryCondition bc_y;    /* Boundary condition in y direction */
    VnaBoundaryCondition bc_z;    /* Boundary condition in z direction */
    VnaLatticeTopology topology;  /* Lattice geometry type */
    VnaUpdateMode update_mode;    /* Synchronous vs asynchronous */
    int64_t       generation;     /* Current generation counter */
} VnaLattice;

/* Neighborhood descriptor — specifies the relative offsets of neighbor cells.
 * For von Neumann: offsets = {(0,-1), (-1,0), (0,0), (1,0), (0,1)}
 * For Moore (r=1): 3x3 grid centered on cell
 * For Margolus: block of 2x2 cells */
typedef struct {
    VnaNeighborhoodType type;
    int  radius;                  /* Neighborhood radius */
    int  num_neighbors;           /* |N| — total number of neighbors including self */
    int* offset_dx;               /* x-offsets for each neighbor */
    int* offset_dy;               /* y-offsets for each neighbor */
    int* offset_dz;               /* z-offsets (NULL for 2D) */
    bool include_self;            /* Whether the center cell is included */
} VnaNeighborhood;

/* Transition rule — the local update function f: S^{|N|} → S.
 * For elementary CA (1D, 2-state, radius 1): 2^3 = 8 possible neighborhood
 * configurations, each mapping to 0 or 1 → 2^8 = 256 possible rules.
 * Wolfram codes encode rules as integers 0-255.
 * For 2D CA with k states and n neighbors: k^n entries in rule table. */
typedef struct {
    int           num_states;      /* |S| */
    int           num_neighbors;   /* |N| */
    uint8_t*      rule_table;      /* Size: num_states^num_neighbors entries */
    bool          is_totalistic;   /* True if rule depends only on sum of neighbor states */
    bool          is_outer_totalistic; /* True if rule depends on sum of neighbors + center */
    int64_t       wolfram_code;    /* Wolfram code (for 1D binary CA) */
    char*         name;            /* Human-readable rule name */
    char*         description;     /* Rule behavior description */
} VnaTransitionRule;

/* CA state histogram — distribution of states across the lattice.
 * Used to compute entropy and other statistical properties. */
typedef struct {
    double* state_counts;         /* Histogram counts per state */
    double* state_probs;          /* Normalized probabilities */
    double  shannon_entropy;      /* Shannon entropy H(S) */
    int     dominant_state;       /* Most frequent state */
    double  dominant_fraction;    /* Fraction of lattice in dominant state */
    int     num_states;           /* Number of possible states */
} VnaStateHistogram;

/* CA statistics — aggregate lattice properties.
 * Used to characterize the macrostate of the automaton. */
typedef struct {
    double  density;              /* Average cell value (for binary: fraction alive) */
    double  activity;             /* Fraction of cells that changed this step */
    double  spatial_entropy;      /* Shannon entropy of the lattice state distribution */
    double  mutual_information;   /* I(cell; neighborhood) mutual information */
    double  lyapunov_approx;      /* Approximate Lyapunov exponent (damage spreading) */
    int64_t active_clusters;      /* Number of connected active regions */
    double  cluster_size_mean;    /* Mean cluster size */
    double  cluster_size_variance;/* Variance of cluster size */
    double  correlation_length;   /* Spatial correlation decay length */
    int     period_estimate;      /* Estimated temporal period (0 if not periodic) */
    double  glider_count_estimate;/* Estimated number of moving patterns */
} VnaCAStatistics;

/* Pattern — a contiguous subregion of the lattice.
 * Used for pattern insertion, extraction, and matching operations. */
typedef struct {
    int      width;
    int      height;
    uint8_t* data;               /* Row-major pattern data */
    char*    name;
    char*    origin;             /* Source attribution */
    int      period;             /* Temporal period (1=still life, 2+=oscillator, 0=others) */
    int      dx, dy;             /* Displacement per period (for spaceships) */
    bool     is_glider;          /* True if the pattern is a spaceship */
} VnaPattern;

/* --- L1: Von Neumann's Universal Constructor Types --- */

/* Construction arm state — von Neumann's constructor uses a moving arm
 * to build the offspring automaton. The arm can extend, retract, turn,
 * and inject states into target cells. */
typedef enum {
    VNA_ARM_IDLE      = 0,  /* Arm at rest */
    VNA_ARM_EXTENDING = 1,  /* Arm growing toward target */
    VNA_ARM_INJECTING = 2,  /* Arm placing state into target cell */
    VNA_ARM_RETRACTING = 3,/* Arm withdrawing */
    VNA_ARM_TURNING   = 4   /* Arm changing direction */
} VnaArmState;

/* A single instruction in the construction tape.
 * The tape is a 1D sequence of instructions that the constructor reads
 * and executes. Von Neumann's original tape used 29 states per cell. */
typedef struct {
    uint8_t operation;           /* Operation code */
    uint8_t argument;            /* Argument (state to inject, direction, etc.) */
    int     repeat_count;        /* Number of times to repeat (-1 = infinite) */
} VnaTapeInstruction;

/* The construction tape — a 1D cellular automaton that encodes
 * the blueprint for building the offspring. This is von Neumann's
 * key insight: the information for self-replication is stored
 * on a tape that is both read and copied. */
typedef struct {
    VnaTapeInstruction* instructions;
    int  length;
    int  capacity;
    int  position;               /* Current read position */
    bool is_looping;             /* Whether the tape loops infinitely */
} VnaConstructionTape;

/* Universal constructor — the full von Neumann self-replicating machine.
 * Composed of: construction arm + construction tape + control automaton.
 * The constructor reads from the tape, interprets instructions, and
 * constructs a copy of itself (including a copy of the tape). */
typedef struct {
    VnaArmState        arm_state;
    int                arm_x, arm_y;       /* Arm tip position */
    int                arm_direction;      /* 0=N, 1=E, 2=S, 3=W */
    int                arm_length;         /* Current arm extension */
    VnaConstructionTape tape;
    bool               tape_copied;        /* Whether tape has been duplicated */
    int                construction_phase; /* Current phase of construction */
    int                offspring_progress; /* % of offspring completed */
} VnaUniversalConstructor;

/* --- L2: Self-Replication Cycle States --- */

/* The self-replication cycle proceeds through distinct phases.
 * Von Neumann identified these phases in his theoretical analysis.
 * Phase 0-3: Reading and interpreting the tape
 * Phase 4-6: Constructing the offspring body
 * Phase 7-8: Copying the tape and separating */
typedef enum {
    VNA_REPL_IDLE      = 0,  /* No replication in progress */
    VNA_REPL_READ_TAPE = 1,  /* Reading construction instructions */
    VNA_REPL_BUILD_ARM = 2,  /* Constructing the arm mechanism */
    VNA_REPL_BUILD_BODY = 3, /* Constructing the automaton body */
    VNA_REPL_COPY_TAPE = 4,  /* Duplicating the construction tape */
    VNA_REPL_SEPARATE  = 5,  /* Separating parent and offspring */
    VNA_REPL_DONE      = 6   /* Replication complete */
} VnaReplicationPhase;

/* --- Core API Declarations --- */

/* Lattice management */
VnaLattice* vna_lattice_create(int width, int height, int num_states,
                                VnaLatticeTopology topology);
void vna_lattice_free(VnaLattice* lattice);
void vna_lattice_set_boundary(VnaLattice* lattice, VnaBoundaryCondition bc_x,
                               VnaBoundaryCondition bc_y, VnaBoundaryCondition bc_z);
void vna_lattice_set_update_mode(VnaLattice* lattice, VnaUpdateMode mode);
uint8_t vna_lattice_get_cell(VnaLattice* lattice, int x, int y, int z);
void vna_lattice_set_cell(VnaLattice* lattice, int x, int y, int z, uint8_t state);
void vna_lattice_clear(VnaLattice* lattice, uint8_t fill_state);
void vna_lattice_randomize(VnaLattice* lattice, double density, unsigned int seed);
void vna_lattice_randomize_seeded(VnaLattice* lattice, double density,
                                   unsigned int seed);

/* Lattice I/O */
void vna_lattice_print_ascii(VnaLattice* lattice, int x0, int y0,
                              int view_w, int view_h);
void vna_lattice_export_pgm(VnaLattice* lattice, const char* filename);
void vna_lattice_import_pgm(VnaLattice* lattice, const char* filename,
                             int* offset_x, int* offset_y);

/* Neighborhood construction */
VnaNeighborhood* vna_neighborhood_create(VnaNeighborhoodType type, int radius,
                                          int num_states);
void vna_neighborhood_free(VnaNeighborhood* nb);
int vna_neighborhood_get_num_neighbors(VnaNeighborhoodType type, int radius);

/* Transition rule operations */
VnaTransitionRule* vna_rule_create(int num_states, int num_neighbors);
void vna_rule_free(VnaTransitionRule* rule);
void vna_rule_set_entry(VnaTransitionRule* rule, const uint8_t* neighbor_states,
                         uint8_t output);
uint8_t vna_rule_lookup(VnaTransitionRule* rule, const uint8_t* neighbor_states);
void vna_rule_from_wolfram(VnaTransitionRule* rule, int wolfram_code);
int vna_rule_to_wolfram(VnaTransitionRule* rule);
void vna_rule_set_totalistic(VnaTransitionRule* rule, const uint8_t* sum_to_state,
                              int max_sum);
VnaTransitionRule* vna_rule_compose(VnaTransitionRule* r1, VnaTransitionRule* r2);

/* CA evolution */
int vna_lattice_evolve(VnaLattice* lattice, VnaNeighborhood* nb,
                        VnaTransitionRule* rule);
int vna_lattice_evolve_n_steps(VnaLattice* lattice, VnaNeighborhood* nb,
                                VnaTransitionRule* rule, int n);
void vna_lattice_evolve_async(VnaLattice* lattice, VnaNeighborhood* nb,
                               VnaTransitionRule* rule, double update_prob);

/* Pattern operations */
VnaPattern* vna_pattern_create(int width, int height, const uint8_t* data,
                                const char* name);
void vna_pattern_free(VnaPattern* pattern);
void vna_pattern_insert(VnaLattice* lattice, VnaPattern* pattern,
                         int x0, int y0);
VnaPattern* vna_pattern_extract(VnaLattice* lattice, int x0, int y0,
                                 int w, int h);
double vna_pattern_match(VnaLattice* lattice, VnaPattern* pattern,
                          int* best_x, int* best_y);
VnaPattern* vna_pattern_rotate(VnaPattern* pattern, int quarters);
VnaPattern* vna_pattern_reflect(VnaPattern* pattern, bool horizontal);

/* Histogram and statistics */
VnaStateHistogram* vna_histogram_compute(VnaLattice* lattice);
void vna_histogram_free(VnaStateHistogram* hist);
VnaCAStatistics* vna_statistics_compute(VnaLattice* lattice,
                                         VnaNeighborhood* nb);
void vna_statistics_free(VnaCAStatistics* stats);
void vna_statistics_print(VnaCAStatistics* stats);

/* Universal constructor operations */
VnaUniversalConstructor* vna_constructor_create(void);
void vna_constructor_free(VnaUniversalConstructor* uc);
void vna_constructor_add_instruction(VnaUniversalConstructor* uc,
                                      uint8_t op, uint8_t arg, int repeat);
bool vna_constructor_step(VnaUniversalConstructor* uc, VnaLattice* target);
VnaReplicationPhase vna_constructor_get_phase(VnaUniversalConstructor* uc);

/* Fault tolerance utilities */
double vna_error_probability_network(int depth, double gate_error_prob);
double vna_multiplexing_reliability(int multiplexing_degree,
                                     double component_error);
int vna_optimal_multiplexing_degree(double component_error,
                                     double target_error);

#endif /* VNA_CORE_H */