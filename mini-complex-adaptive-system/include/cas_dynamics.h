#ifndef CAS_DYNAMICS_H
#define CAS_DYNAMICS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "cas_agent.h"
#include "cas_landscape.h"
#include "cas_network.h"

/* =========================================================================
 * CAS Dynamics and Evolution (L2-L4)
 *
 * The collective dynamics of interacting agents produce emergent
 * system-level behaviors: phase transitions, self-organized criticality,
 * co-evolutionary arms races, and information cascades.
 *
 * L2: Population dynamics, co-evolution, emergence, phase transition
 * L3: Master equation, mean-field theory, Langevin dynamics, Markov chains
 * L4: Fisher's fundamental theorem, Price equation, Bak-Tang-Wiesenfeld SOC
 *
 * Reference:
 *   Bak, P. (1996). How Nature Works. Copernicus.
 *   Nowak, M.A. (2006). Evolutionary Dynamics. Harvard.
 *   Helbing, D. (2010). Quantitative Sociodynamics. Springer.
 * ========================================================================= */

#define CAS_POP_MAX_SIZE      100
#define CAS_POP_MAX_STEPS     10000
#define CAS_POP_HISTORY_MAX   1000

/* L1: Population state */
typedef struct {
    cas_agent_t agents[CAS_POP_MAX_SIZE];
    uint32_t    size;
    uint32_t    step;
    double      mean_fitness;
    double      fitness_variance;
    double      diversity_index;
    double      entropy;
    double      cooperation_ratio;
    double      energy_total;
    uint32_t    births;
    uint32_t    deaths;
    cas_network_t network;
} cas_population_t;

/* L3: Master equation state (probability distribution over states) */
typedef struct {
    double   prob[CAS_POP_MAX_SIZE];
    uint32_t num_states;
    double   transition_matrix[CAS_POP_MAX_SIZE * CAS_POP_MAX_SIZE / 8];
} cas_master_eq_t;

/* L3: Mean-field approximation state */
typedef struct {
    double   order_parameter;
    double   susceptibility;
    double   correlation_length;
    double   critical_point;
    bool     is_ordered_phase;
} cas_mean_field_t;

/* L3: Langevin dynamics state (stochastic differential equation) */
typedef struct {
    double   position;
    double   velocity;
    double   noise_intensity;
    double   friction;
    double   temperature;
    double   potential_energy;
} cas_langevin_t;

/* L2: Co-evolution pair state */
typedef struct {
    uint32_t agent_a;
    uint32_t agent_b;
    double   fitness_a;
    double   fitness_b;
    double   interaction_history[100];
    uint32_t history_len;
    bool     is_mutualistic;
    bool     is_competitive;
    bool     is_predator_prey;
} cas_coevolution_t;

/* L2: Emergence metrics */
typedef struct {
    double   nonlinearity_index;
    double   self_organization_index;
    double   complexity_metric;
    double   emergence_measure;
    double   mutual_information;
    double   integrated_information;
    double   causal_density;
    uint32_t emergent_levels;
} cas_emergence_metrics_t;

/* ===== L5: Population Dynamics API ===== */
void cas_pop_init(cas_population_t *pop, uint32_t size,
                  cas_agent_type_t default_type, uint32_t state_dim,
                  uint64_t seed);
void cas_pop_step(cas_population_t *pop, double *environment,
                  uint32_t env_dim, uint64_t *seed);
void cas_pop_run(cas_population_t *pop, double *environment,
                 uint32_t env_dim, uint32_t n_steps, uint64_t *seed);
void cas_pop_selection(cas_population_t *pop, uint64_t *seed);
void cas_pop_reproduction(cas_population_t *pop, uint64_t *seed);
void cas_pop_mutation(cas_population_t *pop, double rate, uint64_t *seed);
void cas_pop_compute_stats(cas_population_t *pop);

/* ===== L5: Co-evolution API ===== */
void cas_coevolve_init(cas_coevolution_t *ce, uint32_t a, uint32_t b);
void cas_coevolve_update(cas_coevolution_t *ce, double payoff_a, double payoff_b);
int cas_coevolve_classify(cas_coevolution_t *ce);
double cas_coevolve_red_queen_effect(const cas_coevolution_t *ce);

/* ===== L5: Master Equation API ===== */
void cas_master_eq_init(cas_master_eq_t *me, uint32_t num_states,
                        const double *initial_prob);
void cas_master_eq_step(cas_master_eq_t *me, double dt);
void cas_master_eq_steady_state(cas_master_eq_t *me, double tol,
                                uint32_t max_iter);
double cas_master_eq_entropy(const cas_master_eq_t *me);

/* ===== L5: Mean-Field API ===== */
void cas_mean_field_compute(cas_mean_field_t *mf,
                            const double *observations, uint32_t n,
                            double control_param);
bool cas_mean_field_is_critical(const cas_mean_field_t *mf);

/* ===== L5: Langevin Dynamics API ===== */
void cas_langevin_init(cas_langevin_t *l, double temp, double friction,
                       double noise);
void cas_langevin_step(cas_langevin_t *l, double dt, uint64_t *seed);
double cas_langevin_escape_rate(const cas_langevin_t *l, double barrier_height);

/* ===== L5: Emergence Detection API ===== */
void cas_emergence_detect(const cas_population_t *pop,
                          cas_emergence_metrics_t *metrics);
double cas_mutual_information(const double *x, const double *y, uint32_t n,
                              uint32_t bins);
double cas_integrated_information(const cas_population_t *pop);
double cas_causal_density(const cas_network_t *net);

/* ===== L4: Bak-Tang-Wiesenfeld Self-Organized Criticality ===== */
#define CAS_SANDPILE_SIZE 64

typedef struct {
    uint32_t grid[CAS_SANDPILE_SIZE][CAS_SANDPILE_SIZE];
    uint32_t size;
    uint64_t total_avalanches;
    uint64_t total_topplings;
    double   avalanche_size_dist[100];
    uint32_t dist_len;
} cas_sandpile_t;

void cas_sandpile_init(cas_sandpile_t *sp, uint32_t size);
void cas_sandpile_drop(cas_sandpile_t *sp, uint64_t *seed);
void cas_sandpile_topple(cas_sandpile_t *sp, uint32_t x, uint32_t y);
void cas_sandpile_run(cas_sandpile_t *sp, uint64_t n_drops, uint64_t *seed);
double cas_sandpile_powerlaw_exponent(const cas_sandpile_t *sp);

#endif /* CAS_DYNAMICS_H */
