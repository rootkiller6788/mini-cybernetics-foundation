/**
 * @file organizational_closure.h
 * @brief Algorithms for computing organizational closure in autopoietic systems.
 *
 * Organizational closure is the fundamental property of autopoietic systems:
 * all components of the system are produced by the network of productions
 * that they themselves constitute.
 *
 * Knowledge coverage:
 *   L4 - Fundamental laws: organizational invariance, closure theorems
 *   L5 - Algorithms: production closure, catalytic closure, fixed-point computation
 *
 * Reference: Maturana & Varela, "Autopoiesis: The Organization of the Living" (1973)
 *            Letelier et al., "Organizational Invariance and Metabolic Closure" (2011)
 *            Montevil & Mossio, "Biological Organisation as Closure of Constraints" (2015)
 */

#ifndef ORGANIZATIONAL_CLOSURE_H
#define ORGANIZATIONAL_CLOSURE_H

#include <stddef.h>
#include <stdint.h>

#define OC_MAX_COMPONENTS 256
#define OC_MAX_REACTIONS  512

/**
 * @brief Representation of organizational closure analysis.
 *
 * Components are nodes; reactions are hyperedges connecting substrates to products.
 * Catalysis forms a second graph overlaid on the reaction hypergraph.
 */
typedef struct {
    int n_components;                     /**< Number of components */
    int n_reactions;                      /**< Number of reactions */

    /* Reaction data: for each reaction, which components are substrates/products */
    int **reaction_substrates;            /**< Substrate sets per reaction */
    int *reaction_substrate_counts;
    int **reaction_products;              /**< Product sets per reaction */
    int *reaction_product_counts;

    /* Catalysis: catalysis[i][j] = 1 if component i catalyzes reaction j */
    int **catalysis;

    /* Self-production: self_prod[i] = 1 if component i is produced by some reaction */
    int *is_produced;

    /* Boundary components */
    int *is_boundary;
    int boundary_count;

    /* Workspace */
    int *reachable;                       /**< BFS/DFS reachability from initial set */
    int *closure_set;                     /**< Computed closure */
    int closure_size;
} oc_system_t;

/* ==========================================================================
 * System Management
 * ========================================================================== */

/** Initialize an organizational closure analysis system. */
void oc_system_init(oc_system_t *sys, int n_components, int n_reactions);

/** Add a reaction to the closure system. */
int oc_add_reaction(oc_system_t *sys, int reaction_id,
                     const int *substrates, int substrate_count,
                     const int *products, int product_count);

/** Mark component i as catalyzing reaction j. */
void oc_set_catalysis(oc_system_t *sys, int component_id, int reaction_id, int value);

/** Mark component i as a boundary component. */
void oc_set_boundary(oc_system_t *sys, int component_id);

/** Free all resources. */
void oc_system_destroy(oc_system_t *sys);

/* ==========================================================================
 * Closure Operations — Core Theorems
 * ========================================================================== */

/**
 * @brief Theorem: Production Closure (L4)
 *
 * Given an initial set of available components F (food set),
 * compute the set of all components that can be produced through
 * any sequence of reactions using already-available substrates.
 *
 * This is the fundamental operation underlying autopoietic organization.
 *
 * Algorithm: iterative fixed-point — repeatedly apply all reactions
 * whose substrates are available, adding their products, until no
 * new components can be produced.
 *
 * @param sys The system.
 * @param food_set Indices of initially available components.
 * @param food_count Size of food set.
 * @param closure Output: reachable component indices.
 * @param closure_size Output: number of reachable components.
 */
void oc_production_closure(const oc_system_t *sys,
                            const int *food_set, int food_count,
                            int *closure, int *closure_size);

/**
 * @brief Theorem: Catalytic Closure (L4)
 *
 * Compute the set of reactions that are catalyzed by a given set of
 * components. A reaction is catalytically accessible if at least one
 * of its catalysts is in the given component set.
 *
 * @param sys The system.
 * @param component_set Set of available components (as bitmask-like array).
 * @param component_count Number of components.
 * @param catalyzed_reactions Output: indices of catalyzed reactions.
 * @param cat_count Output: number of catalyzed reactions.
 */
void oc_catalytic_closure(const oc_system_t *sys,
                           const int *component_set, int component_count,
                           int *catalyzed_reactions, int *cat_count);

/**
 * @brief Theorem: Organizational Closure (fixed point)
 *
 * Compute the organizational closure: the set of components that are
 * both produced by the system AND whose catalysts are produced by the system.
 *
 * This is the simultaneous fixed point of production closure and
 * catalytic closure, corresponding to the autopoietic condition
 * described by Maturana & Varela.
 *
 * @param sys The system.
 * @param initial_set Starting set.
 * @param initial_count Size of starting set.
 * @param org_closure Output: organizational closure set.
 * @param org_size Output: size of organizational closure.
 * @param converged Output: 1 if fixed point reached, 0 if oscillating.
 */
void oc_organizational_closure(const oc_system_t *sys,
                                const int *initial_set, int initial_count,
                                int *org_closure, int *org_size,
                                int *converged);

/**
 * @brief Theorem: Minimal Organizational Set (L4)
 *
 * Find the minimal set of components that achieves organizational closure.
 * This corresponds to the "core" of the autopoietic system.
 *
 * Uses a heuristic greedy algorithm: iteratively remove components and
 * check if closure is maintained.
 *
 * @param sys The system.
 * @param minimal_set Output: minimal component set.
 * @param min_size Output: size of minimal set.
 * @return 0 on success.
 */
int oc_find_minimal_organizational_set(const oc_system_t *sys,
                                        int *minimal_set, int *min_size);

/* ==========================================================================
 * Closure Metrics
 * ========================================================================== */

/**
 * @brief Compute the closure index: |closure| / |total components|.
 *
 * A value of 1.0 means all components are in the organizational closure.
 */
double oc_closure_index(const oc_system_t *sys);

/**
 * @brief Compute the dependency depth of each component.
 *
 * Dependency depth is the length of the longest production chain
 * from the food set to the component.
 *
 * @param sys The system.
 * @param food_set Initial food components.
 * @param food_count Size of food set.
 * @param depths Output: depth for each component (-1 if unreachable).
 */
void oc_dependency_depths(const oc_system_t *sys,
                           const int *food_set, int food_count,
                           int *depths);

/**
 * @brief Check if the boundary is produced by the organizational closure.
 *
 * This is a key autopoietic criterion: the boundary itself must be
 * a product of the organization it encloses.
 *
 * @param sys The system.
 * @return 1 if boundary is self-produced, 0 otherwise.
 */
int oc_boundary_is_self_produced(const oc_system_t *sys);

/**
 * @brief Identify components that are both produced AND consumed
 * within the organizational closure (autopoietic core).
 *
 * @param sys The system.
 * @param core Output: core component indices.
 * @param core_size Output: number of core components.
 */
void oc_identify_autopoietic_core(const oc_system_t *sys,
                                   int *core, int *core_size);

/**
 * @brief Print a human-readable analysis of organizational closure.
 */
void oc_system_print_analysis(const oc_system_t *sys);

#endif /* ORGANIZATIONAL_CLOSURE_H */
