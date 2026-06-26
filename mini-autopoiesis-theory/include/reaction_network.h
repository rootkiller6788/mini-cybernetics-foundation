/**
 * @file reaction_network.h
 * @brief Reaction network data structures for autopoietic systems.
 *
 * Defines the reaction network formalism used to model autopoietic organization.
 * A reaction network consists of molecular species (components) and reactions
 * that transform sets of substrates into sets of products, optionally catalyzed.
 *
 * Knowledge coverage:
 *   L1 - Definitions: Reaction, ReactionNetwork, StoichiometricMatrix
 *   L2 - Core concepts: autocatalysis, cross-catalysis, network closure
 *   L3 - Mathematical structures: hypergraph representation, adjacency
 *
 * Reference: Feinberg, "Foundations of Chemical Reaction Network Theory" (2019)
 *            Gánti, "The Principles of Life" (1971/2003)
 */

#ifndef REACTION_NETWORK_H
#define REACTION_NETWORK_H

#include <stddef.h>
#include <stdint.h>

#define RN_MAX_SPECIES  256    /**< Maximum distinct molecular species */
#define RN_MAX_REACTIONS 512   /**< Maximum reactions */
#define RN_MAX_REACTANTS 8     /**< Maximum reactants per reaction */
#define RN_MAX_PRODUCTS 8      /**< Maximum products per reaction */
#define RN_MAX_CATALYSTS 4     /**< Maximum catalysts per reaction */
#define RN_MAX_NAME 64         /**< Maximum species name length */

/**
 * @brief A single molecular species in the reaction network.
 *
 * Maturana & Varela's "components" are modeled as molecular species
 * in the reaction-network formalism.
 */
typedef struct {
    char name[RN_MAX_NAME];       /**< Species identifier */
    double initial_concentration; /**< Initial concentration */
    double concentration;         /**< Current concentration */
    double molecular_weight;      /**< Relative molecular weight */
    int is_food;                  /**< 1 if externally supplied (food set) */
    int is_boundary;              /**< 1 if part of system boundary */
    uint32_t flags;               /**< Species property flags */
} rn_species_t;

#define RN_FLAG_MACROMOLECULE 0x01
#define RN_FLAG_CATALYST      0x02
#define RN_FLAG_TEMPLATE      0x04   /**< Information-carrying (like RNA/DNA) */
#define RN_FLAG_MEMBRANE      0x08   /**< Membrane-forming molecule */

/**
 * @brief A single reaction in the network.
 *
 * Each reaction consumes a set of reactants (substrates) and produces
 * a set of products, potentially accelerated by catalysts.
 * This models the "production relation" in autopoietic theory.
 */
typedef struct {
    char label[RN_MAX_NAME];               /**< Reaction identifier */
    int reactants[RN_MAX_REACTANTS];       /**< Indices of consumed species */
    int reactant_count;
    int reactant_stoich[RN_MAX_REACTANTS]; /**< Stoichiometric coefficients (positive) */
    int products[RN_MAX_PRODUCTS];         /**< Indices of produced species */
    int product_count;
    int product_stoich[RN_MAX_PRODUCTS];   /**< Stoichiometric coefficients (positive) */
    int catalysts[RN_MAX_CATALYSTS];       /**< Indices of catalyzing species */
    int catalyst_count;
    double rate_constant;                  /**< Kinetic rate constant (mass-action) */
    int is_reversible;                     /**< 1 if reaction is reversible */
    double reverse_rate_constant;          /**< Rate constant for reverse reaction */
} rn_reaction_t;

/**
 * @brief The complete reaction network.
 *
 * This is the formal representation of an autopoietic system's
 * production network as a chemical reaction network.
 */
typedef struct {
    rn_species_t species[RN_MAX_SPECIES];
    int species_count;

    rn_reaction_t reactions[RN_MAX_REACTIONS];
    int reaction_count;

    /* Stoichiometric matrix: S[i][j] = net production of species i in reaction j */
    double stoichiometry[RN_MAX_SPECIES][RN_MAX_REACTIONS];

    /* Catalysis matrix: C[i][j] = 1 if species i catalyzes reaction j */
    int catalysis[RN_MAX_SPECIES][RN_MAX_REACTIONS];

    /* Food set indices */
    int food_set[RN_MAX_SPECIES];
    int food_count;
} rn_network_t;

/* ==========================================================================
 * Network Construction
 * ========================================================================== */

/** Initialize an empty reaction network. */
void rn_network_init(rn_network_t *net);

/**
 * @brief Add a species to the network.
 * @return Species index, -1 on error.
 */
int rn_add_species(rn_network_t *net, const char *name,
                   double initial_conc, double mol_weight);

/**
 * @brief Add a reaction to the network.
 * @param reactants Array of reactant species indices.
 * @param reactant_stoich Stoichiometric coefficients for reactants.
 * @param reactant_count Number of reactants.
 * @param products Array of product species indices.
 * @param product_stoich Stoichiometric coefficients for products.
 * @param product_count Number of products.
 * @param catalysts Array of catalyst species indices.
 * @param catalyst_count Number of catalysts.
 * @param rate_constant Forward rate constant.
 * @return Reaction index, -1 on error.
 */
int rn_add_reaction(rn_network_t *net,
                    const int *reactants, const int *reactant_stoich,
                    int reactant_count,
                    const int *products, const int *product_stoich,
                    int product_count,
                    const int *catalysts, int catalyst_count,
                    double rate_constant);

/**
 * @brief Build the stoichiometric and catalysis matrices.
 * Must be called after all species and reactions are added.
 */
void rn_build_matrices(rn_network_t *net);

/* ==========================================================================
 * Network Analysis
 * ========================================================================== */

/**
 * @brief Compute reaction rates using mass-action kinetics.
 *
 * For reaction j with rate constant k, the rate is:
 *   v_j = k_j * ∏_{i} [x_i]^{s_{ij}}   (where s_{ij} is stoichiometric coeff)
 *
 * @param net The reaction network.
 * @param rates Output array of size reaction_count.
 */
void rn_compute_reaction_rates(const rn_network_t *net, double *rates);

/**
 * @brief Compute species concentration derivatives (ODE right-hand side).
 *   dx/dt = S * v(x)
 * where S is the stoichiometric matrix and v(x) are reaction rates.
 *
 * @param net The network.
 * @param derivatives Output array of size species_count.
 */
void rn_compute_derivatives(const rn_network_t *net, double *derivatives);

/**
 * @brief Compute the production set: all species that can be produced
 * given an initial set of available species.
 *
 * @param net The network.
 * @param available Bitmask of available species.
 * @param producible Output bitmask of producible species.
 */
void rn_compute_production_set(const rn_network_t *net,
                                const uint64_t *available,
                                uint64_t *producible);

/**
 * @brief Check if species i is autocatalytic: i catalyzes a reaction
 * that produces more of i.
 *
 * @param net The network.
 * @param species_id Index of species to check.
 * @return 1 if autocatalytic, 0 otherwise.
 */
int rn_is_autocatalytic_species(const rn_network_t *net, int species_id);

/**
 * @brief Find all autocatalytic cycles in the network.
 *
 * An autocatalytic cycle is a set of species where each species
 * catalyzes the production of the next, forming a cycle.
 *
 * @param net The network.
 * @param cycles Output: array of cycle sets (terminated by -1).
 * @param max_cycles Maximum number of cycles to find.
 * @return Number of cycles found.
 */
int rn_find_autocatalytic_cycles(const rn_network_t *net,
                                  int *cycles, int max_cycles);

/* ==========================================================================
 * Network Properties
 * ========================================================================== */

/**
 * @brief Compute the degree of network connectivity.
 *
 * Connectivity = (number of species connected by reactions) / total species.
 *
 * @param net The network.
 * @return Connectivity measure [0, 1].
 */
double rn_network_connectivity(const rn_network_t *net);

/**
 * @brief Check if the network is catalytically closed.
 *
 * Catalytic closure: every reaction is catalyzed by at least one
 * species that is produced within the network.
 *
 * @param net The network.
 * @return 1 if catalytically closed, 0 otherwise.
 */
int rn_is_catalytically_closed(const rn_network_t *net);

/**
 * @brief Compute the autocatalytic capacity of the network.
 *
 * Fraction of species that participate in at least one autocatalytic loop.
 *
 * @param net The network.
 * @return Autocatalytic capacity [0, 1].
 */
double rn_autocatalytic_capacity(const rn_network_t *net);

/**
 * @brief Print a human-readable description of the network.
 */
void rn_network_print(const rn_network_t *net);

/**
 * @brief Free resources associated with the network.
 */
void rn_network_destroy(rn_network_t *net);

#endif /* REACTION_NETWORK_H */
