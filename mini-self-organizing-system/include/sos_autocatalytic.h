#ifndef SOS_AUTOCATALYTIC_H
#define SOS_AUTOCATALYTIC_H

#include "sos_core.h"

/* ============================================================================
 * Autocatalytic Sets, Hypercycles, and NK Fitness Landscapes
 *
 * Eigen & Schuster (1977): Hypercycles — self-organizing catalytic cycles
 * Kauffman (1971, 1993): Autocatalytic sets and the origins of order
 * Hordijk & Steel (2004): RAF (Reflexively Autocatalytic and Food-generated) sets
 * Kauffman & Levin (1987): NK fitness landscapes
 * ============================================================================ */

/* --- Molecule/Reaction Types --- */
typedef enum {
    MOLECULE_FOOD      = 0,  /* Externally supplied (ambient) */
    MOLECULE_CATALYST  = 1,  /* Catalyzes reactions */
    MOLECULE_PRODUCT   = 2,  /* Produced by reactions */
    MOLECULE_INTERMEDIATE = 3  /* Transient intermediate */
} MoleculeType;

/* --- A single molecule species --- */
typedef struct {
    int           id;
    char*         name;
    MoleculeType  type;
    double        concentration;
    double        formation_rate;    /* Rate of spontaneous formation */
    double        decay_rate;        /* First-order decay rate */
} Molecule;

/* --- A chemical reaction A + B -> C, catalyzed by K --- */
typedef struct {
    int           id;
    int           substrate_ids[2];  /* Reactant molecule IDs (-1 if none) */
    int           n_substrates;
    int           product_id;        /* Product molecule ID */
    int           catalyst_id;       /* Catalyst molecule ID (-1 if uncatalyzed) */
    double        rate_constant;     /* k for catalyzed, k0 for uncatalyzed */
    double        catalytic_power;   /* k_cat / k0: catalytic enhancement factor */
} CatalyzedReaction;

/* --- A set of molecules and catalyzed reactions --- */
typedef struct {
    int               n_molecules;
    int               n_reactions;
    Molecule*         molecules;
    CatalyzedReaction* reactions;

    /* Adjacency: which molecule catalyzes which reaction */
    int**             catalysis_matrix;  /* [mol][rxn] = catalyzes? */
    int**             reaction_products; /* [rxn][mol] = produces? */

    /* Food set: indices of externally supplied molecules */
    int               n_food;
    int*              food_set;

    /* RAF analysis results */
    int*              raf_set;       /* Indices of molecules in the RAF */
    int               n_raf;
    int               has_raf;       /* 1 if RAF exists, 0 otherwise */
} CatalyticNetwork;

/* --- Eigen's Hypercycle (Eigen & Schuster 1977) ---
 * A hypercycle is a cyclic network of self-replicating entities where
 * each member catalyzes the replication of the next.
 *   I_1 -> I_2 -> I_3 -> ... -> I_n -> I_1
 * Hypercycles exhibit cooperative self-organization and can overcome
 * the error threshold (Eigen's paradox).
 */
typedef struct {
    int               n_species;
    double*           concentrations;  /* x_i: concentration of species i */
    double*           fitness;         /* A_i: replication rate */
    double*           quality;         /* Q_i: copying fidelity (0..1) */
    double**          coupling;        /* k_{ij}: catalytic coupling */
    double            total_concentration; /* Sum of all x_i (constant) */
    double            mutation_rate;
    double            mean_fitness;    /* Weighted average fitness */
    int               coexist;         /* All species coexist? */
} Hypercycle;

/* --- NK Fitness Landscape (Kauffman & Levin 1987) ---
 * N = number of genes (binary loci)
 * K = number of epistatic interactions per gene (0..N-1)
 * Fitness f: {0,1}^N -> [0,1] is determined by local interactions.
 *
 * K=0: single smooth peak (additive, fully correlated)
 * K=N-1: fully random landscape (uncorrelated, many local optima)
 * Intermediate K: "complexity catastrophe" — tunable ruggedness
 */
typedef struct {
    int               N;      /* String length (# of genes) */
    int               K;      /* Epistasis parameter */
    int               genome_len;
    double***         fitness_table;  /* fitness_table[gene][2^K context] = random fitness */
} NKLandscape;

/* ===== Catalytic Network Functions ===== */

CatalyticNetwork* cn_create(void);
void cn_free(CatalyticNetwork* cn);

/** Add a molecule species. Returns index. */
int cn_add_molecule(CatalyticNetwork* cn, const char* name, MoleculeType type);

/** Add a catalyzed reaction. Returns index. */
int cn_add_reaction(CatalyticNetwork* cn, int sub1, int sub2,
                    int product, int catalyst, double k_cat, double k_uncat);

/** Set the food set (externally supplied molecules). */
void cn_set_food(CatalyticNetwork* cn, const int* food_ids, int n_food);

/** Compute the RAF set using the Hordijk-Steel (2004) algorithm.
 *  A set R of molecules is Reflexively Autocatalytic and Food-generated (RAF) if:
 *    1. Every reaction in R has all substrates either in the food set or produced by R
 *    2. Every molecule in R (not in food) is produced by some reaction in R
 *  Returns: size of the maximal RAF set. */
int cn_find_raf(CatalyticNetwork* cn);

/** Check if the network is collectively autocatalytic.
 *  A network is collectively autocatalytic if the closure of the food set
 *  under all catalyzed reactions equals the full molecule set. */
int cn_is_autocatalytic(const CatalyticNetwork* cn);

/** Simulate the catalytic network dynamics using ODE integration.
 *  dx_i/dt = formation_i + sum(production) - sum(consumption) - decay_i * x_i */
void cn_simulate_dynamics(CatalyticNetwork* cn, double dt, int steps);

/** Compute catalytic closure: the set of molecules reachable from the food set. */
int cn_catalytic_closure(CatalyticNetwork* cn, int* closure_set);

/** Get concentration of a molecule by index. */
double cn_get_concentration(const CatalyticNetwork* cn, int mol_id);

/* ===== Hypercycle Functions (Eigen & Schuster 1977) ===== */

Hypercycle* hc_create(int n_species);
void hc_free(Hypercycle* hc);
void hc_set_fitness(Hypercycle* hc, const double* fitness);
void hc_set_quality(Hypercycle* hc, const double* quality);
void hc_set_coupling(Hypercycle* hc, double strength);
void hc_set_mutation_rate(Hypercycle* hc, double mu);

/** Evolve the hypercycle by one timestep using the deterministic ODE:
 *  dx_i/dt = x_i (A_i Q_i - phi) + sum_{j != i} k_{ji} x_j
 *  where phi = sum_i A_i x_i maintains constant total concentration. */
void hc_evolve(Hypercycle* hc, double dt);

/** Check if all species coexist (none below extinction threshold). */
int hc_check_coexistence(const Hypercycle* hc, double threshold);

/** Compute the error threshold: the maximum mutation rate for which
 *  the master sequence can be maintained (Eigen 1971).
 *  Q_min = 1 / sigma_m where sigma_m = A_master / A_mutant_max */
double hc_error_threshold(const Hypercycle* hc);

/* ===== NK Fitness Landscape Functions ===== */

NKLandscape* nk_create(int N, int K);
void nk_free(NKLandscape* nk);

/** Compute the fitness of a binary genome string.
 *  Each gene's contribution depends on itself and K neighboring genes. */
double nk_fitness(const NKLandscape* nk, const int* genome);

/** Find a local fitness optimum by greedy hill-climbing (1-mutant neighborhood). */
int nk_local_optimum(const NKLandscape* nk, int* genome, int max_steps);

/** Count the number of local optima by sampling random starting points. */
int nk_count_local_optima(const NKLandscape* nk, int n_samples);

/** Compute the autocorrelation of fitness along a random walk on the landscape.
 *  Weinberger (1990): correlation length = 1 / (K+1) * N */
double nk_fitness_autocorrelation(const NKLandscape* nk, int walk_length);

/** Generate a random binary genome. */
void nk_random_genome(const NKLandscape* nk, int* genome);

#endif /* SOS_AUTOCATALYTIC_H */
