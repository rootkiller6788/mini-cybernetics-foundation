#ifndef VNA_REPLICATION_H
#define VNA_REPLICATION_H

#include "vna_core.h"

/* ============================================================================
 * Self-Replication Theory — von Neumann's Universal Constructor
 *
 * Covers: L1 (definitions), L2 (concepts), L3 (mathematical models),
 *         L4 (theorem: existence of self-replication), L5 (algorithms)
 *
 * Fundamental theorem: There exists a cellular automaton configuration
 * that can produce an identical copy of itself (von Neumann, 1940s–1950s).
 *
 * Key references:
 *   von Neumann (1966) — Theory of Self-Reproducing Automata
 *   Burks (1970) — Essays on Cellular Automata
 *   Codd (1968) — Cellular Automata (8-state simplification)
 *   Langton (1984) — Self-Reproduction in Cellular Automata (loops)
 *   Byl (1989) — Self-Reproduction in small cellular automata
 * ============================================================================ */

/* --- L1: Replication Mechanisms --- */

/* Replication strategy — the method by which self-replication is achieved.
 * Universal construction: the configuration contains a complete description
 *   of itself and a mechanism to interpret that description (von Neumann).
 * Template replication: simpler scheme where the pattern copies itself
 *   by direct template matching (like DNA replication).
 * Loop replication: Langton's sheathed loops that propagate and turn. */
typedef enum {
    VNA_REPL_UNIVERSAL_CONSTRUCTION = 0,  /* von Neumann's original */
    VNA_REPL_TEMPLATE             = 1,    /* Direct template copying */
    VNA_REPL_LOOP                 = 2,    /* Langton-style loops */
    VNA_REPL_WIRE                 = 3,    /* Wire-based information transfer */
    VNA_REPL_DIVISION             = 4     /* Cell-division metaphor */
} VnaReplicationStrategy;

/* --- L1: Codd's Self-Replicating Automaton (8 states) --- */

/* Codd (1968) simplified von Neumann's 29-state automaton to 8 states.
 * States: 0=empty, 1=core, 2=sheath, 3=data path, 4=data, 5=cap,
 *          6=injector, 7=trigger.
 * The sheathed data path protects the information signal. */
typedef enum {
    CODD_EMPTY    = 0,
    CODD_CORE     = 1,
    CODD_SHEATH   = 2,
    CODD_DATA_PATH = 3,
    CODD_DATA     = 4,
    CODD_CAP      = 5,
    CODD_INJECTOR = 6,
    CODD_TRIGGER  = 7
} CoddState;

/* --- L3: Langton's Self-Reproducing Loop --- */

/* Langton (1984) discovered a simple self-replicating structure in Codd's
 * 8-state CA. The loop consists of a data path surrounded by a sheath.
 * Information circulates counterclockwise around the loop.
 * When the data signals reach the corner, they trigger construction of
 * a new arm, which eventually forms a new loop. */

/* Loop component types */
typedef enum {
    LANGTON_EMPTY        = 0,
    LANGTON_CORE         = 1,
    LANGTON_SHEATH       = 2,
    LANGTON_DATA_PATH    = 3,
    LANGTON_SIGNAL_0     = 4,
    LANGTON_SIGNAL_1     = 5,
    LANGTON_SIGNAL_2     = 6,
    LANGTON_SIGNAL_3     = 7
} LangtonState;

/* A Langton loop descriptor including its geometric dimensions.
 * The loop is square with side length L. The sheath forms the boundary,
 * and the data path runs just inside the sheath. */
typedef struct {
    int     side_length;        /* Loop side length */
    int     x0, y0;             /* Top-left corner */
    int     sheath_thickness;    /* Thickness of the protective sheath */
    int     signal_count;        /* Number of circulating signals */
    uint8_t* encoded_data;      /* Data encoded in the loop */
    int     data_bits;          /* Number of information bits */
    VnaReplicationPhase phase;  /* Current replication phase */
} VnaLangtonLoop;

/* --- L2: Replication Phase Analysis --- */

/* Initialize a Langton loop at a given position.
 * Returns the pattern that can be inserted into the lattice. */
VnaPattern* vna_langton_loop_pattern(int side_length, int x0, int y0);

/* Extract a Langton loop from the lattice (if present). */
VnaLangtonLoop* vna_langton_loop_extract(VnaLattice* lattice, int x0, int y0);
void vna_langton_loop_free(VnaLangtonLoop* loop);

/* --- L5: Replication Detection and Tracking --- */

/* Detect self-replicating structures in the lattice.
 * Compares the lattice at time t and t+1 to find regions where
 * the configuration is approximately duplicated.
 * Returns the number of replication events detected. */
int vna_detect_replication_events(VnaLattice* before, VnaLattice* after,
                                   int min_size, int* event_x, int* event_y,
                                   int max_events);

/* Track a replication lineage: follow a cell's ancestors.
 * For a cell (x,y) at time t, find its corresponding cell in the
 * parent at time t-1 (or earlier). Returns the ancestor position. */
bool vna_track_lineage(VnaLattice* history[], int num_timesteps,
                        int final_x, int final_y,
                        int* ancestor_x, int* ancestor_y, int* ancestor_t);

/* --- L4: Self-Replication Feasibility --- */

/* Check if a given CA rule supports self-replication.
 * This is a heuristic analysis based on:
 * 1. Existence of a quiescent state
 * 2. Presence of stable sheaths (propagation channels)
 * 3. Signal propagation and crossing capability
 * 4. Constructability of arbitrary patterns
 *
 * Returns a score from 0 (impossible) to 1 (highly likely). */
double vna_rule_replication_score(VnaTransitionRule* rule, int num_states,
                                   VnaNeighborhood* nb);

/* --- L3: Template Replication (Simpler than Universal Construction) --- */

/* Template replication: given a pattern P, find a transition rule
 * that causes P to duplicate itself into an adjacent empty region.
 * This is simpler than universal construction because the pattern
 * does not need to contain a self-description. */
int vna_template_replication_rule(VnaPattern* template_pattern,
                                   int num_states, VnaNeighborhood* nb,
                                   VnaTransitionRule** candidate_rules,
                                   int max_candidates);

/* --- L5: Complexity of Self-Replication --- */

/* Compute the Kolmogorov-chaitin complexity lower bound of a
 * self-replicating configuration. A self-replicator must encode
 * at least its own description → K(C) ≤ |C| + O(1).
 * Returns the estimated description length (bits). */
int64_t vna_replication_complexity_bound(VnaLattice* lattice);

/* --- L7: Artificial Life Application --- */

/* Evolve a population of self-replicating structures over time.
 * Tracks the population count, total biomass, and replication rate. */
typedef struct {
    int64_t population;
    int64_t total_live_cells;
    double  replication_rate;
    double  mutation_rate;
    double  mean_lifespan;
    int64_t generation;
} VnaPopulation;

VnaPopulation* vna_population_create(void);
void vna_population_free(VnaPopulation* pop);
void vna_population_update(VnaPopulation* pop, VnaLattice* before,
                             VnaLattice* after);
void vna_population_print(VnaPopulation* pop);

#endif /* VNA_REPLICATION_H */