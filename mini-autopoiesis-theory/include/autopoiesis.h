/**
 * @file autopoiesis.h
 * @brief Core autopoiesis theory definitions and system representation.
 *
 * Defines the central concept of autopoiesis as introduced by Maturana & Varela
 * (1973, 1980): a system that continuously produces and maintains itself.
 *
 * Knowledge coverage:
 *   L1 - Core definitions: AutopoieticSystem, Component, Boundary
 *   L2 - Core concepts: self-production, operational closure, structural determinism
 *   L3 - Mathematical structures: component set, production network, boundary topology
 *
 * Reference: Maturana & Varela, "Autopoiesis and Cognition" (1980)
 *            Maturana & Varela, "The Tree of Knowledge" (1987)
 */

#ifndef AUTOPOIESIS_H
#define AUTOPOIESIS_H

#include <stddef.h>
#include <stdint.h>

/* ==========================================================================
 * L1: Core Type Definitions
 * ========================================================================== */

/** Maximum number of components in an autopoietic system */
#define AP_MAX_COMPONENTS 512
/** Maximum number of reactions in an autopoietic system */
#define AP_MAX_REACTIONS 1024
/** Maximum number of neighbors in boundary adjacency */
#define AP_MAX_NEIGHBORS 32
/** Maximum length of a component label */
#define AP_MAX_LABEL 64

/**
 * @brief Role of a component within an autopoietic system.
 *
 * Maturana & Varela distinguish between components that form the boundary
 * and components that form the internal production network.
 */
typedef enum {
    AP_COMPONENT_INTERNAL,   /**< Internal component: part of production network */
    AP_COMPONENT_BOUNDARY,   /**< Boundary component: forms system boundary */
    AP_COMPONENT_CATALYST,   /**< Catalytic component: facilitates reactions */
    AP_COMPONENT_SUBSTRATE,  /**< Substrate component: consumed in reactions */
    AP_COMPONENT_PRODUCT     /**< Product component: produced by reactions */
} ap_component_role_t;

/**
 * @brief State of an autopoietic system as a whole.
 *
 * An autopoietic system can be in various states of self-maintenance.
 * The key criterion is whether the system produces its own components
 * and maintains its organization.
 */
typedef enum {
    AP_STATE_INACTIVE,        /**< System not active */
    AP_STATE_SELF_PRODUCING,  /**< System actively producing its components */
    AP_STATE_MAINTAINING,     /**< System in steady-state self-maintenance */
    AP_STATE_DEGRADING,       /**< System losing components */
    AP_STATE_ADAPTING,        /**< System undergoing structural change while maintaining organization */
    AP_STATE_DISINTEGRATING   /**< System losing organizational identity */
} ap_system_state_t;

/**
 * @brief A single component within an autopoietic system.
 *
 * Each component has a type, concentration, and role. The component is
 * the basic unit of the autopoietic network.
 */
typedef struct {
    char label[AP_MAX_LABEL];         /**< Component identifier */
    ap_component_role_t role;         /**< Role in the system */
    double concentration;             /**< Current concentration (arbitrary units) */
    double decay_rate;                /**< Natural decay/degradation rate */
    uint32_t flags;                   /**< Bit flags for component properties */
    int production_count;             /**< Number of reactions producing this component */
    int consumption_count;            /**< Number of reactions consuming this component */
} ap_component_t;

/** Flags for ap_component_t.flags */
#define AP_FLAG_ESSENTIAL     0x01    /**< Component essential for autopoiesis */
#define AP_FLAG_SELF_PRODUCED 0x02    /**< Component produced within the system */
#define AP_FLAG_IMPORTED      0x04    /**< Component imported from environment */
#define AP_FLAG_BOUNDARY      0x08    /**< Component part of the boundary */
#define AP_FLAG_CATALYTIC     0x10    /**< Component has catalytic function */

/**
 * @brief A production relation: specifies how one component produces another.
 *
 * In Maturana & Varela's framework, the production relation is the basic
 * unit of the autopoietic network. The network of productions constitutes
 * the organization of the system.
 */
typedef struct {
    int producer_id;          /**< Index of producing component (or reaction) */
    int product_id;           /**< Index of produced component */
    double rate;              /**< Production rate constant */
    int catalyst_id;          /**< Index of catalyst (-1 if none) */
    int substrate_ids[8];     /**< Indices of required substrates */
    int substrate_count;      /**< Number of required substrates */
} ap_production_t;

/**
 * @brief The boundary of an autopoietic system.
 *
 * The boundary is what defines the system as a unity distinguishable from
 * its environment. In autopoietic systems, the boundary itself is produced
 * by the system's components.
 */
typedef struct {
    int component_ids[AP_MAX_NEIGHBORS]; /**< Components forming the boundary */
    int component_count;                 /**< Number of boundary components */
    double permeability;                 /**< Boundary permeability [0,1] */
    double surface_area;                 /**< Effective surface area */
    double thickness;                    /**< Boundary thickness */
    int topology[AP_MAX_NEIGHBORS][AP_MAX_NEIGHBORS]; /**< Adjacency matrix */
    int topology_size;                   /**< Size of topology matrix */
} ap_boundary_t;

/**
 * @brief The complete autopoietic system.
 *
 * This is the central data structure representing an autopoietic unity
 * as defined by Maturana & Varela.
 */
typedef struct {
    /* Component inventory */
    ap_component_t components[AP_MAX_COMPONENTS];
    int component_count;

    /* Production network */
    ap_production_t productions[AP_MAX_REACTIONS];
    int production_count;

    /* The boundary that defines the system as a unity */
    ap_boundary_t boundary;

    /* System state */
    ap_system_state_t state;
    double total_mass;
    double entropy;
    double organization_measure;  /**< Quantification of organizational invariance */

    /* Interaction history for structural coupling */
    double time;
    int perturbation_count;
    int adaptation_count;
} ap_system_t;

/* ==========================================================================
 * L2: Core Autopoiesis Functions
 * ========================================================================== */

/**
 * @brief Initialize an empty autopoietic system.
 * @param sys Pointer to system to initialize.
 */
void ap_system_init(ap_system_t *sys);

/**
 * @brief Add a component to the autopoietic system.
 * @param sys Target system.
 * @param label Component name/identifier.
 * @param role The component's role.
 * @param concentration Initial concentration.
 * @param decay_rate Natural decay rate.
 * @return Component index on success, -1 on failure.
 */
int ap_add_component(ap_system_t *sys, const char *label,
                     ap_component_role_t role,
                     double concentration, double decay_rate);

/**
 * @brief Add a production relation to the system.
 * @param sys Target system.
 * @param producer_id Index of the producer.
 * @param product_id Index of the product.
 * @param rate Production rate constant.
 * @param catalyst_id Index of catalyst (-1 if uncatalyzed).
 * @param substrate_ids Array of required substrate indices.
 * @param substrate_count Number of substrates.
 * @return Production index on success, -1 on failure.
 */
int ap_add_production(ap_system_t *sys, int producer_id, int product_id,
                      double rate, int catalyst_id,
                      const int *substrate_ids, int substrate_count);

/**
 * @brief Check if the system satisfies the autopoiesis criteria.
 *
 * A system is autopoietic if:
 *   1. All components are produced by the network of productions (operational closure)
 *   2. The boundary is produced by the system's components
 *   3. The organization remains invariant through structural changes
 *
 * @param sys The system to check.
 * @return 1 if autopoietic, 0 otherwise.
 */
int ap_check_autopoiesis(const ap_system_t *sys);

/**
 * @brief Compute the organizational closure measure.
 *
 * Organizational closure quantifies the degree to which all system components
 * are produced within the system. A value of 1.0 indicates complete closure.
 *
 * @param sys The system to analyze.
 * @return Closure measure in [0, 1].
 */
double ap_compute_organizational_closure(const ap_system_t *sys);

/**
 * @brief Evolve the system state over one time step.
 *
 * Propagates concentrations according to production and decay rates,
 * updates system state, and records structural changes.
 *
 * @param sys The system to evolve.
 * @param dt Time step size.
 */
void ap_evolve_step(ap_system_t *sys, double dt);

/**
 * @brief Compute the system's current state classification.
 * @param sys The system.
 * @return Current system state.
 */
ap_system_state_t ap_classify_state(const ap_system_t *sys);

/**
 * @brief Apply an environmental perturbation to the system.
 *
 * This is central to structural coupling: the system responds to
 * perturbations while maintaining its organization.
 *
 * @param sys Target system.
 * @param component_id Index of perturbed component.
 * @param delta Change in concentration.
 * @return New system state after perturbation.
 */
ap_system_state_t ap_apply_perturbation(ap_system_t *sys, int component_id,
                                         double delta);

/**
 * @brief Compute the degree of structural coupling between two autopoietic systems.
 *
 * Structural coupling is the history of recurrent interactions between
 * a system and its environment that trigger structural changes.
 *
 * @param sys1 First system.
 * @param sys2 Second system (or NULL for environment).
 * @return Coupling measure in [0, 1].
 */
double ap_structural_coupling_measure(const ap_system_t *sys1,
                                       const ap_system_t *sys2);

/**
 * @brief Free all resources associated with the system.
 * @param sys System to destroy.
 */
void ap_system_destroy(ap_system_t *sys);

/* ==========================================================================
 * L3: Mathematical Structure Functions
 * ========================================================================== */

/**
 * @brief Build the adjacency matrix of the production network.
 *
 * A[i][j] = 1 if component i produces component j, 0 otherwise.
 *
 * @param sys The system.
 * @param matrix Output matrix (caller-allocated, size n×n).
 * @param n Dimension (number of components).
 */
void ap_build_production_adjacency(const ap_system_t *sys,
                                    int *matrix, int n);

/**
 * @brief Compute the transitive closure of the production network.
 *
 * Determines all components that can eventually be produced
 * starting from an initial set.
 *
 * @param sys The system.
 * @param initial_set Indices of initially available components.
 * @param initial_count Size of initial set.
 * @param closure_set Output: all producible components.
 * @param closure_count Output: size of closure set.
 */
void ap_production_transitive_closure(const ap_system_t *sys,
                                       const int *initial_set,
                                       int initial_count,
                                       int *closure_set,
                                       int *closure_count);

/**
 * @brief Find all minimal generating sets for the system.
 *
 * A minimal generating set is a smallest set of components from which
 * all other components can be produced.
 *
 * @param sys The system.
 * @param generators Output array of component indices.
 * @param gen_count Output: number of components in minimal set.
 * @return 0 on success, -1 if no generating set exists.
 */
int ap_find_minimal_generators(const ap_system_t *sys,
                                int *generators, int *gen_count);

/**
 * @brief Verify that the boundary is self-produced.
 *
 * A boundary is self-produced if all boundary components can be
 * generated from internal components only.
 *
 * @param sys The system.
 * @return 1 if boundary is self-produced, 0 otherwise.
 */
int ap_verify_self_produced_boundary(const ap_system_t *sys);

/* ==========================================================================
 * Utility / introspection
 * ========================================================================== */

/** Print a human-readable summary of the system to stdout. */
void ap_system_print_summary(const ap_system_t *sys);

/** Get the number of essential components (flag AP_FLAG_ESSENTIAL). */
int ap_count_essential_components(const ap_system_t *sys);

/** Get the fraction of components that are self-produced. */
double ap_self_production_fraction(const ap_system_t *sys);

#endif /* AUTOPOIESIS_H */
