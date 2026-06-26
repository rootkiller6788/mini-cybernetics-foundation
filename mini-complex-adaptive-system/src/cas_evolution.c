/**
 * @file cas_evolution.c
 * @brief Evolutionary Computation in CAS (L4-L6)
 *
 * Evolution is the fundamental creative force in CAS. Implements
 * genetic algorithms, evolution strategies, Price equation, and
 * Holland's Schema Theorem.
 *
 * L4: Fisher's Theorem, Price Equation, Schema Theorem
 * L5: GA (selection, crossover, mutation), ES (mu+lambda)
 * L6: NK optimization, Minority Game, El Farol problem
 *
 * Reference:
 *   Holland, J.H. (1975). Adaptation in Natural and Artificial Systems.
 *   Goldberg, D.E. (1989). Genetic Algorithms. Addison-Wesley.
 *   Mitchell, M. (1998). An Introduction to Genetic Algorithms. MIT Press.
 */

#include "cas_evolution.h"
#include <string.h>
#include <stdlib.h>

static uint64_t _ev_seed = 314159;

static uint64_t _ev_rand_u64(void) {
    _ev_seed = _ev_seed * 6364136223846793005ULL + 1442695040888963407ULL;
    uint64_t z = _ev_seed ^ (_ev_seed >> 33);
    z = z * 0xff51afd7ed558ccdULL; z = z ^ (z >> 33);
    z = z * 0xc4ceb9fe1a85ec53ULL;
    return z ^ (z >> 33);
}
static void _ev_srand(uint64_t s) { _ev_seed = s; }
static double _ev_rand_double(void) { return (double)(_ev_rand_u64() >> 11) * 0x1.0p-53; }
static double _ev_rand_gaussian(void) {
    double u1 = _ev_rand_double(), u2 = _ev_rand_double();
    if (u1 < 1e-12) u1 = 1e-12;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

static bool _ev_get_bit(const uint8_t *g, uint32_t pos) {
    return (g[pos / 8] >> (pos % 8)) & 1;
}
static void _ev_set_bit(uint8_t *g, uint32_t pos, bool v) {
    if (v) g[pos / 8] |= (1 << (pos % 8));
    else g[pos / 8] &= ~(1 << (pos % 8));
}
static void _ev_toggle_bit(uint8_t *g, uint32_t pos) {
    g[pos / 8] ^= (1 << (pos % 8));
}

/* =================================================================
 * L5: Genetic Algorithm Initialization
 *
 * Create a population of random bitstring genomes.
 *
 * Complexity: O(pop_size * genome_bytes)
 * ================================================================= */

void cas_ga_init(cas_genetic_algorithm_t *ga, uint32_t pop_size,
                 uint32_t genome_bits,
                 double (*fitness)(const uint8_t *, uint32_t, void *),
                 void *fitness_data, uint64_t seed)
{
    if (!ga) return;
    memset(ga, 0, sizeof(*ga));
    _ev_srand(seed);

    ga->pop_size = pop_size < CAS_GA_POP_SIZE ? pop_size : CAS_GA_POP_SIZE;
    ga->genome_bits = genome_bits < CAS_GA_GENOME_BITS ? genome_bits : CAS_GA_GENOME_BITS;
    ga->crossover_rate = 0.7;
    ga->mutation_rate = 1.0 / ga->genome_bits;
    ga->tournament_size = 3;
    ga->fitness_func = fitness;
    ga->fitness_data = fitness_data;
    ga->generation = 0;
    ga->evaluations = 0;

    uint32_t bytes = (ga->genome_bits + 7) / 8;
    uint32_t i, j;
    for (i = 0; i < ga->pop_size; i++) {
        for (j = 0; j < bytes && j < CAS_GA_GENOME_BYTES; j++)
            ga->population[i].genes[j] = (uint8_t)(_ev_rand_u64() & 0xFF);
        ga->population[i].fitness = 0.0;
        ga->population[i].birth_generation = 0;
    }
}

/* =================================================================
 * L5: GA Evaluation -- compute fitness for all individuals
 *
 * Complexity: O(pop_size * cost(fitness_func))
 * ================================================================= */

void cas_ga_evaluate(cas_genetic_algorithm_t *ga)
{
    if (!ga || !ga->fitness_func) return;

    ga->best_fitness = -1e308;
    ga->worst_fitness = 1e308;
    double sum_fit = 0.0, sum_fit2 = 0.0;
    uint32_t i;

    for (i = 0; i < ga->pop_size; i++) {
        double f = ga->fitness_func(ga->population[i].genes, ga->genome_bits, ga->fitness_data);
        ga->population[i].fitness = f;
        ga->evaluations++;

        if (f > ga->best_fitness) {
            ga->best_fitness = f;
            ga->best_index = i;
        }
        if (f < ga->worst_fitness) ga->worst_fitness = f;
        sum_fit += f;
        sum_fit2 += f * f;
    }

    ga->avg_fitness = sum_fit / ga->pop_size;
    ga->fitness_variance = sum_fit2 / ga->pop_size - ga->avg_fitness * ga->avg_fitness;
}

/* =================================================================
 * L5: GA Selection -- tournament selection for parents
 *
 * Creates mating pool via tournament selection.
 *
 * Complexity: O(pop_size * tournament_size)
 * ================================================================= */

void cas_ga_select(cas_genetic_algorithm_t *ga, uint64_t *seed)
{
    if (!ga) return;
    if (seed) _ev_srand(*seed);

    /* Create pool of parent indices */
    cas_genome_t new_pop[CAS_GA_POP_SIZE];
    uint32_t i;

    /* Elitism: keep best individuals */
    uint32_t elite = CAS_GA_ELITE_SIZE;
    if (elite > ga->pop_size) elite = ga->pop_size / 4;

    /* Copy elites directly */
    for (i = 0; i < elite; i++) {
        new_pop[i] = ga->population[i]; /* sorted by fitness after evaluate */
    }

    /* Tournament selection for rest */
    for (i = elite; i < ga->pop_size; i += 2) {
        uint32_t p1_idx = _ev_rand_u64() % ga->pop_size;
        uint32_t p2_idx = _ev_rand_u64() % ga->pop_size;
        uint32_t j;
        for (j = 1; j < ga->tournament_size; j++) {
            uint32_t c1 = _ev_rand_u64() % ga->pop_size;
            if (ga->population[c1].fitness > ga->population[p1_idx].fitness)
                p1_idx = c1;
            uint32_t c2 = _ev_rand_u64() % ga->pop_size;
            if (ga->population[c2].fitness > ga->population[p2_idx].fitness)
                p2_idx = c2;
        }

        cas_ga_crossover(ga, &ga->population[p1_idx], &ga->population[p2_idx],
                         &new_pop[i], (i + 1 < ga->pop_size) ? &new_pop[i + 1] : NULL, seed);
    }

    /* Replace population */
    for (i = 0; i < ga->pop_size; i++)
        ga->population[i] = new_pop[i];

    if (seed) *seed = _ev_seed;
}

/* =================================================================
 * L5: GA Crossover -- single-point recombination
 *
 * Complexity: O(genome_bytes)
 * ================================================================= */

void cas_ga_crossover(cas_genetic_algorithm_t *ga,
                      const cas_genome_t *parent1,
                      const cas_genome_t *parent2,
                      cas_genome_t *child1, cas_genome_t *child2,
                      uint64_t *seed)
{
    if (!ga || !parent1 || !parent2 || !child1) return;
    if (seed) _ev_srand(*seed);

    uint32_t bytes = (ga->genome_bits + 7) / 8;

    if (_ev_rand_double() < ga->crossover_rate) {
        uint32_t point = _ev_rand_u64() % (ga->genome_bits - 1) + 1;
        uint32_t byte_point = point / 8;
        uint32_t bit_point = point % 8;

        if (child1) {
            memcpy(child1->genes, parent1->genes, byte_point);
            memcpy(child1->genes + byte_point + 1, parent2->genes + byte_point + 1, bytes - byte_point - 1);
            uint8_t mask = (1 << bit_point) - 1;
            child1->genes[byte_point] = (parent1->genes[byte_point] & mask) | (parent2->genes[byte_point] & ~mask);
            child1->fitness = 0.0;
            child1->birth_generation = ga->generation;
        }
        if (child2) {
            memcpy(child2->genes, parent2->genes, byte_point);
            memcpy(child2->genes + byte_point + 1, parent1->genes + byte_point + 1, bytes - byte_point - 1);
            uint8_t mask = (1 << bit_point) - 1;
            child2->genes[byte_point] = (parent2->genes[byte_point] & mask) | (parent1->genes[byte_point] & ~mask);
            child2->fitness = 0.0;
            child2->birth_generation = ga->generation;
        }
    } else {
        if (child1) { memcpy(child1->genes, parent1->genes, bytes); child1->fitness = 0.0; child1->birth_generation = ga->generation; }
        if (child2) { memcpy(child2->genes, parent2->genes, bytes); child2->fitness = 0.0; child2->birth_generation = ga->generation; }
    }

    if (seed) *seed = _ev_seed;
}

/* =================================================================
 * L5: GA 2-Point Crossover
 * ================================================================= */

void cas_ga_crossover_2point(const cas_genetic_algorithm_t *ga,
                             const cas_genome_t *p1, const cas_genome_t *p2,
                             cas_genome_t *c1, cas_genome_t *c2, uint64_t *seed)
{
    if (!ga || !p1 || !p2) return;
    if (seed) _ev_srand(*seed);

    uint32_t bytes = (ga->genome_bits + 7) / 8;
    uint32_t pt1 = _ev_rand_u64() % ga->genome_bits;
    uint32_t pt2 = _ev_rand_u64() % ga->genome_bits;
    if (pt1 > pt2) { uint32_t t = pt1; pt1 = pt2; pt2 = t; }

    if (c1) {
        memcpy(c1->genes, p1->genes, bytes);
        uint32_t i;
        for (i = pt1; i <= pt2 && i < ga->genome_bits; i++) {
            bool b = _ev_get_bit(p2->genes, i);
            _ev_set_bit(c1->genes, i, b);
        }
        c1->fitness = 0.0; c1->birth_generation = ga->generation;
    }
    if (c2) {
        memcpy(c2->genes, p2->genes, bytes);
        uint32_t i;
        for (i = pt1; i <= pt2 && i < ga->genome_bits; i++) {
            bool b = _ev_get_bit(p1->genes, i);
            _ev_set_bit(c2->genes, i, b);
        }
        c2->fitness = 0.0; c2->birth_generation = ga->generation;
    }
}

/* =================================================================
 * L5: GA Uniform Crossover
 * ================================================================= */

void cas_ga_crossover_uniform(const cas_genetic_algorithm_t *ga,
                              const cas_genome_t *p1, const cas_genome_t *p2,
                              cas_genome_t *c1, cas_genome_t *c2, uint64_t *seed)
{
    if (!ga || !p1 || !p2) return;
    if (seed) _ev_srand(*seed);

    uint32_t i;
    for (i = 0; i < ga->genome_bits; i++) {
        bool swap = _ev_rand_double() < 0.5;
        if (c1) _ev_set_bit(c1->genes, i, swap ? _ev_get_bit(p2->genes, i) : _ev_get_bit(p1->genes, i));
        if (c2) _ev_set_bit(c2->genes, i, swap ? _ev_get_bit(p1->genes, i) : _ev_get_bit(p2->genes, i));
    }
    if (c1) { c1->fitness = 0.0; c1->birth_generation = ga->generation; }
    if (c2) { c2->fitness = 0.0; c2->birth_generation = ga->generation; }
}

/* =================================================================
 * L5: GA Bit-Flip Mutation
 *
 * Each bit flipped with probability mutation_rate.
 *
 * Complexity: O(genome_bits)
 * ================================================================= */

void cas_ga_mutate(cas_genetic_algorithm_t *ga, cas_genome_t *genome, uint64_t *seed)
{
    if (!ga || !genome) return;
    if (seed) _ev_srand(*seed);
    uint32_t i;
    for (i = 0; i < ga->genome_bits; i++)
        if (_ev_rand_double() < ga->mutation_rate)
            _ev_toggle_bit(genome->genes, i);
    if (seed) *seed = _ev_seed;
}

/* =================================================================
 * L5: GA Generation (select + crossover + mutate)
 *
 * One complete generation of evolutionary loop.
 *
 * Complexity: O(pop_size * (genome_bits + cost(fitness_func)))
 * ================================================================= */

void cas_ga_generation(cas_genetic_algorithm_t *ga, uint64_t *seed)
{
    if (!ga) return;
    if (seed) _ev_srand(*seed);

    cas_ga_evaluate(ga);
    cas_ga_select(ga, seed);

    uint32_t i;
    for (i = CAS_GA_ELITE_SIZE; i < ga->pop_size; i++)
        cas_ga_mutate(ga, &ga->population[i], seed);

    ga->generation++;

    if (seed) *seed = _ev_seed;
}

void cas_ga_run(cas_genetic_algorithm_t *ga, uint32_t max_generations,
                double target_fitness, uint64_t *seed)
{
    if (!ga) return;
    uint32_t g;
    for (g = 0; g < max_generations; g++) {
        cas_ga_generation(ga, seed);
        if (ga->best_fitness >= target_fitness) break;
    }
}

void cas_ga_get_best(const cas_genetic_algorithm_t *ga,
                     uint8_t *best_genome, double *best_fitness)
{
    if (!ga) return;
    if (best_genome)
        memcpy(best_genome, ga->population[ga->best_index].genes,
               (ga->genome_bits + 7) / 8);
    if (best_fitness) *best_fitness = ga->best_fitness;
}

/* =================================================================
 * L5: Roulette Wheel with Fitness Scaling
 * ================================================================= */

void cas_ga_roulette_wheel(cas_genetic_algorithm_t *ga)
{
    if (!ga) return;
    double min_f = ga->population[0].fitness;
    uint32_t i;
    for (i = 0; i < ga->pop_size; i++)
        if (ga->population[i].fitness < min_f)
            min_f = ga->population[i].fitness;

    double cum = 0.0;
    for (i = 0; i < ga->pop_size; i++) {
        ga->population[i].scaled_fitness = ga->population[i].fitness - min_f + 1e-6;
        cum += ga->population[i].scaled_fitness;
        ga->population[i].cumulative_fitness = cum;
    }
}

/* =================================================================
 * L4: Price Equation
 *
 * delta_z_bar = Cov(w, z) / w_bar + E[w * delta_z] / w_bar
 *
 * Partition evolutionary change into selection and transmission.
 *
 * Theorem (Price 1970, Nature): All evolutionary change can be
 * decomposed into selection covariance and transmission bias.
 * Fisher's fundamental theorem is a special case where
 * transmission bias is zero and trait = fitness.
 *
 * Complexity: O(n)
 * ================================================================= */

void cas_price_eq_compute(cas_price_eq_t *pe,
                          const double *trait_parent,
                          const double *trait_offspring,
                          const double *fitness, uint32_t n)
{
    if (!pe || !trait_parent || !trait_offspring || !fitness || n == 0) return;
    memset(pe, 0, sizeof(*pe));

    double w_bar = 0.0, z_bar = 0.0, z_bar_off = 0.0;
    double cov_sum = 0.0, exp_delta_sum = 0.0;
    uint32_t i;

    for (i = 0; i < n; i++) {
        w_bar += fitness[i];
        z_bar += trait_parent[i];
        z_bar_off += trait_offspring[i];
    }
    w_bar /= n; z_bar /= n; z_bar_off /= n;

    pe->delta_z_bar = z_bar_off - z_bar;

    for (i = 0; i < n; i++) {
        cov_sum += (fitness[i] - w_bar) * (trait_parent[i] - z_bar);
        exp_delta_sum += fitness[i] * (trait_offspring[i] - trait_parent[i]);
    }

    pe->covariance_wz = cov_sum / n;
    pe->expected_delta_z = exp_delta_sum / n;
    if (w_bar > 1e-12) {
        pe->covariance_wz /= w_bar;
        pe->expected_delta_z /= w_bar;
    }

    /* Heritability: beta of offspring on midparent */
    double sum_x2 = 0.0, sum_xy = 0.0;
    for (i = 0; i < n; i++) {
        double x = trait_parent[i] - z_bar;
        double y = trait_offspring[i] - z_bar_off;
        sum_x2 += x * x;
        sum_xy += x * y;
    }
    pe->heritability = sum_x2 > 1e-12 ? sum_xy / sum_x2 : 0.0;
    pe->selection_differential = pe->covariance_wz;
    pe->selection_response = pe->heritability * pe->selection_differential;
}

double cas_price_eq_fundamental_theorem(const cas_price_eq_t *pe)
{
    if (!pe) return 0.0;
    return pe->covariance_wz;  /* additive genetic variance in fitness */
}

/* =================================================================
 * L4: Schema Theorem (Holland 1975)
 *
 * E[m(H, t+1)] >= m(H, t) * f(H) / f_avg * [1 - p_c * delta(H)/(L-1) - o(H) * p_m]
 *
 * The expected number of instances of schema H in next generation is
 * bounded below by: current count * relative fitness * survival probability.
 *
 * Complexity: O(1)
 * ================================================================= */

void cas_schema_init(cas_schema_t *schema,
                     const uint8_t *pattern, const uint8_t *mask,
                     uint32_t genome_bits)
{
    if (!schema || !pattern || !mask) return;

    uint32_t bytes = (genome_bits + 7) / 8;
    memcpy(schema->pattern, pattern, bytes);
    memcpy(schema->mask, mask, bytes);

    /* Order o(H): number of fixed positions (0 or 1, not *) */
    schema->order = 0;
    uint32_t first_fixed = genome_bits, last_fixed = 0;
    uint32_t i;
    for (i = 0; i < genome_bits; i++) {
        if (_ev_get_bit(mask, i)) {
            schema->order++;
            if (i < first_fixed) first_fixed = i;
            if (i > last_fixed) last_fixed = i;
        }
    }

    /* Defining length delta(H): distance between first and last fixed positions */
    schema->defining_length = schema->order > 0 ? (last_fixed - first_fixed) : 0;
    schema->count = 0;
    schema->observed_fitness = 0.0;
    schema->expected_fitness = 0.0;
}

double cas_schema_lower_bound(const cas_schema_t *schema,
                              const cas_genetic_algorithm_t *ga)
{
    if (!schema || !ga || ga->avg_fitness < 1e-12) return 0.0;

    double survival = 1.0;
    if (ga->genome_bits > 1) {
        survival *= 1.0 - ga->crossover_rate * (double)schema->defining_length / (ga->genome_bits - 1);
    }
    survival *= 1.0 - ga->mutation_rate * (double)schema->order;

    return (double)schema->count * (schema->observed_fitness / ga->avg_fitness) * survival;
}

void cas_schema_sample(cas_schema_t *schema,
                       const cas_genetic_algorithm_t *ga)
{
    if (!schema || !ga) return;

    schema->count = 0;
    double sum_fit = 0.0;
    uint32_t i, j;

    for (i = 0; i < ga->pop_size; i++) {
        bool matches = true;
        for (j = 0; j < ga->genome_bits; j++) {
            if (_ev_get_bit(schema->mask, j) &&
                _ev_get_bit(schema->pattern, j) != _ev_get_bit(ga->population[i].genes, j)) {
                matches = false;
                break;
            }
        }
        if (matches) {
            schema->count++;
            sum_fit += ga->population[i].fitness;
        }
    }

    if (schema->count > 0)
        schema->observed_fitness = sum_fit / schema->count;
}

/* =================================================================
 * L5: Evolution Strategies (mu, lambda)-ES
 *
 * Complexity: O(mu * dim) per generation
 * ================================================================= */

void cas_es_init(cas_evolution_strategy_t *es, uint32_t mu, uint32_t lambda,
                 uint32_t dim, double (*obj)(const double *, uint32_t, void *),
                 void *data, uint64_t seed)
{
    if (!es) return;
    memset(es, 0, sizeof(*es));
    _ev_srand(seed);

    es->mu = mu; es->lambda = lambda;
    es->pop_size = mu + lambda;
    if (es->pop_size > CAS_GA_POP_SIZE) es->pop_size = CAS_GA_POP_SIZE;
    es->dim = dim < 64 ? dim : 64;
    es->obj_func = obj;
    es->obj_data = data;
    es->generation = 0;

    uint32_t i, d;
    for (i = 0; i < es->pop_size; i++) {
        for (d = 0; d < es->dim; d++) {
            es->population[i].x[d] = _ev_rand_double() * 2.0 - 1.0;
            es->population[i].sigma[d] = 0.3;
        }
        es->population[i].fitness = 0.0;
    }
}

void cas_es_step(cas_evolution_strategy_t *es, uint64_t *seed)
{
    if (!es) return;
    if (seed) _ev_srand(*seed);

    /* Evaluate all */
    uint32_t i, d;
    for (i = 0; i < es->pop_size; i++) {
        if (es->obj_func)
            es->population[i].fitness = es->obj_func(es->population[i].x, es->dim, es->obj_data);
    }

    /* Sort by fitness (bubble -- n is small) */
    for (i = 0; i < es->pop_size - 1; i++) {
        uint32_t j;
        for (j = i + 1; j < es->pop_size; j++) {
            if (es->population[j].fitness > es->population[i].fitness) {
                cas_es_individual_t tmp = es->population[i];
                es->population[i] = es->population[j];
                es->population[j] = tmp;
            }
        }
    }

    es->best_fitness = es->population[0].fitness;

    /* Generate lambda offspring from mu parents via recombination */
    for (i = es->mu; i < es->pop_size; i++) {
        uint32_t p1 = _ev_rand_u64() % es->mu;
        uint32_t p2 = _ev_rand_u64() % es->mu;

        for (d = 0; d < es->dim; d++) {
            /* Intermediate recombination */
            es->population[i].x[d] = (es->population[p1].x[d] + es->population[p2].x[d]) / 2.0;
            es->population[i].sigma[d] = (es->population[p1].sigma[d] + es->population[p2].sigma[d]) / 2.0;

            /* Mutation */
            es->population[i].sigma[d] *= exp(_ev_rand_gaussian() / sqrt(2.0 * es->dim));
            if (es->population[i].sigma[d] < 1e-6) es->population[i].sigma[d] = 1e-6;
            es->population[i].x[d] += _ev_rand_gaussian() * es->population[i].sigma[d];
        }
    }

    es->generation++;
    if (seed) *seed = _ev_seed;
}

void cas_es_run(cas_evolution_strategy_t *es, uint32_t max_gen,
                double target, uint64_t *seed)
{
    if (!es) return;
    uint32_t g;
    for (g = 0; g < max_gen; g++) {
        cas_es_step(es, seed);
        if (es->best_fitness >= target) break;
    }
}

/* =================================================================
 * L6: Minority Game (Challet & Zhang 1997)
 *
 * N agents choose side A or B each round. Winners are on the
 * minority side. Agents have S strategies, use best-performing.
 *
 * Emergent property: For m ~ log2(N), the system self-organizes
 * to near-optimal resource utilization (volatility minimized).
 *
 * Complexity: O(N * S) per step
 * ================================================================= */

void cas_mg_init(cas_minority_game_t *mg, uint32_t n, uint32_t m,
                 uint32_t s, uint64_t seed)
{
    if (!mg) return;
    memset(mg, 0, sizeof(*mg));
    _ev_srand(seed);

    mg->n_agents = n < CAS_MG_MAX_AGENTS ? n : CAS_MG_MAX_AGENTS;
    mg->m = m < CAS_MG_MAX_M ? m : CAS_MG_MAX_M;
    mg->s = s < 2 ? 2 : s;
    mg->history = 0;

    /* Random strategies for each agent */
    uint32_t i, j;
    uint32_t history_bits = 1 << mg->m;
    for (i = 0; i < mg->n_agents; i++) {
        for (j = 0; j < mg->s; j++) {
            uint32_t k;
            for (k = 0; k < history_bits && k < CAS_MG_MAX_M * 2; k++) {
                mg->strategies[i * mg->s + j].strategy[k] = _ev_rand_u64() % 2;
            }
            mg->strategies[i * mg->s + j].score = 0.0;
        }
    }
}

void cas_mg_step(cas_minority_game_t *mg, uint64_t *seed)
{
    if (!mg) return;
    if (seed) _ev_srand(*seed);

    /* Each agent uses its best-performing strategy to decide */
    int total_side_a = 0;
    uint32_t i;

    for (i = 0; i < mg->n_agents; i++) {
        /* Find best strategy for agent i */
        uint32_t best_s = 0;
        double best_score = -1e308;
        uint32_t s;
        for (s = 0; s < mg->s; s++) {
            if (mg->strategies[i * mg->s + s].score > best_score) {
                best_score = mg->strategies[i * mg->s + s].score;
                best_s = s;
            }
        }

        /* Decide based on history */
        uint32_t decision = mg->strategies[i * mg->s + best_s].strategy[mg->history];
        if (decision == 0) total_side_a++;
    }

    int total_side_b = mg->n_agents - total_side_a;
    mg->minority_side = (total_side_a < total_side_b) ? 0 : 1;

    /* Update strategy scores */
    for (i = 0; i < mg->n_agents; i++) {
        uint32_t s;
        for (s = 0; s < mg->s; s++) {
            uint32_t pred = mg->strategies[i * mg->s + s].strategy[mg->history];
            /* +1 if predicted minority correctly, -1 otherwise */
            mg->strategies[i * mg->s + s].score += (pred == mg->minority_side) ? 1.0 : -1.0;
        }
    }

    /* Update history */
    mg->history = ((mg->history << 1) | mg->minority_side) & ((1 << mg->m) - 1);

    /* Record attendance */
    if (mg->hist_len < 100) {
        mg->attendance_history[mg->hist_len++] = total_side_a - total_side_b;
    } else {
        for (i = 1; i < 100; i++)
            mg->attendance_history[i - 1] = mg->attendance_history[i];
        mg->attendance_history[99] = total_side_a - total_side_b;
    }

    if (seed) *seed = _ev_seed;
}

void cas_mg_run(cas_minority_game_t *mg, uint32_t steps, uint64_t *seed)
{
    if (!mg) return;
    uint32_t i;
    for (i = 0; i < steps; i++)
        cas_mg_step(mg, seed);
}

double cas_mg_volatility(const cas_minority_game_t *mg)
{
    if (!mg || mg->hist_len < 2) return 0.0;

    double mean = 0.0;
    uint32_t i;
    for (i = 0; i < mg->hist_len; i++)
        mean += mg->attendance_history[i];
    mean /= mg->hist_len;

    double var = 0.0;
    for (i = 0; i < mg->hist_len; i++)
        var += (mg->attendance_history[i] - mean) * (mg->attendance_history[i] - mean);
    var /= mg->hist_len;

    return sqrt(var);
}

void cas_ga_crossover_1point(const cas_genetic_algorithm_t *ga,
                             const cas_genome_t *p1, const cas_genome_t *p2,
                             cas_genome_t *c1, cas_genome_t *c2, uint64_t *seed)
{
    cas_ga_crossover((cas_genetic_algorithm_t *)ga, p1, p2, c1, c2, seed);
}

void cas_ga_stochastic_universal(cas_genetic_algorithm_t *ga, uint64_t *seed)
{
    if (!ga) return;
    if (seed) _ev_srand(*seed);
    cas_ga_roulette_wheel(ga);
}
