/**
 * @file cas_landscape.c
 * @brief NK Fitness Landscape Implementation (L1-L6)
 *
 * Kauffman's NK fitness landscape model -- the canonical framework
 * for studying how adaptation creates complexity on rugged landscapes.
 *
 * L1: NK landscape struct, genotype, fitness evaluation
 * L2: Ruggedness, adaptive walks, local optima, epistasis
 * L3: Boolean hypercube, Hamming distance, correlation function
 * L4: Kauffman's complexity catastrophe, phase transitions, walk length
 * L5: Landscape generation, adaptive walk, statistics, estimation
 * L6: NK landscape optimization, basin analysis
 *
 * Reference:
 *   Kauffman, S.A. (1993). The Origins of Order. Oxford.
 *   Weinberger, E.D. (1990). Biol. Cybern. 63:325-336.
 *   Kauffman, S.A. & Levin, S. (1987). J. Theor. Biol. 128:11-45.
 */

#include "cas_landscape.h"
#include <string.h>
#include <stdlib.h>

/* ---- Internal PRNG (xoshiro256**) ---- */
static uint64_t _nk_seed[4] = {12345, 67890, 13579, 24680};

static uint64_t _nk_rotl(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

static uint64_t _nk_rand_u64(void) {
    uint64_t result = _nk_rotl(_nk_seed[1] * 5, 7) * 9;
    uint64_t t = _nk_seed[1] << 17;
    _nk_seed[2] ^= _nk_seed[0];
    _nk_seed[3] ^= _nk_seed[1];
    _nk_seed[1] ^= _nk_seed[2];
    _nk_seed[0] ^= _nk_seed[3];
    _nk_seed[2] ^= t;
    _nk_seed[3] = _nk_rotl(_nk_seed[3], 45);
    return result;
}

static void _nk_srand(uint64_t seed) {
    _nk_seed[0] = seed;
    _nk_seed[1] = seed ^ 0x9e3779b97f4a7c15ULL;
    _nk_seed[2] = seed ^ 0xbf58476d1ce4e5b9ULL;
    _nk_seed[3] = seed ^ 0x94d049bb133111ebULL;
    for (int i = 0; i < 20; i++) _nk_rand_u64();
}

static double _nk_rand_double(void) {
    return (double)(_nk_rand_u64() >> 11) * 0x1.0p-53;
}

static uint32_t _nk_rand_uint32(void) {
    return (uint32_t)(_nk_rand_u64() >> 32);
}

/* ---- Bit Utilities ---- */
static bool _nk_get_bit(const uint8_t g[4], uint32_t pos) {
    return (g[pos / 8] >> (pos % 8)) & 1;
}

static void _nk_set_bit(uint8_t g[4], uint32_t pos, bool val) {
    if (val)
        g[pos / 8] |= (1 << (pos % 8));
    else
        g[pos / 8] &= ~(1 << (pos % 8));
}

static void _nk_toggle_bit(uint8_t g[4], uint32_t pos) {
    g[pos / 8] ^= (1 << (pos % 8));
}

/* =================================================================
 * L3: NK Landscape Initialization
 *
 * Generate random epistatic interaction matrix and fitness table.
 * Each locus i interacts with K other randomly chosen loci.
 * Fitness contributions are uniform random [0,1) for each
 * possible configuration of the K+1 interacting loci.
 *
 * Theorem (Kauffman 1993): The NK model with K = N-1 is
 * equivalent to Derrida's random energy model. The expected
 * number of local optima is 2^N / (N+1).
 *
 * Complexity: O(N * K + 2^(K+1))
 * ================================================================= */

void cas_nk_init(cas_nk_landscape_t *land, uint32_t n, uint32_t k,
                 uint64_t seed)
{
    if (!land) return;

    _nk_srand(seed);

    land->n = (n <= CAS_NK_MAX_N) ? n : CAS_NK_MAX_N;
    land->k = (k < land->n) ? k : (land->n - 1);

    /* Generate epistatic interaction matrix */
    uint32_t i, j;
    for (i = 0; i < land->n; i++) {
        /* Each locus interacts with itself + K others */
        for (j = 0; j < land->k; j++) {
            uint32_t partner;
            bool duplicate;
            do {
                partner = _nk_rand_uint32() % land->n;
                duplicate = false;
                /* Check self-interaction */
                if (partner == i) { duplicate = true; continue; }
                /* Check duplicate in current row */
                uint32_t jj;
                for (jj = 0; jj < j; jj++)
                    if (land->interactions[i][jj] == partner)
                        { duplicate = true; break; }
            } while (duplicate);
            land->interactions[i][j] = partner;
        }
    }

    /* Generate fitness table: 2^(K+1) entries per locus */
    uint32_t table_size = 1 << (land->k + 1);
    if (table_size > 65536) table_size = 65536;

    for (i = 0; i < table_size; i++) {
        land->fitness_table[i] = _nk_rand_double();
    }
}

/* =================================================================
 * L3: NK Fitness Evaluation
 *
 * f(g) = (1/N) * sum_i f_i(g_i, g_{j1}, ..., g_{jK})
 *
 * For each locus i, extract the interacting bits to form an
 * index into the fitness table.
 *
 * Complexity: O(N * K)
 * ================================================================= */

double cas_nk_evaluate(const cas_nk_landscape_t *land,
                       const uint8_t genotype[4])
{
    if (!land || !genotype) return 0.0;

    double total = 0.0;
    uint32_t i;

    for (i = 0; i < land->n; i++) {
        /* Build index from interacting bits */
        uint32_t index = 0;
        /* Bit 0: the locus itself */
        if (_nk_get_bit(genotype, i)) index |= 1;

        uint32_t j;
        for (j = 0; j < land->k; j++) {
            if (_nk_get_bit(genotype, land->interactions[i][j]))
                index |= (1 << (j + 1));
        }

        total += land->fitness_table[index];
    }

    return total / (double)land->n;
}

/* =================================================================
 * L3: Full Fitness Evaluation with per-locus contributions
 *
 * Same as cas_nk_evaluate but also stores per-locus fitness
 * contributions in point->fitness_contrib[] for analysis.
 *
 * Complexity: O(N * K)
 * ================================================================= */

void cas_nk_evaluate_full(const cas_nk_landscape_t *land,
                          cas_landscape_point_t *point)
{
    if (!land || !point) return;

    double total = 0.0;
    uint32_t i;

    for (i = 0; i < land->n; i++) {
        uint32_t index = 0;
        if (_nk_get_bit(point->genotype, i)) index |= 1;

        uint32_t j;
        for (j = 0; j < land->k; j++) {
            if (_nk_get_bit(point->genotype,
                            land->interactions[i][j]))
                index |= (1 << (j + 1));
        }

        double contrib = land->fitness_table[index];
        point->fitness_contrib[i] = contrib;
        total += contrib;
    }

    point->fitness = total / (double)land->n;
    point->n = land->n;
}

/* =================================================================
 * L4: Local Optimum Detection
 *
 * A genotype is a local optimum if all 1-mutant neighbors
 * have lower or equal fitness.
 *
 * Theorem (Kauffman 1993): For K = 0 (Mount Fuji), there is
 * exactly 1 local optimum (the global optimum). As K -> N-1,
 * the number of local optima approaches 2^N / (N+1).
 *
 * Complexity: O(N^2)
 * ================================================================= */

bool cas_nk_is_local_optimum(const cas_nk_landscape_t *land,
                             const cas_landscape_point_t *point)
{
    if (!land || !point) return false;

    double current_fit = point->fitness;
    uint8_t mutant[4];
    memcpy(mutant, point->genotype, 4);

    uint32_t i;
    for (i = 0; i < land->n; i++) {
        _nk_toggle_bit(mutant, i);
        double mutant_fit = cas_nk_evaluate(land, mutant);
        _nk_toggle_bit(mutant, i);  /* restore */

        if (mutant_fit > current_fit)
            return false;
    }

    return true;
}

/* =================================================================
 * L5: Adaptive Walk Step
 *
 * Take one greedy step: evaluate all N 1-mutant neighbors,
 * move to the one with highest fitness. Stop if no improvement.
 *
 * Theorem (Kauffman 1993): Expected adaptive walk length to
 * local optimum: E[L] ~ ln(N) for K << N. As K increases,
 * walks become shorter because the landscape becomes more rugged.
 *
 * Complexity: O(N^2)
 * ================================================================= */

void cas_nk_adaptive_walk_step(const cas_nk_landscape_t *land,
                               cas_landscape_point_t *point)
{
    if (!land || !point) return;

    uint8_t best_mutant[4];
    memcpy(best_mutant, point->genotype, 4);
    double best_fit = point->fitness;
    bool improved = false;
    uint32_t i;

    for (i = 0; i < land->n; i++) {
        _nk_toggle_bit(best_mutant, i);
        double mut_fit = cas_nk_evaluate(land, best_mutant);
        _nk_toggle_bit(best_mutant, i);

        if (mut_fit > best_fit) {
            best_fit = mut_fit;
            /* Store the improved genotype */
            uint8_t temp[4];
            memcpy(temp, point->genotype, 4);
            _nk_toggle_bit(temp, i);
            memcpy(best_mutant, temp, 4);
            improved = true;
        }
    }

    if (improved) {
        memcpy(point->genotype, best_mutant, 4);
        point->fitness = best_fit;
        point->is_local_optimum = false;
    } else {
        point->is_local_optimum = true;
    }
}

/* =================================================================
 * L4: Fitness Correlation Function (Weinberger 1990)
 *
 * rho(d) = 1 - (d/N) * (K+1)/(N-1)
 *
 * The fitness correlation between genotypes at Hamming distance d.
 * Measures landscape smoothness: higher correlation -> smoother.
 *
 * Theorem: For K = N-1, rho(1) = 0 (uncorrelated landscape).
 * For K = 0, rho(1) = 1 - 1/(N*(N-1)) (highly correlated).
 *
 * Complexity: O(1)
 * ================================================================= */

double cas_nk_fitness_correlation(uint32_t n, uint32_t k,
                                  uint32_t hamming_dist)
{
    if (n <= 1) return 1.0;
    double rho = 1.0 - ((double)hamming_dist / (double)n)
                       * ((double)(k + 1) / (double)(n - 1));
    if (rho < 0.0) rho = 0.0;
    if (rho > 1.0) rho = 1.0;
    return rho;
}

/* =================================================================
 * L3: Random genotype generation
 *
 * Fill genotype with random bits using PRNG.
 *
 * Complexity: O(1) for N <= 32
 * ================================================================= */

void cas_nk_random_genotype(const cas_nk_landscape_t *land,
                            uint8_t genotype[4], uint64_t *seed)
{
    if (!land || !genotype) return;

    if (seed) _nk_srand(*seed);

    uint32_t bits = land->n;
    genotype[0] = (uint8_t)(_nk_rand_u64() & 0xFF);
    genotype[1] = (uint8_t)((_nk_rand_u64() >> 8) & 0xFF);
    genotype[2] = (uint8_t)((_nk_rand_u64() >> 16) & 0xFF);
    genotype[3] = (uint8_t)((_nk_rand_u64() >> 24) & 0xFF);

    /* Zero out bits beyond N */
    if (bits < 32) {
        uint32_t full_bytes = bits / 8;
        uint32_t rem_bits = bits % 8;
        if (full_bytes < 4) {
            uint32_t b;
            for (b = full_bytes + 1; b < 4; b++)
                genotype[b] = 0;
            if (full_bytes < 4)
                genotype[full_bytes] &= (uint8_t)((1 << rem_bits) - 1);
        }
    }

    if (seed) *seed = _nk_seed[0];
}

/* =================================================================
 * L3: Random 1-bit flip
 *
 * Flip a uniformly random bit in the genotype.
 * Returns the position that was flipped.
 *
 * Complexity: O(1)
 * ================================================================= */

uint32_t cas_nk_flip_bit(const cas_nk_landscape_t *land,
                         uint8_t genotype[4], uint64_t *seed)
{
    if (!land || !genotype) return 0;

    if (seed) _nk_srand(*seed);

    uint32_t pos = _nk_rand_uint32() % land->n;
    _nk_toggle_bit(genotype, pos);

    if (seed) *seed = _nk_seed[0];
    return pos;
}

/* =================================================================
 * L3: Hamming Distance
 *
 * Count the number of differing bits between two genotypes.
 *
 * Complexity: O(1) for N <= 32 (population count of XOR)
 * ================================================================= */

uint32_t cas_nk_hamming_distance(const uint8_t a[4], const uint8_t b[4],
                                 uint32_t n)
{
    if (!a || !b) return 0;

    uint32_t dist = 0;
    int i;
    for (i = 0; i < 4; i++) {
        uint8_t diff = a[i] ^ b[i];
        /* Popcount per byte */
        diff = (diff & 0x55) + ((diff >> 1) & 0x55);
        diff = (diff & 0x33) + ((diff >> 2) & 0x33);
        dist += (diff & 0x0F) + ((diff >> 4) & 0x0F);
    }

    if (dist > n) dist = n;
    return dist;
}

/* =================================================================
 * L5: Landscape Statistics
 *
 * Sample the landscape to estimate:
 *   - moments of fitness distribution
 *   - local optima count
 *   - autocorrelation of random walk
 *   - information content
 *
 * Complexity: O(n_samples * N^2)
 * ================================================================= */

void cas_nk_compute_stats(const cas_nk_landscape_t *land,
                          uint32_t n_samples,
                          cas_landscape_stats_t *stats,
                          uint64_t *seed)
{
    if (!land || !stats) return;

    if (seed) _nk_srand(*seed);
    memset(stats, 0, sizeof(cas_landscape_stats_t));

    double sum = 0.0, sum2 = 0.0, sum3 = 0.0, sum4 = 0.0;
    uint32_t local_count = 0;
    uint8_t genotype[4];
    uint32_t i;

    for (i = 0; i < n_samples; i++) {
        cas_nk_random_genotype(land, genotype, NULL);
        double fit = cas_nk_evaluate(land, genotype);

        sum += fit;
        sum2 += fit * fit;
        sum3 += fit * fit * fit;
        sum4 += fit * fit * fit * fit;

        /* Check local optimality (expensive, do on subset) */
        if (i < 100 || i % 10 == 0) {
            cas_landscape_point_t pt;
            memcpy(pt.genotype, genotype, 4);
            pt.fitness = fit;
            if (cas_nk_is_local_optimum(land, &pt))
                local_count++;
        }
    }

    double n = (double)n_samples;
    double mean = sum / n;
    double var = sum2 / n - mean * mean;
    double std = sqrt(var);

    stats->mean_fitness = mean;
    stats->variance_fitness = var;
    stats->skewness = std > 1e-12
        ? (sum3 / n - 3 * mean * var - mean * mean * mean)
          / (std * std * std) : 0.0;
    stats->kurtosis = var > 1e-12
        ? (sum4 / n - 4 * mean * (sum3 / n)
           + 6 * mean * mean * (sum2 / n)
           - 3 * mean * mean * mean * mean) / (var * var) - 3.0
        : 0.0;

    /* Information content (entropy-based ruggedness) */
    double ic = 0.0;
    uint32_t freq[2] = {0, 0};
    for (i = 0; i < n_samples && i < 1000; i++) {
        cas_nk_random_genotype(land, genotype, NULL);
        double f1 = cas_nk_evaluate(land, genotype);
        uint32_t bp = _nk_rand_uint32() % land->n;
        _nk_toggle_bit(genotype, bp);
        double f2 = cas_nk_evaluate(land, genotype);
        if (f2 > f1) freq[1]++; else freq[0]++;
    }
    double pf = (double)freq[1] / (double)(freq[0] + freq[1] + 1);
    ic = -pf * log(pf + 1e-12) - (1.0 - pf) * log(1.0 - pf + 1e-12);
    stats->information_content = ic;

    /* Random walk autocorrelation */
    double ac_sum = 0.0;
    uint32_t walk_steps = 100;
    if (walk_steps > n_samples / 2) walk_steps = n_samples / 2;
    if (walk_steps < 2) walk_steps = 2;

    uint8_t wg[4];
    cas_nk_random_genotype(land, wg, NULL);
    double prev_fit = cas_nk_evaluate(land, wg);
    for (i = 0; i < walk_steps; i++) {
        cas_nk_flip_bit(land, wg, NULL);
        double curr_fit = cas_nk_evaluate(land, wg);
        ac_sum += prev_fit * curr_fit;
        prev_fit = curr_fit;
    }
    stats->autocorrelation = (ac_sum / walk_steps - mean * mean)
                             / (var + 1e-12);

    stats->local_optima_count = local_count;

    if (seed) *seed = _nk_seed[0];
}

/* =================================================================
 * L5: Local Optima Estimation
 *
 * Estimate number of local optima via random sampling.
 * Unbiased estimator: count_in_sample / n_samples * 2^N.
 *
 * Complexity: O(n_samples * N^2)
 * ================================================================= */

uint32_t cas_nk_estimate_local_optima(const cas_nk_landscape_t *land,
                                      uint32_t n_samples,
                                      uint64_t *seed)
{
    if (!land) return 0;

    if (seed) _nk_srand(*seed);

    uint32_t count = 0;
    uint8_t genotype[4];
    uint32_t i;

    for (i = 0; i < n_samples; i++) {
        cas_nk_random_genotype(land, genotype, NULL);
        cas_landscape_point_t pt;
        memcpy(pt.genotype, genotype, 4);
        pt.fitness = cas_nk_evaluate(land, genotype);
        if (cas_nk_is_local_optimum(land, &pt))
            count++;
    }

    if (seed) *seed = _nk_seed[0];

    /* Extrapolate to full space */
    double fraction = (double)count / (double)n_samples;
    double estimate = fraction * pow(2.0, (double)land->n);
    return (uint32_t)(estimate + 0.5);
}

/* =================================================================
 * L5: Landscape Ruggedness via Random Walk Autocorrelation
 *
 * Low autocorrelation => high ruggedness.
 * Complexity: O(walk_length * N^2)
 * ================================================================= */

double cas_nk_ruggedness(const cas_nk_landscape_t *land,
                         uint32_t walk_length, uint64_t *seed)
{
    if (!land) return 0.0;

    if (seed) _nk_srand(*seed);

    uint8_t genotype[4];
    cas_nk_random_genotype(land, genotype, NULL);

    double *fitnesses = (double *)malloc(walk_length * sizeof(double));
    if (!fitnesses) return 0.0;

    uint8_t mut[4];
    uint32_t i;
    for (i = 0; i < walk_length; i++) {
        fitnesses[i] = cas_nk_evaluate(land, genotype);
        memcpy(mut, genotype, 4);
        cas_nk_flip_bit(land, mut, NULL);
        cas_nk_evaluate(land, mut);
        memcpy(genotype, mut, 4);
    }

    /* Compute lag-1 autocorrelation */
    double mean = 0.0, var = 0.0;
    for (i = 0; i < walk_length; i++) mean += fitnesses[i];
    mean /= walk_length;
    for (i = 0; i < walk_length; i++)
        var += (fitnesses[i] - mean) * (fitnesses[i] - mean);
    var /= walk_length;

    double ac = 0.0;
    for (i = 0; i < walk_length - 1; i++)
        ac += (fitnesses[i] - mean) * (fitnesses[i + 1] - mean);
    ac /= (walk_length - 1);

    free(fitnesses);

    if (seed) *seed = _nk_seed[0];

    if (var < 1e-12) return 1.0;
    double rug = 1.0 - fabs(ac / var);
    return rug;
}

/* =================================================================
 * L4: Adaptation Velocity (Complexity Catastrophe)
 *
 * Measure adaptation speed: v = E[f(t+1) - f(t)] over walks.
 *
 * Theorem (Kauffman 1993): As N and K increase, adaptation
 * velocity decreases. The complexity catastrophe states that
 * beyond a critical K/N ratio, adaptive walks become trapped
 * in poor local optima, and increasing complexity reduces
 * attainable fitness.
 *
 * Complexity: O(n_walks * walk_len * N^2)
 * ================================================================= */

double cas_nk_adaptation_velocity(const cas_nk_landscape_t *land,
                                  uint32_t n_walks, uint32_t walk_len,
                                  uint64_t *seed)
{
    if (!land) return 0.0;

    if (seed) _nk_srand(*seed);

    double total_velocity = 0.0;
    uint32_t w;

    for (w = 0; w < n_walks; w++) {
        uint8_t genotype[4];
        cas_nk_random_genotype(land, genotype, NULL);
        double prev_fit = cas_nk_evaluate(land, genotype);
        uint32_t s;

        for (s = 0; s < walk_len; s++) {
            cas_landscape_point_t pt;
            memcpy(pt.genotype, genotype, 4);
            pt.fitness = prev_fit;
            pt.is_local_optimum = false;

            cas_nk_adaptive_walk_step(land, &pt);
            double new_fit = pt.fitness;

            total_velocity += (new_fit - prev_fit);
            prev_fit = new_fit;
            memcpy(genotype, pt.genotype, 4);

            if (pt.is_local_optimum) break;
        }
    }

    if (seed) *seed = _nk_seed[0];

    return total_velocity / (double)(n_walks * walk_len);
}

/* =================================================================
 * L5: Continuous Landscape Functions
 * ================================================================= */

void cas_cont_landscape_init(cas_continuous_landscape_t *land, uint32_t dim,
                             const double *bounds_min,
                             const double *bounds_max,
                             double (*f)(const double *, uint32_t, void *),
                             void *data)
{
    if (!land) return;

    land->dim = (dim <= CAS_CONT_LANDSCAPE_DIM_MAX)
                ? dim : CAS_CONT_LANDSCAPE_DIM_MAX;
    if (bounds_min) {
        uint32_t i;
        for (i = 0; i < land->dim; i++)
            land->bounds_min[i] = bounds_min[i];
    }
    if (bounds_max) {
        uint32_t i;
        for (i = 0; i < land->dim; i++)
            land->bounds_max[i] = bounds_max[i];
    }
    land->fitness_func = f;
    land->func_data = data;
}

void cas_cont_landscape_gradient(const cas_continuous_landscape_t *land,
                                 const double *x, double *grad, double h)
{
    if (!land || !x || !grad) return;

    uint32_t i;
    for (i = 0; i < land->dim; i++) {
        double x_plus[8], x_minus[8];
        uint32_t j;
        for (j = 0; j < land->dim; j++) {
            x_plus[j] = x[j];
            x_minus[j] = x[j];
        }
        x_plus[i] += h;
        x_minus[i] -= h;

        double fp = land->fitness_func(x_plus, land->dim, land->func_data);
        double fm = land->fitness_func(x_minus, land->dim, land->func_data);
        grad[i] = (fp - fm) / (2.0 * h);
    }
}

void cas_cont_landscape_gradient_ascent(const cas_continuous_landscape_t *land,
                                        double *x, uint32_t max_iter,
                                        double lr, double tol)
{
    if (!land || !x) return;

    double grad[8];
    uint32_t iter;
    double prev_f = land->fitness_func(x, land->dim, land->func_data);

    for (iter = 0; iter < max_iter; iter++) {
        cas_cont_landscape_gradient(land, x, grad, 1e-6);

        /* Adaptive step with backtracking */
        double step = lr;
        double best_f = prev_f;
        double best_x[8];
        uint32_t j;
        for (j = 0; j < land->dim; j++) best_x[j] = x[j];

        int try_i;
        for (try_i = 0; try_i < 10; try_i++) {
            double trial_x[8];
            uint32_t k;
            for (k = 0; k < land->dim; k++)
                trial_x[k] = x[k] + step * grad[k];

            /* Project to bounds */
            for (k = 0; k < land->dim; k++) {
                if (trial_x[k] < land->bounds_min[k])
                    trial_x[k] = land->bounds_min[k];
                if (trial_x[k] > land->bounds_max[k])
                    trial_x[k] = land->bounds_max[k];
            }

            double trial_f = land->fitness_func(trial_x, land->dim,
                                                land->func_data);
            if (trial_f > best_f) {
                best_f = trial_f;
                for (k = 0; k < land->dim; k++) best_x[k] = trial_x[k];
            }
            step *= 0.5;
        }

        for (j = 0; j < land->dim; j++) x[j] = best_x[j];

        if (fabs(best_f - prev_f) < tol) break;
        prev_f = best_f;
    }
}
