#ifndef SOS_CRITICALITY_H
#define SOS_CRITICALITY_H

#include "sos_core.h"

/* ============================================================================
 * Self-Organized Criticality (SOC)
 *
 * Bak, Tang & Wiesenfeld (1987, 1988): "Self-Organized Criticality:
 * An Explanation of 1/f Noise"
 *
 * Key insight: composite systems naturally evolve to a critical state
 * where avalanches of all sizes occur with power-law distributions,
 * without any external tuning of parameters to a critical point.
 *
 * Core observables:
 *   - Power-law avalanche size distribution: P(s) ~ s^{-tau}, tau ~ 1.0-1.5
 *   - 1/f noise: S(f) ~ f^{-alpha}, alpha ~ 1
 *   - Fractal spatial structure: D_f = fractal dimension
 *
 * Bak-Sneppen model (1993): evolutionary self-organized criticality
 * where species with lowest fitness are replaced, driving the system
 * to a critical state with punctuated equilibrium dynamics.
 * ============================================================================ */

/* --- SOC Observable Statistics --- */
typedef struct {
    double  tau_avalanche_size;     /* Exponent of P(s) ~ s^{-tau} */
    double  tau_avalanche_lifetime;  /* Exponent of P(T) ~ T^{-tau_T} */
    double  alpha_psd;              /* 1/f^alpha noise exponent */
    double  fractal_dimension;       /* D_f of avalanche clusters */
    double  cutoff_size;            /* Finite-size cutoff s_max */
    int     n_avalanches;           /* Total avalanches recorded */
    double  mean_size;              /* Mean avalanche size */
    double  variance_size;          /* Variance of avalanche sizes */
} SOCStatistics;

/* --- Bak-Sneppen Evolution Model (1993) --- */
typedef struct {
    int     n_species;              /* Number of species on a 1D ring */
    double* fitness;               /* Fitness values f_i in [0,1] */
    int*    species_id;            /* Species identifiers */
    double  threshold;             /* Fitness threshold for replacement */
    int     n_extinctions;         /* Total extinction events */
    int*    extinction_sizes;      /* History of extinction avalanche sizes */
    int     ext_history_len;
    long long total_time;
} BakSneppenModel;

/* --- Forest Fire Model (Drossel & Schwabl 1992) --- */
typedef struct {
    int     width;
    int     height;
    int**   state;           /* 0=empty, 1=tree, 2=burning */
    double  p_growth;        /* Probability of tree growth per step */
    double  p_lightning;     /* Probability of lightning strike */
    int     n_trees;
    int     n_burning;
    int     n_burned;
    int*    fire_sizes;      /* History of fire sizes */
    int     fire_history_len;
} ForestFireModel;

/* ===== Bak-Sneppen Model Functions ===== */

BakSneppenModel* bs_create(int n_species);
void bs_free(BakSneppenModel* bs);

/** Initialize with random fitness values in [0,1]. */
void bs_randomize(BakSneppenModel* bs);

/** Perform one update step:
 *  1. Find species with minimum fitness and its two neighbors
 *  2. Replace all three with new random fitness values
 *  Returns: number of species replaced (size of this "avalanche") */
int bs_step(BakSneppenModel* bs);

/** Run n_steps, recording avalanche sizes.
 *  The Bak-Sneppen model self-organizes to a critical state
 *  where the fitness threshold f_c ~ 0.667 (exact value: 2/3 = 0.6666...). */
void bs_run(BakSneppenModel* bs, int n_steps);

/** Get the current fitness threshold (minimum fitness in the system). */
double bs_fitness_threshold(const BakSneppenModel* bs);

/** Check if system has reached the critical state.
 *  Criteria: fitness threshold has converged to ~0.667
 *  and avalanche size distribution follows a power law. */
int bs_is_critical(const BakSneppenModel* bs);

/** Compute avalanche size distribution P(s). */
void bs_avalanche_distribution(const BakSneppenModel* bs,
                               double* sizes, double* probs, int n_bins);

/* ===== Forest Fire Model Functions ===== */

ForestFireModel* ff_create(int width, int height, double p_growth, double p_lightning);
void ff_free(ForestFireModel* ff);

/** Evolve one timestep:
 *  1. Empty cells grow trees with probability p_growth
 *  2. Trees are struck by lightning with probability p_lightning
 *  3. Burning trees ignite neighboring trees
 *  4. Burning trees become empty
 *  Returns: size of fire (if any) */
int ff_step(ForestFireModel* ff);

/** Run full simulation with periodic output. */
void ff_run(ForestFireModel* ff, int n_steps);

/** Get fire size distribution. */
void ff_fire_distribution(const ForestFireModel* ff,
                          double* sizes, double* probs, int n_bins);

/* ===== SOC Analysis (Model-Agnostic) ===== */

/** Test whether a distribution follows a power law.
 *  Uses the Kolmogorov-Smirnov statistic between the empirical CDF
 *  and the best-fit power-law CDF (Clauset, Shalizi, Newman 2009). */
int soc_test_powerlaw(const double* values, int n, double x_min,
                      double* p_value);

/** Compute the 1/f noise exponent from a time series.
 *  Fits P(f) = C * f^{-alpha} using linear regression on log-log data. */
double soc_noise_exponent(const double* time_series, int n, double dt);

/** Estimate the finite-size scaling exponent:
 *  <s>(L) ~ L^{gamma} where L = system size, <s> = mean avalanche size. */
double soc_finite_size_scaling(const double* mean_sizes,
                               const double* system_sizes, int n);

/** Compute the avalanche shape collapse.
 *  Sethna, Dahmen, Myers (2001): Avalanche shapes for different durations
 *  collapse onto a universal scaling function:
 *    V(t, T) ~ T^{gamma-1} * F(t/T)
 *  where V = avalanche velocity, T = duration. */
double soc_avalanche_shape_collapse(const double** avalanche_profiles,
                                    const double* durations,
                                    int n_avalanches, int max_duration);

#endif /* SOS_CRITICALITY_H */
