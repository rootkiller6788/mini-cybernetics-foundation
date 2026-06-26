#ifndef CAS_LANDSCAPE_H
#define CAS_LANDSCAPE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* =========================================================================
 * NK Fitness Landscapes (L1-L4)
 *
 * Kauffman's NK model -- canonical mathematical framework for studying
 * how adaptation creates complexity on rugged fitness landscapes.
 *
 * f: {0,1}^N -> R; each locus i contributes fitness based on itself
 * and K other loci (epistatic interactions).
 *
 * K=0: Mount Fuji landscape (single peak, smooth gradient)
 * K=N-1: Random energy model (maximally rugged, many local optima)
 *
 * L1: NK landscape, fitness, epistasis, genotype
 * L2: Ruggedness, adaptive walk, local optimum, complexity catastrophe
 * L3: Boolean hypercube, Hamming distance, correlation function
 * L4: Kauffman theorems on walk length, local optima density
 *
 * Reference: Kauffman (1993) Origins of Order. Oxford.
 * Course: MIT 6.883, SFI Complexity Explorer, Cambridge Part III
 * ========================================================================= */

#define CAS_NK_MAX_N  32
#define CAS_NK_MAX_K  16

/* L3: Landscape statistics */
typedef struct {
    double   mean_fitness;
    double   variance_fitness;
    double   skewness;
    double   kurtosis;
    uint32_t local_optima_count;
    double   autocorrelation;
    double   information_content;
    double   fitness_distance_correlation;
    uint32_t basin_size_max;
    double   basin_size_mean;
} cas_landscape_stats_t;

/* L3: Point on Boolean hypercube */
typedef struct {
    uint8_t  genotype[4];
    double   fitness;
    double   fitness_contrib[CAS_NK_MAX_N];
    uint32_t n;
    bool     is_local_optimum;
} cas_landscape_point_t;

/* L3: NK landscape structure */
typedef struct {
    uint32_t n;
    uint32_t k;
    uint32_t interactions[CAS_NK_MAX_N][CAS_NK_MAX_K];
    double   fitness_table[65536];
} cas_nk_landscape_t;

/* L3: Continuous fitness landscape */
#define CAS_CONT_LANDSCAPE_DIM_MAX  8

typedef struct {
    uint32_t dim;
    double   bounds_min[CAS_CONT_LANDSCAPE_DIM_MAX];
    double   bounds_max[CAS_CONT_LANDSCAPE_DIM_MAX];
    double   (*fitness_func)(const double *x, uint32_t dim, void *data);
    void    *func_data;
} cas_continuous_landscape_t;

/* L5: NK Landscape API */
void cas_nk_init(cas_nk_landscape_t *land, uint32_t n, uint32_t k,
                 uint64_t seed);
double cas_nk_evaluate(const cas_nk_landscape_t *land,
                       const uint8_t genotype[4]);
void cas_nk_evaluate_full(const cas_nk_landscape_t *land,
                          cas_landscape_point_t *point);
bool cas_nk_is_local_optimum(const cas_nk_landscape_t *land,
                             const cas_landscape_point_t *point);
void cas_nk_adaptive_walk_step(const cas_nk_landscape_t *land,
                               cas_landscape_point_t *point);
double cas_nk_fitness_correlation(uint32_t n, uint32_t k,
                                  uint32_t hamming_dist);
void cas_nk_random_genotype(const cas_nk_landscape_t *land,
                            uint8_t genotype[4], uint64_t *seed);
uint32_t cas_nk_flip_bit(const cas_nk_landscape_t *land,
                         uint8_t genotype[4], uint64_t *seed);
uint32_t cas_nk_hamming_distance(const uint8_t a[4], const uint8_t b[4],
                                 uint32_t n);

void cas_nk_compute_stats(const cas_nk_landscape_t *land,
                          uint32_t n_samples, cas_landscape_stats_t *stats,
                          uint64_t *seed);
uint32_t cas_nk_estimate_local_optima(const cas_nk_landscape_t *land,
                                      uint32_t n_samples, uint64_t *seed);
double cas_nk_ruggedness(const cas_nk_landscape_t *land,
                         uint32_t walk_length, uint64_t *seed);
double cas_nk_adaptation_velocity(const cas_nk_landscape_t *land,
                                  uint32_t n_walks, uint32_t walk_len,
                                  uint64_t *seed);

/* L5: Continuous landscape API */
void cas_cont_landscape_init(cas_continuous_landscape_t *land, uint32_t dim,
                             const double *bounds_min, const double *bounds_max,
                             double (*f)(const double *, uint32_t, void *),
                             void *data);
void cas_cont_landscape_gradient(const cas_continuous_landscape_t *land,
                                 const double *x, double *grad, double h);
void cas_cont_landscape_gradient_ascent(const cas_continuous_landscape_t *land,
                                        double *x, uint32_t max_iter,
                                        double lr, double tol);

#endif /* CAS_LANDSCAPE_H */
