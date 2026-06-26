/**
 * @file autocatalytic_set.h
 * @brief Detection and analysis of autocatalytic sets (RAF sets).
 *
 * An autocatalytic set is a collection of molecules and reactions where:
 *   1. Every reaction is catalyzed by at least one molecule in the set (R)
 *   2. Every molecule can be produced from a food set using reactions in the set (A)
 *   3. All reactants are either in the food set or produced by the set (F)
 *
 * Knowledge coverage:
 *   L4 - Fundamental laws: RAF theory, autocatalytic closure theorems
 *   L5 - Algorithms: RAF detection, minimal RAF enumeration
 *   L8 - Advanced: stochastic RAF formation, evolvability measures
 *
 * Reference: Hordijk & Steel, "Detecting Autocatalytic, Self-Sustaining Sets
 *            in Chemical Reaction Systems" (2004)
 *            Kauffman, "At Home in the Universe" (1995)
 *            Steel, "The Emergence of a Self-Catalysing Structure in
 *            Abstract Origin-of-Life Models" (2000)
 */

#ifndef AUTOCATALYTIC_SET_H
#define AUTOCATALYTIC_SET_H

#include <stddef.h>
#include <stdint.h>

#define ACS_MAX_MOLECULES  256
#define ACS_MAX_REACTIONS  512

/**
 * @brief A molecule type in the autocatalytic set analysis.
 */
typedef struct {
    int id;
    char name[64];
    int is_in_food_set;    /**< 1 if externally available */
    double concentration;
} acs_molecule_t;

/**
 * @brief A reaction in the autocatalytic set analysis.
 */
typedef struct {
    int id;
    int reactants[8];      /**< Required reactant molecule indices */
    int reactant_count;
    int products[8];       /**< Produced molecule indices */
    int product_count;
    int catalysts[4];      /**< Catalyst molecule indices */
    int catalyst_count;
    double rate;
    int in_raf;            /**< Computed: 1 if in the maximal RAF */
} acs_reaction_t;

/**
 * @brief The complete autocatalytic set system.
 */
typedef struct {
    acs_molecule_t molecules[ACS_MAX_MOLECULES];
    int molecule_count;

    acs_reaction_t reactions[ACS_MAX_REACTIONS];
    int reaction_count;

    int food_set[ACS_MAX_MOLECULES];  /**< Indices of food molecules */
    int food_count;

    /* Computed results */
    int max_raf_reactions[ACS_MAX_REACTIONS]; /**< Maximal RAF reaction indices */
    int max_raf_size;

    int min_raf_reactions[ACS_MAX_REACTIONS]; /**< One minimal RAF */
    int min_raf_size;

    int irreducible_raf_count;  /**< Number of irreducible RAFs found */
} acs_system_t;

/* ==========================================================================
 * System Management
 * ========================================================================== */

void acs_system_init(acs_system_t *acs);
int acs_add_molecule(acs_system_t *acs, const char *name, int is_food);
int acs_add_reaction(acs_system_t *acs,
                      const int *reactants, int reactant_count,
                      const int *products, int product_count,
                      const int *catalysts, int catalyst_count,
                      double rate);
void acs_system_destroy(acs_system_t *acs);

/* ==========================================================================
 * RAF Detection Algorithms
 * ========================================================================== */

/**
 * @brief Find the maximal RAF (Reflexively Autocatalytic and Food-generated) set.
 *
 * The maximal RAF is the union of all RAF subsets. It is unique.
 * Algorithm by Hordijk & Steel (2004): iterative removal of reactions
 * that cannot be sustained.
 *
 * 1. Start with all reactions
 * 2. Remove any reaction whose reactants are not in the closure of the food set
 * 3. Remove any reaction that is not catalyzed by any molecule in the closure
 * 4. Repeat until fixed point
 *
 * Complexity: O(R² · M) where R = reactions, M = molecules.
 *
 * @param acs The ACS system.
 * @return 1 if a non-empty RAF exists, 0 otherwise.
 */
int acs_find_maximal_raf(acs_system_t *acs);

/**
 * @brief Compute the closure of the food set under a given reaction set.
 *
 * closure(F, R) = all molecules producible from F using reactions in R.
 *
 * @param acs The system.
 * @param reaction_mask Bitmask of allowed reactions.
 * @param closure Output: bitmask of produced molecules.
 */
void acs_compute_closure(const acs_system_t *acs,
                          const uint64_t *reaction_mask,
                          uint64_t *closure);

/**
 * @brief Find one minimal RAF (irreducible RAF).
 *
 * An irreducible RAF is a RAF that contains no proper sub-RAF.
 * Finding a minimal one is done by iteratively removing reactions
 * and checking if the RAF property is maintained.
 *
 * @param acs The system (must have maximal RAF already computed).
 * @return 1 if a minimal RAF was found and stored, 0 otherwise.
 */
int acs_find_minimal_raf(acs_system_t *acs);

/**
 * @brief Enumerate all irreducible RAFs up to a limit.
 *
 * Uses randomized algorithm: repeatedly find minimal RAFs by
 * different removal orders.
 *
 * @param acs The system.
 * @param max_enumerate Maximum number of RAFs to find.
 * @return Number of irreducible RAFs found.
 */
int acs_enumerate_irreducible_rafs(acs_system_t *acs, int max_enumerate);

/* ==========================================================================
 * RAF Properties
 * ========================================================================== */

/**
 * @brief Check if a set of reactions forms a RAF.
 *
 * @param acs The system.
 * @param reaction_set Array of reaction indices.
 * @param set_size Number of reactions.
 * @return 1 if RAF, 0 otherwise.
 */
int acs_is_raf(const acs_system_t *acs, const int *reaction_set, int set_size);

/**
 * @brief Compute the RAF diversity: number of distinct molecules in the RAF.
 */
int acs_raf_molecule_count(const acs_system_t *acs);

/**
 * @brief Compute the catalytic closure measure of the RAF.
 *
 * Fraction of reactions in the RAF that are catalyzed by molecules
 * within the RAF.
 */
double acs_catalytic_closure_measure(const acs_system_t *acs);

/**
 * @brief Compute the food-dependency ratio.
 *
 * What fraction of molecules must be supplied externally for the RAF
 * to function?
 */
double acs_food_dependency_ratio(const acs_system_t *acs);

/**
 * @brief Simulate RAF formation via random catalysis addition.
 *
 * Kauffman's hypothesis: when molecular diversity increases,
 * autocatalytic sets emerge spontaneously.
 *
 * @param acs System to simulate on.
 * @param n_steps Number of random catalysis additions.
 * @param threshold Fraction of species that must be catalysts for emergence.
 * @return 1 if a RAF emerges, 0 otherwise.
 */
int acs_simulate_random_emergence(acs_system_t *acs, int n_steps,
                                   double threshold);

/**
 * @brief Print RAF analysis report.
 */
void acs_print_raf_report(const acs_system_t *acs);

#endif /* AUTOCATALYTIC_SET_H */
