/**
 * @file structural_coupling.h
 * @brief Structural coupling dynamics between autopoietic systems and their environment.
 *
 * Structural coupling is the history of recurrent interactions between a system
 * and its medium/environment that trigger structural changes while preserving
 * organization. This is the mechanism by which autopoietic systems adapt.
 *
 * Knowledge coverage:
 *   L2 - Core concepts: structural determinism, structural drift, niche construction
 *   L4 - Fundamental laws: structural coupling dynamics, perturbation-response
 *   L7 - Applications: social autopoiesis, ecological coupling
 *   L8 - Advanced: second-order coupling, enactive cognition
 *
 * Reference: Maturana & Varela, "The Tree of Knowledge" (1987)
 *            Maturana, "Biology of Cognition" (1970)
 *            Di Paolo et al., "Lived Body and Structural Coupling" (2022)
 */

#ifndef STRUCTURAL_COUPLING_H
#define STRUCTURAL_COUPLING_H

#include <stddef.h>

#define SC_MAX_STATES 256
#define SC_MAX_HISTORY 1024

/**
 * @brief A perturbation event: an environmental change that affects the system.
 */
typedef struct {
    double time;
    int component_id;          /**< Component affected (-1 for boundary) */
    double magnitude;          /**< Perturbation magnitude */
    double duration;           /**< How long the perturbation lasts */
    int type;                  /**< Perturbation type */
} sc_perturbation_t;

#define SC_PERTURB_CHEMICAL   0   /**< Chemical concentration change */
#define SC_PERTURB_THERMAL    1   /**< Temperature change */
#define SC_PERTURB_MECHANICAL 2   /**< Physical/mechanical stress */
#define SC_PERTURB_INFORMATIONAL 3 /**< Signal/message from another system */

/**
 * @brief The response of the system to a perturbation.
 */
typedef struct {
    double time;
    double response_magnitude;   /**< Magnitude of system's change */
    double recovery_time;        /**< Time to return to pre-perturbation state */
    double structural_change;    /**< Degree of structural modification */
    int state_transition;        /**< 1 if system state changed */
} sc_response_t;

/**
 * @brief State of a structurally coupled system.
 */
typedef struct {
    double *concentrations;      /**< Component concentrations */
    int n_components;
    double *boundary_state;      /**< Boundary properties */
    int n_boundary_params;
    double organization_measure; /**< Current organizational invariance */
} sc_system_state_t;

/**
 * @brief The full structural coupling simulation.
 */
typedef struct {
    /* System definition */
    sc_system_state_t current_state;
    sc_system_state_t reference_state; /**< The "ideal" autopoietic state */

    /* Environmental parameters */
    double env_temperature;
    double env_resource_levels[16];
    int env_resource_count;
    double env_pH;
    double env_pressure;

    /* Coupling history */
    sc_perturbation_t perturbation_history[SC_MAX_HISTORY];
    int perturbation_count;

    sc_response_t response_history[SC_MAX_HISTORY];
    int response_count;

    /* Coupling metrics */
    double coupling_strength;     /**< Current coupling strength [0, 1] */
    double adaptation_rate;       /**< Rate of adaptation to environment */
    double structural_drift;      /**< Accumulated structural change */

    /* Simulation time */
    double time;
    double dt;
} sc_coupling_t;

/* ==========================================================================
 * System Setup
 * ========================================================================== */

void sc_coupling_init(sc_coupling_t *sc, int n_components, int n_boundary_params);
void sc_coupling_set_reference(sc_coupling_t *sc, const double *concentrations,
                                const double *boundary);
void sc_coupling_set_environment(sc_coupling_t *sc, double temp, double pH,
                                  const double *resources, int n_resources);
void sc_coupling_destroy(sc_coupling_t *sc);

/* ==========================================================================
 * Perturbation and Response — Structural Determinism Theorem
 * ========================================================================== */

/**
 * @brief Apply an environmental perturbation.
 *
 * According to Maturana & Varela's structural determinism, the system's
 * response is determined by its current structure, not by the perturbation
 * itself. The perturbation only triggers a structural change that is
 * already possible within the system's organization.
 *
 * @param sc Coupling system.
 * @param pert The perturbation to apply.
 * @param response Output: the system's structural response.
 */
void sc_apply_perturbation(sc_coupling_t *sc, const sc_perturbation_t *pert,
                            sc_response_t *response);

/**
 * @brief Compute the structural determinism index.
 *
 * Measures how much the system's response is determined by its own
 * structure rather than by the perturbation characteristics.
 * High SDI = system's own structure dominates response.
 *
 * @param sc Coupling system.
 * @param pert The perturbation.
 * @return SDI in [0, 1].
 */
double sc_structural_determinism_index(const sc_coupling_t *sc,
                                        const sc_perturbation_t *pert);

/* ==========================================================================
 * Structural Drift
 * ========================================================================== */

/**
 * @brief Evolve structural coupling over time.
 *
 * Simulates the continuous structural drift that results from
 * recurrent perturbations and the system's structurally determined
 * responses. Organization remains invariant while structure changes.
 *
 * @param sc Coupling system.
 * @param dt Time step.
 */
void sc_evolve_coupling(sc_coupling_t *sc, double dt);

/**
 * @brief Compute the structural drift magnitude.
 *
 * The accumulated difference between current structure and
 * reference structure, quantifying how far the system has drifted
 * while maintaining its organization.
 *
 * @param sc Coupling system.
 * @return Drift magnitude.
 */
double sc_compute_structural_drift(const sc_coupling_t *sc);

/**
 * @brief Check if organization is preserved despite structural changes.
 *
 * @param sc Coupling system.
 * @param tolerance Maximum allowed deviation from reference organization.
 * @return 1 if organization preserved, 0 otherwise.
 */
int sc_organization_preserved(const sc_coupling_t *sc, double tolerance);

/* ==========================================================================
 * Niche Construction
 * ========================================================================== */

/**
 * @brief Simulate niche construction: the system modifies its environment.
 *
 * Autopoietic systems not only respond to their environment but also
 * modify it, creating conditions favorable to their continued existence.
 * This is a form of second-order structural coupling.
 *
 * @param sc Coupling system.
 * @param resource_id Which resource to modify.
 * @param modification_amount Amount to modify by.
 */
void sc_niche_construction(sc_coupling_t *sc, int resource_id,
                             double modification_amount);

/**
 * @brief Compute the niche construction index.
 *
 * Measures how much the system has modified its environment
 * relative to initial conditions.
 *
 * @param sc Coupling system.
 * @return Niche construction index [0, 1].
 */
double sc_niche_construction_index(const sc_coupling_t *sc);

/* ==========================================================================
 * Social Coupling (Luhmann's Social Autopoiesis)
 * ========================================================================== */

/**
 * @brief Measure the coupling between two autopoietic systems.
 *
 * Two structurally coupled systems trigger structural changes in
 * each other, forming a consensual domain of interactions.
 *
 * @param sc1 First system.
 * @param sc2 Second system.
 * @return Cross-coupling measure.
 */
double sc_cross_coupling_measure(const sc_coupling_t *sc1,
                                  const sc_coupling_t *sc2);

/**
 * @brief Simulate a consensual interaction between two coupled systems.
 *
 * In Luhmann's social autopoiesis, communication events are the
 * components of social autopoietic systems.
 *
 * @param sc1 First system (perturber).
 * @param sc2 Second system (perturbed).
 * @param signal_magnitude Strength of communication.
 * @return Response of sc2 to the communication.
 */
sc_response_t sc_consensual_interaction(sc_coupling_t *sc1,
                                         sc_coupling_t *sc2,
                                         double signal_magnitude);

/**
 * @brief Print the coupling state report.
 */
void sc_coupling_print_report(const sc_coupling_t *sc);

#endif /* STRUCTURAL_COUPLING_H */
