#ifndef CAS_EVOLUTION_H
#define CAS_EVOLUTION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "cas_agent.h"
#include "cas_landscape.h"

/* =========================================================================
 * Evolutionary Computation in CAS (L4-L6)
 *
 * Evolution is the fundamental creative force in CAS. This module
 * implements the algorithmic machinery of variation, selection, and
 * heredity that drives adaptation across biological, social, and
 * computational complex systems.
 *
 * L4: Fisher's Fundamental Theorem, Price Equation, Schema Theorem (Holland)
 * L5: Genetic Algorithm, Evolution Strategies, Genetic Programming
 * L6: NK landscape optimization, El Farol problem, Minority Game
 *
 * Reference:
 *   Mitchell, M. (1998). An Introduction to Genetic Algorithms. MIT Press.
 *   Holland, J.H. (1975). Adaptation in Natural and Artificial Systems.
 *   Goldberg, D.E. (1989). Genetic Algorithms. Addison-Wesley.
 * ========================================================================= */

#define CAS_GA_POP_SIZE       100
#define CAS_GA_GENOME_BITS    128
#define CAS_GA_GENOME_BYTES   (CAS_GA_GENOME_BITS / 8)
#define CAS_GA_MAX_GENERATIONS 10000
#define CAS_GA_ELITE_SIZE     4

/* L3: Individual genome */
typedef struct {
    uint8_t  genes[CAS_GA_GENOME_BYTES];
    double   fitness;
    double   scaled_fitness;
    double   cumulative_fitness;
    uint32_t parent_a;
    uint32_t parent_b;
    uint32_t birth_generation;
    bool     is_elite;
} cas_genome_t;

/* L3: Genetic Algorithm state */
typedef struct {
    cas_genome_t population[CAS_GA_POP_SIZE];
    uint32_t     pop_size;
    uint32_t     genome_bits;
    uint32_t     generation;
    double       crossover_rate;
    double       mutation_rate;
    uint32_t     tournament_size;
    double       (*fitness_func)(const uint8_t *, uint32_t, void *);
    void        *fitness_data;
    double       best_fitness;
    double       avg_fitness;
    double       worst_fitness;
    double       fitness_variance;
    uint32_t     best_index;
    uint32_t     evaluations;
} cas_genetic_algorithm_t;

/* L3: Price equation components */
typedef struct {
    double   delta_z_bar;
    double   covariance_wz;
    double   expected_delta_z;
    double   selection_differential;
    double   heritability;
    double   selection_response;
} cas_price_eq_t;

/* L3: Schema (Holland) -- template in {0,1,*} */
typedef struct {
    uint8_t  pattern[CAS_GA_GENOME_BYTES];
    uint8_t  mask[CAS_GA_GENOME_BYTES];
    uint32_t order;
    uint32_t defining_length;
    double   observed_fitness;
    double   expected_fitness;
    uint32_t count;
} cas_schema_t;

/* ===== L5: Genetic Algorithm API ===== */
void cas_ga_init(cas_genetic_algorithm_t *ga, uint32_t pop_size,
                 uint32_t genome_bits,
                 double (*fitness)(const uint8_t *, uint32_t, void *),
                 void *fitness_data, uint64_t seed);
void cas_ga_evaluate(cas_genetic_algorithm_t *ga);
void cas_ga_select(cas_genetic_algorithm_t *ga, uint64_t *seed);
void cas_ga_crossover(cas_genetic_algorithm_t *ga,
                      const cas_genome_t *parent1,
                      const cas_genome_t *parent2,
                      cas_genome_t *child1, cas_genome_t *child2,
                      uint64_t *seed);
void cas_ga_mutate(cas_genetic_algorithm_t *ga, cas_genome_t *genome,
                   uint64_t *seed);
void cas_ga_generation(cas_genetic_algorithm_t *ga, uint64_t *seed);
void cas_ga_run(cas_genetic_algorithm_t *ga, uint32_t max_generations,
                double target_fitness, uint64_t *seed);
void cas_ga_get_best(const cas_genetic_algorithm_t *ga,
                     uint8_t *best_genome, double *best_fitness);
void cas_ga_roulette_wheel(cas_genetic_algorithm_t *ga);
void cas_ga_stochastic_universal(cas_genetic_algorithm_t *ga, uint64_t *seed);

/* ===== L5: Single-point, Two-point, Uniform Crossover ===== */
void cas_ga_crossover_1point(const cas_genetic_algorithm_t *ga,
                             const cas_genome_t *p1, const cas_genome_t *p2,
                             cas_genome_t *c1, cas_genome_t *c2,
                             uint64_t *seed);
void cas_ga_crossover_2point(const cas_genetic_algorithm_t *ga,
                             const cas_genome_t *p1, const cas_genome_t *p2,
                             cas_genome_t *c1, cas_genome_t *c2,
                             uint64_t *seed);
void cas_ga_crossover_uniform(const cas_genetic_algorithm_t *ga,
                              const cas_genome_t *p1, const cas_genome_t *p2,
                              cas_genome_t *c1, cas_genome_t *c2,
                              uint64_t *seed);

/* ===== L4: Price Equation (evolutionary change decomposition) ===== */
void cas_price_eq_compute(cas_price_eq_t *pe,
                          const double *trait_parent,
                          const double *trait_offspring,
                          const double *fitness, uint32_t n);
double cas_price_eq_fundamental_theorem(const cas_price_eq_t *pe);

/* ===== L4: Schema Theorem (Holland 1975) ===== */
void cas_schema_init(cas_schema_t *schema,
                     const uint8_t *pattern, const uint8_t *mask,
                     uint32_t genome_bits);
double cas_schema_lower_bound(const cas_schema_t *schema,
                              const cas_genetic_algorithm_t *ga);
void cas_schema_sample(cas_schema_t *schema,
                       const cas_genetic_algorithm_t *ga);

/* ===== L5: Evolution Strategies (Rechenberg/Schwefel) ===== */
typedef struct {
    double   x[64];
    double   sigma[64];
    uint32_t dim;
    double   fitness;
} cas_es_individual_t;

typedef struct {
    cas_es_individual_t population[CAS_GA_POP_SIZE];
    uint32_t pop_size;
    uint32_t mu;
    uint32_t lambda;
    uint32_t dim;
    uint32_t generation;
    double   (*obj_func)(const double *, uint32_t, void *);
    void    *obj_data;
    double   best_fitness;
} cas_evolution_strategy_t;

void cas_es_init(cas_evolution_strategy_t *es, uint32_t mu, uint32_t lambda,
                 uint32_t dim,
                 double (*obj)(const double *, uint32_t, void *),
                 void *data, uint64_t seed);
void cas_es_step(cas_evolution_strategy_t *es, uint64_t *seed);
void cas_es_run(cas_evolution_strategy_t *es, uint32_t max_gen,
                double target, uint64_t *seed);

/* ===== L6: Minority Game (Challet & Zhang 1997) ===== */
#define CAS_MG_MAX_AGENTS  256
#define CAS_MG_MAX_M       16

typedef struct {
    uint32_t strategy[CAS_MG_MAX_M * 2];
    double   score;
} cas_mg_strategy_t;

typedef struct {
    uint32_t n_agents;
    uint32_t m;
    uint32_t s;
    uint32_t history;
    uint32_t minority_side;
    cas_mg_strategy_t strategies[CAS_MG_MAX_AGENTS * 2];
    int32_t  attendance_history[100];
    uint32_t hist_len;
    double   volatility;
    double   predictability;
} cas_minority_game_t;

void cas_mg_init(cas_minority_game_t *mg, uint32_t n, uint32_t m,
                 uint32_t s, uint64_t seed);
void cas_mg_step(cas_minority_game_t *mg, uint64_t *seed);
void cas_mg_run(cas_minority_game_t *mg, uint32_t steps, uint64_t *seed);
double cas_mg_volatility(const cas_minority_game_t *mg);

#endif /* CAS_EVOLUTION_H */
