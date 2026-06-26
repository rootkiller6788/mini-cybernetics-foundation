/**
 * @file cas_dynamics.c
 * @brief CAS Population Dynamics Implementation (L1-L6)
 *
 * Collective dynamics of interacting agents produce emergent
 * system-level behaviors: phase transitions, self-organized
 * criticality, co-evolution, and information cascades.
 *
 * L1: Population, co-evolution structs, sandpile model
 * L2: Population dynamics, emergence, phase transitions
 * L3: Master equation, mean-field theory, Langevin dynamics
 * L4: Bak-Tang-Wiesenfeld SOC, Fisher's theorem, Price equation
 * L5: Population stepping, selection, reproduction, mutation
 * L6: Sandpile power-law detection, emergence metrics
 *
 * Reference:
 *   Bak, P. (1996). How Nature Works. Copernicus.
 *   Nowak, M.A. (2006). Evolutionary Dynamics. Harvard.
 *   Helbing, D. (2010). Quantitative Sociodynamics. Springer.
 */

#include "cas_dynamics.h"
#include <string.h>
#include <stdlib.h>

static uint64_t _dyn_seed = 78901;

static uint64_t _dyn_rand_u64(void) {
    _dyn_seed = _dyn_seed * 6364136223846793005ULL + 1442695040888963407ULL;
    uint64_t z = _dyn_seed ^ (_dyn_seed >> 33);
    z = z * 0xff51afd7ed558ccdULL;
    z = z ^ (z >> 33);
    z = z * 0xc4ceb9fe1a85ec53ULL;
    return z ^ (z >> 33);
}
static void _dyn_srand(uint64_t s) { _dyn_seed = s; }
static double _dyn_rand_double(void) {
    return (double)(_dyn_rand_u64() >> 11) * 0x1.0p-53;
}
static double _dyn_rand_gaussian(void) {
    double u1 = _dyn_rand_double(), u2 = _dyn_rand_double();
    if (u1 < 1e-12) u1 = 1e-12;
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

/* =================================================================
 * L5: Population Initialization
 *
 * Create a population of CAS agents ready for simulation.
 * Each agent gets a unique ID and random spatial position.
 *
 * Complexity: O(pop_size * state_dim)
 * ================================================================= */

void cas_pop_init(cas_population_t *pop, uint32_t size,
                  cas_agent_type_t default_type, uint32_t state_dim,
                  uint64_t seed)
{
    if (!pop) return;
    memset(pop, 0, sizeof(cas_population_t));
    _dyn_srand(seed);

    pop->size = size < CAS_POP_MAX_SIZE ? size : CAS_POP_MAX_SIZE;
    uint32_t i;
    for (i = 0; i < pop->size; i++) {
        cas_agent_init(&pop->agents[i], i, default_type, state_dim);
        /* Add diversity in adaptation strategy */
        pop->agents[i].adapt_strategy = (cas_adapt_strategy_t)(_dyn_rand_u64() % 6);
        pop->agents[i].mutation_rate = _dyn_rand_double() * 0.1;
    }

    cas_net_init(&pop->network, pop->size, CAS_NET_ERDOS_RENYI, _dyn_rand_u64());
    pop->step = 0;
}

/* =================================================================
 * L5: Population Step
 *
 * Execute one time step for the whole population:
 *   1. Each agent senses, decides, acts, adapts
 *   2. Compute population-level statistics
 *
 * Complexity: O(pop_size * state_dim * num_effectors)
 * ================================================================= */

void cas_pop_step(cas_population_t *pop, double *environment,
                  uint32_t env_dim, uint64_t *seed)
{
    if (!pop) return;
    if (seed) _dyn_srand(*seed);

    uint32_t i;
    for (i = 0; i < pop->size; i++) {
        double reward = _dyn_rand_gaussian() * 0.1;
        cas_agent_step(&pop->agents[i], environment, env_dim, reward);
    }

    cas_pop_compute_stats(pop);
    pop->step++;

    if (seed) *seed = _dyn_seed;
}

void cas_pop_run(cas_population_t *pop, double *environment,
                 uint32_t env_dim, uint32_t n_steps, uint64_t *seed)
{
    if (!pop) return;
    uint32_t i;
    for (i = 0; i < n_steps; i++)
        cas_pop_step(pop, environment, env_dim, seed);
}

/* =================================================================
 * L5: Population Selection (natural selection within CAS)
 *
 * Tournament selection removes low-fitness agents and replaces
 * them with offspring of high-fitness agents.
 *
 * Complexity: O(pop_size * state_dim)
 * ================================================================= */

void cas_pop_selection(cas_population_t *pop, uint64_t *seed)
{
    if (!pop) return;
    if (seed) _dyn_srand(*seed);

    /* Sort agents by fitness (selection sort on pointers) */
    cas_agent_t *sorted[CAS_POP_MAX_SIZE];
    uint32_t i;
    for (i = 0; i < pop->size; i++)
        sorted[i] = &pop->agents[i];

    /* Simple O(n^2) sort -- n is small */
    uint32_t j;
    for (i = 0; i < pop->size; i++) {
        uint32_t best = i;
        for (j = i + 1; j < pop->size; j++)
            if (sorted[j]->fitness > sorted[best]->fitness)
                best = j;
        cas_agent_t *tmp = sorted[i];
        sorted[i] = sorted[best];
        sorted[best] = tmp;
    }

    /* Bottom half replaced by offspring of top half */
    uint32_t half = pop->size / 2;
    for (i = half; i < pop->size; i++) {
        uint32_t p1_idx = _dyn_rand_u64() % half;
        uint32_t p2_idx = _dyn_rand_u64() % half;
        cas_agent_t *p1 = sorted[p1_idx];
        cas_agent_t *p2 = sorted[p2_idx];

        /* Simple crossover of model state */
        cas_agent_t *child = sorted[i];
        uint32_t dim = child->model.state_dim;
        uint32_t d;
        for (d = 0; d < dim; d++) {
            child->model.state[d] = (_dyn_rand_double() < 0.5)
                ? p1->model.state[d] : p2->model.state[d];
        }
        child->fitness = 0.0;
        child->age = 0;
    }

    pop->births += (pop->size - half);
    pop->deaths += (pop->size - half);

    if (seed) *seed = _dyn_seed;
}

void cas_pop_reproduction(cas_population_t *pop, uint64_t *seed)
{
    if (!pop) return;
    if (seed) _dyn_srand(*seed);

    cas_agent_t **ptrs = (cas_agent_t **)malloc(pop->size * sizeof(cas_agent_t *));
    if (!ptrs) return;
    uint32_t i;
    for (i = 0; i < pop->size; i++) ptrs[i] = &pop->agents[i];

    uint32_t new_count = pop->size / 4;
    for (i = 0; i < new_count && pop->size < CAS_POP_MAX_SIZE; i++) {
        uint32_t pi = cas_agent_tournament_select(ptrs, pop->size, 3);
        cas_agent_t *child = &pop->agents[pop->size];
        memcpy(child, &pop->agents[pi], sizeof(cas_agent_t));
        child->id = pop->size;
        child->age = 0;
        child->energy = child->max_energy * 0.5;
        pop->size++;
        pop->births++;
    }
    free(ptrs);
    if (seed) *seed = _dyn_seed;
}

void cas_pop_mutation(cas_population_t *pop, double rate, uint64_t *seed)
{
    if (!pop) return;
    if (seed) _dyn_srand(*seed);

    uint32_t i, d;
    for (i = 0; i < pop->size; i++) {
        if (_dyn_rand_double() < rate) {
            cas_agent_t *a = &pop->agents[i];
            for (d = 0; d < a->model.state_dim; d++) {
                a->model.state[d] += _dyn_rand_gaussian() * 0.1;
            }
            a->mutation_rate = fabs(a->mutation_rate + _dyn_rand_gaussian() * 0.01);
            if (a->mutation_rate > 0.5) a->mutation_rate = 0.5;
        }
    }
    if (seed) *seed = _dyn_seed;
}

/* =================================================================
 * L5: Population Statistics
 *
 * Compute mean fitness, variance, diversity, entropy,
 * cooperation ratio, and total energy.
 *
 * Complexity: O(pop_size * state_dim)
 * ================================================================= */

void cas_pop_compute_stats(cas_population_t *pop)
{
    if (!pop || pop->size == 0) return;

    double sum_fit = 0.0, sum_fit2 = 0.0, sum_energy = 0.0;
    uint32_t i, j, coop = 0;

    for (i = 0; i < pop->size; i++) {
        double f = pop->agents[i].fitness;
        sum_fit += f;
        sum_fit2 += f * f;
        sum_energy += pop->agents[i].energy;

        /* Cooperation heuristic: agents with similar tags */
        for (j = 0; j < pop->agents[i].num_tags; j++) {
            if (pop->agents[i].tags[j].group_id > 0) {
                coop++;
                break;
            }
        }
    }

    double n = (double)pop->size;
    pop->mean_fitness = sum_fit / n;
    pop->fitness_variance = sum_fit2 / n - pop->mean_fitness * pop->mean_fitness;
    pop->energy_total = sum_energy;
    pop->cooperation_ratio = (double)coop / n;

    /* Diversity index: Simpson's 1 - sum(p_i^2) on tags */
    double tag_freq[32] = {0};
    for (i = 0; i < pop->size; i++) {
        uint32_t gid = pop->agents[i].group_id % 32;
        tag_freq[gid]++;
    }
    double simpson = 0.0;
    for (i = 0; i < 32; i++) {
        double p = tag_freq[i] / n;
        simpson += p * p;
    }
    pop->diversity_index = 1.0 - simpson;

    /* Shannon entropy of fitness distribution */
    uint32_t bins[20] = {0};
    for (i = 0; i < pop->size; i++) {
        int bin = (int)((pop->agents[i].fitness + 5.0) / 0.5);
        if (bin < 0) bin = 0;
        if (bin >= 20) bin = 19;
        bins[bin]++;
    }
    double entropy = 0.0;
    for (i = 0; i < 20; i++) {
        if (bins[i] > 0) {
            double p = (double)bins[i] / n;
            entropy -= p * log(p);
        }
    }
    pop->entropy = entropy;
}

/* =================================================================
 * L5: Co-evolution Dynamics
 *
 * Models pairwise co-evolution between two species/agents.
 * Tracks interaction history to classify relationship type.
 *
 * Red Queen effect: both species must constantly evolve to maintain
 * relative fitness (Van Valen 1973).
 *
 * Complexity: O(1) per update
 * ================================================================= */

void cas_coevolve_init(cas_coevolution_t *ce, uint32_t a, uint32_t b)
{
    if (!ce) return;
    memset(ce, 0, sizeof(cas_coevolution_t));
    ce->agent_a = a; ce->agent_b = b;
    ce->is_mutualistic = false;
    ce->is_competitive = false;
    ce->is_predator_prey = false;
}

void cas_coevolve_update(cas_coevolution_t *ce, double payoff_a, double payoff_b)
{
    if (!ce) return;

    if (ce->history_len < 100) {
        ce->interaction_history[ce->history_len++] = payoff_a - payoff_b;
    } else {
        uint32_t i;
        for (i = 1; i < 100; i++)
            ce->interaction_history[i - 1] = ce->interaction_history[i];
        ce->interaction_history[99] = payoff_a - payoff_b;
    }

    ce->fitness_a = 0.9 * ce->fitness_a + 0.1 * payoff_a;
    ce->fitness_b = 0.9 * ce->fitness_b + 0.1 * payoff_b;
}

int cas_coevolve_classify(cas_coevolution_t *ce)
{
    if (!ce || ce->history_len < 10) return 0;

    double sum_diff = 0.0;
    uint32_t i;
    for (i = 0; i < ce->history_len; i++)
        sum_diff += ce->interaction_history[i];

    double avg_diff = sum_diff / ce->history_len;

    if (fabs(avg_diff) < 0.1) {
        if (ce->fitness_a > 0 && ce->fitness_b > 0) {
            ce->is_mutualistic = true;
            return 1;
        }
        ce->is_competitive = true;
        return 2;
    } else if (avg_diff > 0.5) {
        ce->is_predator_prey = true;
        return 3;
    } else if (avg_diff < -0.5) {
        ce->is_predator_prey = true;
        return 3;
    }

    ce->is_competitive = true;
    return 2;
}

double cas_coevolve_red_queen_effect(const cas_coevolution_t *ce)
{
    if (!ce || ce->history_len < 2) return 0.0;

    /* Red Queen effect: rate of fitness change needed to stay in place */
    double total_abs_change = 0.0;
    uint32_t i;
    for (i = 1; i < ce->history_len; i++)
        total_abs_change += fabs(ce->interaction_history[i] - ce->interaction_history[i - 1]);

    return total_abs_change / (ce->history_len - 1);
}

/* =================================================================
 * L5: Master Equation (Probability Distribution Dynamics)
 *
 * dP_i/dt = sum_j (W_{j->i} P_j - W_{i->j} P_i)
 *
 * Complexity: O(n_states^2) per step
 * ================================================================= */

void cas_master_eq_init(cas_master_eq_t *me, uint32_t num_states,
                        const double *initial_prob)
{
    if (!me) return;
    memset(me, 0, sizeof(cas_master_eq_t));
    me->num_states = num_states < CAS_POP_MAX_SIZE ? num_states : CAS_POP_MAX_SIZE;
    if (initial_prob) {
        uint32_t i;
        for (i = 0; i < me->num_states; i++)
            me->prob[i] = initial_prob[i];
    }
}

void cas_master_eq_step(cas_master_eq_t *me, double dt)
{
    if (!me) return;

    double new_prob[CAS_POP_MAX_SIZE];
    uint32_t i, j;

    for (i = 0; i < me->num_states; i++) {
        double dp = 0.0;
        for (j = 0; j < me->num_states; j++) {
            if (i == j) continue;
            double W_ji = me->transition_matrix[j * me->num_states + i];
            double W_ij = me->transition_matrix[i * me->num_states + j];
            dp += W_ji * me->prob[j] - W_ij * me->prob[i];
        }
        new_prob[i] = me->prob[i] + dt * dp;
        if (new_prob[i] < 0.0) new_prob[i] = 0.0;
    }

    /* Normalize */
    double total = 0.0;
    for (i = 0; i < me->num_states; i++) total += new_prob[i];
    if (total > 1e-12) {
        for (i = 0; i < me->num_states; i++)
            me->prob[i] = new_prob[i] / total;
    }
}

void cas_master_eq_steady_state(cas_master_eq_t *me, double tol,
                                uint32_t max_iter)
{
    if (!me) return;
    uint32_t iter;
    for (iter = 0; iter < max_iter; iter++) {
        double prev[CAS_POP_MAX_SIZE];
        uint32_t i;
        for (i = 0; i < me->num_states; i++) prev[i] = me->prob[i];

        cas_master_eq_step(me, 0.1);

        double err = 0.0;
        for (i = 0; i < me->num_states; i++)
            err += fabs(me->prob[i] - prev[i]);

        if (err < tol) break;
    }
}

double cas_master_eq_entropy(const cas_master_eq_t *me)
{
    if (!me) return 0.0;
    double H = 0.0;
    uint32_t i;
    for (i = 0; i < me->num_states; i++)
        if (me->prob[i] > 1e-12)
            H -= me->prob[i] * log(me->prob[i]);
    return H;
}

/* =================================================================
 * L5: Mean-Field Approximation
 *
 * Assumes all agents experience the same average environment.
 * Detects phase transitions via order parameter variance.
 *
 * Complexity: O(n)
 * ================================================================= */

void cas_mean_field_compute(cas_mean_field_t *mf,
                            const double *observations, uint32_t n,
                            double control_param)
{
    if (!mf || !observations || n == 0) return;
    memset(mf, 0, sizeof(*mf));

    double sum = 0.0, sum2 = 0.0, sum4 = 0.0;
    uint32_t i;
    for (i = 0; i < n; i++) {
        sum += observations[i];
        sum2 += observations[i] * observations[i];
        sum4 += observations[i] * observations[i] * observations[i] * observations[i];
    }

    mf->order_parameter = sum / n;
    mf->susceptibility = n * (sum2 / n - mf->order_parameter * mf->order_parameter);

    /* Binder cumulant for phase transition detection */
    double m2 = sum2 / n;
    double m4 = sum4 / n;
    mf->correlation_length = 1.0 - m4 / (3.0 * m2 * m2 + 1e-12);
    mf->critical_point = control_param;
    mf->is_ordered_phase = fabs(mf->order_parameter) > 0.5;
}

bool cas_mean_field_is_critical(const cas_mean_field_t *mf)
{
    if (!mf) return false;
    /* Peak in susceptibility indicates critical point */
    return mf->susceptibility > 2.0;
}

/* =================================================================
 * L5: Langevin Dynamics (Stochastic Differential Equation)
 *
 * dx/dt = -dV/dx - friction * v + noise(t)
 *
 * Used to model individual agents or order parameters under
 * noise and dissipation.
 *
 * Complexity: O(1) per step
 * ================================================================= */

void cas_langevin_init(cas_langevin_t *l, double temp, double friction,
                       double noise)
{
    if (!l) return;
    memset(l, 0, sizeof(*l));
    l->temperature = temp;
    l->friction = friction;
    l->noise_intensity = noise;
}

void cas_langevin_step(cas_langevin_t *l, double dt, uint64_t *seed)
{
    if (!l) return;
    if (seed) _dyn_srand(*seed);

    /* Harmonic potential: V(x) = 0.5 * x^2 */
    double force = -l->position;
    double noise_term = _dyn_rand_gaussian() * sqrt(2.0 * l->temperature * l->friction * dt);

    l->velocity += (force - l->friction * l->velocity) * dt + noise_term;
    l->position += l->velocity * dt;
    l->potential_energy = 0.5 * l->position * l->position;

    if (seed) *seed = _dyn_seed;
}

double cas_langevin_escape_rate(const cas_langevin_t *l, double barrier_height)
{
    if (!l) return 0.0;
    /* Kramers' escape rate: r ~ exp(-barrier/T) */
    return (l->friction / (2.0 * M_PI)) * exp(-barrier_height / l->temperature);
}

/* =================================================================
 * L4: Bak-Tang-Wiesenfeld Sandpile Model (Self-Organized Criticality)
 *
 * A canonical model of SOC: gradual driving (adding sand) interspersed
 * with avalanches (toppling when slope exceeds threshold).
 *
 * Theorem (Bak, Tang, Wiesenfeld 1987, PRL):
 *   The sandpile self-organizes to a critical state where avalanche
 *   sizes follow a power law P(s) ~ s^{-tau} with tau ~ 1.1 in 2D.
 *   This occurs without any fine-tuning of parameters.
 *
 * Complexity: O(grid_size^2 * n_drops) worst case
 * ================================================================= */

void cas_sandpile_init(cas_sandpile_t *sp, uint32_t size)
{
    if (!sp) return;
    memset(sp, 0, sizeof(*sp));
    sp->size = size < CAS_SANDPILE_SIZE ? size : CAS_SANDPILE_SIZE;
}

void cas_sandpile_topple(cas_sandpile_t *sp, uint32_t x, uint32_t y)
{
    if (!sp) return;
    if (x >= sp->size || y >= sp->size) return;

    sp->grid[x][y] -= 4;
    sp->total_topplings++;

    /* Distribute to neighbors */
    if (x > 0) sp->grid[x - 1][y]++;
    if (x < sp->size - 1) sp->grid[x + 1][y]++;
    if (y > 0) sp->grid[x][y - 1]++;
    if (y < sp->size - 1) sp->grid[x][y + 1]++;
}

void cas_sandpile_drop(cas_sandpile_t *sp, uint64_t *seed)
{
    if (!sp) return;
    if (seed) _dyn_srand(*seed);

    /* Drop grain at random position */
    uint32_t x = _dyn_rand_u64() % sp->size;
    uint32_t y = _dyn_rand_u64() % sp->size;
    sp->grid[x][y]++;

    /* Avalanche: topple all sites with >= 4 grains */
    uint64_t avalanche_size = 0;
    bool changed = true;
    while (changed) {
        changed = false;
        uint32_t i, j;
        for (i = 0; i < sp->size; i++) {
            for (j = 0; j < sp->size; j++) {
                if (sp->grid[i][j] >= 4) {
                    cas_sandpile_topple(sp, i, j);
                    avalanche_size++;
                    changed = true;
                }
            }
        }
    }

    /* Record avalanche size */
    sp->total_avalanches++;
    if (avalanche_size > 0 && avalanche_size < 100) {
        if (sp->dist_len < 100) {
            sp->avalanche_size_dist[sp->dist_len++] = (double)avalanche_size;
        } else {
            uint32_t d;
            for (d = 1; d < 100; d++)
                sp->avalanche_size_dist[d - 1] = sp->avalanche_size_dist[d];
            sp->avalanche_size_dist[99] = (double)avalanche_size;
        }
    }

    if (seed) *seed = _dyn_seed;
}

void cas_sandpile_run(cas_sandpile_t *sp, uint64_t n_drops, uint64_t *seed)
{
    if (!sp) return;
    uint64_t i;
    for (i = 0; i < n_drops; i++)
        cas_sandpile_drop(sp, seed);
}

double cas_sandpile_powerlaw_exponent(const cas_sandpile_t *sp)
{
    if (!sp || sp->dist_len < 10) return 0.0;

    /* Log-log regression: log(P(s)) = -tau * log(s) + C */
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;
    uint32_t n = sp->dist_len;
    uint32_t i;

    for (i = 0; i < n; i++) {
        double s = sp->avalanche_size_dist[i];
        if (s < 1.0) s = 1.0;
        double x = log(s);
        double y = log(1.0 / (double)n);  /* simplified: use rank/frequency */

        uint32_t rank = 1;
        uint32_t j;
        for (j = 0; j < n; j++)
            if (sp->avalanche_size_dist[j] > s) rank++;
        y = log((double)rank / (double)n);

        sum_x += x; sum_y += y;
        sum_xy += x * y; sum_x2 += x * x;
    }

    double num = n * sum_xy - sum_x * sum_y;
    double den = n * sum_x2 - sum_x * sum_x;
    if (fabs(den) < 1e-12) return 0.0;

    return -num / den;  /* tau = -slope */
}

/* =================================================================
 * L5: Emergence Detection
 *
 * Compute emergence metrics for a population:
 *   nonlinearity_index: deviation from linear additivity
 *   self_organization_index: order / disorder ratio
 *   complexity_metric: product of order and disorder (Shalizi)
 *
 * Complexity: O(pop_size^2)
 * ================================================================= */

void cas_emergence_detect(const cas_population_t *pop,
                          cas_emergence_metrics_t *metrics)
{
    if (!pop || !metrics) return;
    memset(metrics, 0, sizeof(*metrics));

    /* Nonlinearity: how much variance is not explained by linear model */
    double total_var = pop->fitness_variance;
    double linear_var = pop->mean_fitness * pop->mean_fitness;
    metrics->nonlinearity_index = total_var > 1e-12
        ? 1.0 - linear_var / (total_var + linear_var) : 0.0;

    /* Self-organization: agent spatial clustering */
    metrics->self_organization_index = pop->cooperation_ratio;

    /* Complexity (Shalizi): product of entropy and mutual information proxy */
    metrics->complexity_metric = pop->entropy * pop->diversity_index;

    /* Emergence measure: aggregate predictability vs individual */
    metrics->emergence_measure = 1.0 - pop->mean_fitness / (pop->fitness_variance + 1.0);

    /* Mutual information proxy via correlation */
    metrics->mutual_information = pop->fitness_variance / (total_var + 1e-12);

    /* Integrated information proxy: network connectedness */
    metrics->integrated_information = pop->network.is_connected
        ? 1.0 / pop->network.avg_path_length : 0.0;

    /* Causal density: edge density */
    uint32_t max_edges = pop->network.num_nodes * (pop->network.num_nodes - 1) / 2;
    metrics->causal_density = max_edges > 0
        ? (double)pop->network.num_edges / max_edges : 0.0;

    /* Emergent levels: count distinct emergent patterns */
    metrics->emergent_levels = (uint32_t)(metrics->nonlinearity_index * 10.0);
    if (metrics->self_organization_index > 0.5) metrics->emergent_levels++;
    if (metrics->complexity_metric > 1.0) metrics->emergent_levels++;
}
