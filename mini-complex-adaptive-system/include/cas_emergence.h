
#ifndef CAS_EMERGENCE_H
#define CAS_EMERGENCE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "cas_agent.h"
#include "cas_network.h"
#include "cas_dynamics.h"

/* =========================================================================
 * Emergence Detection and Measurement (L2-L4)
 *
 * Emergence: macro-level patterns not explicitly encoded at micro level.
 * "The whole is more than the sum of its parts" (Aristotle).
 *
 * L2: Weak vs strong emergence, downward causation, supervenience
 * L3: Information theory (entropy, mutual info, transfer entropy, Phi)
 * L4: Tononi IIT, Hoel causal emergence theorem, Bedau weak emergence
 *
 * Reference:
 *   Bedau, M.A. (1997). Weak Emergence. Phil. Perspectives.
 *   Tononi, G. et al. (2016). IIT 3.0. Nature Rev. Neurosci.
 *   Hoel, E. et al. (2013). Causal Emergence. PNAS 110(49).
 * ========================================================================= */

#define CAS_ENTROPY_BINS_MAX 64
#define CAS_TIME_SERIES_MAX  4096

typedef enum {
    CAS_EMERGENCE_NONE             = 0,
    CAS_EMERGENCE_WEAK_AGGREGATE   = 1,
    CAS_EMERGENCE_WEAK_PATTERN     = 2,
    CAS_EMERGENCE_STRONG_CAUSAL    = 3,
    CAS_EMERGENCE_STRONG_DOWNWARD  = 4
} cas_emergence_level_t;

typedef struct {
    double shannon_entropy;
    double joint_entropy;
    double conditional_entropy;
    double mutual_information;
    double transfer_entropy;
    double active_information;
    double integrated_info;
    double causal_emergence;
    uint32_t n_samples;
} cas_info_measures_t;

typedef struct {
    double *data;
    uint32_t length;
    uint32_t capacity;
    double mean;
    double variance;
    double skewness;
    double kurtosis;
    double hurst_exponent;
    double lyapunov_exponent;
    bool is_stationary;
    bool has_long_memory;
} cas_time_series_t;

typedef struct {
    double macro_effective_info;
    double micro_effective_info;
    double causal_emergence_value;
    uint32_t macro_state_size;
    uint32_t micro_state_size;
    double noise_level;
    bool has_causal_emergence;
} cas_causal_emergence_t;

typedef struct {
    double order_parameter;
    double correlation_length;
    double cluster_size_avg;
    double cluster_size_max;
    double spatial_entropy;
    double temporal_entropy;
    double self_org_index;
    bool is_self_organized;
} cas_self_org_t;

typedef struct {
    double surrogate_error;
    double delay_embedding_dim;
    double false_nearest_neighbors;
    double recurrence_rate;
    double determinism;
    double laminarity;
    double nonlinearity_score;
    bool is_nonlinear;
    bool is_chaotic;
} cas_nonlinearity_t;

/* L5: Information-Theoretic Emergence API */
double cas_entropy_discrete(const double *x, uint32_t n, uint32_t bins,
                            const double *range_min, const double *range_max);
double cas_entropy_joint(const double *x, const double *y, uint32_t n,
                         uint32_t bins_x, uint32_t bins_y);
double cas_mutual_information(const double *x, const double *y,
                              uint32_t n, uint32_t bins);
double cas_entropy_conditional(const double *x, const double *y,
                               uint32_t n, uint32_t bins_x, uint32_t bins_y);
double cas_transfer_entropy(const double *x, const double *y,
                            uint32_t n, uint32_t bins, uint32_t lag);
void cas_info_measures_compute(const double *x, const double *y,
                               uint32_t n, uint32_t bins, uint32_t lag,
                               cas_info_measures_t *measures);

/* L5: Causal Emergence API (Hoel 2013) */
void cas_causal_emergence_compute(cas_causal_emergence_t *ce,
                                  const double *transition_matrix,
                                  uint32_t micro_states,
                                  uint32_t macro_states);
void cas_causal_emergence_optimize(cas_causal_emergence_t *ce,
                                   const double *transition_matrix,
                                   uint32_t micro_states,
                                   uint32_t max_macro_states);

/* L5: Self-Organization API */
void cas_self_org_compute(cas_self_org_t *so, const double *x,
                          const double *y, uint32_t n, uint32_t grid_bins);
bool cas_phase_transition_detect(const double *order_param, uint32_t n,
                                 double *critical_point);

/* L5: Nonlinearity and Chaos Detection */
double cas_false_nearest_neighbors(const double *x, uint32_t n,
                                   uint32_t dim, uint32_t lag, double rtol);
double cas_lyapunov_exponent(const double *x, uint32_t n,
                             uint32_t dim, uint32_t lag);
double cas_hurst_exponent(const double *x, uint32_t n);
void cas_nonlinearity_detect(cas_nonlinearity_t *nl, const double *x,
                             uint32_t n, uint32_t n_surrogates,
                             uint64_t *seed);

/* L5: Time Series Analysis */
void cas_ts_init(cas_time_series_t *ts, uint32_t capacity);
void cas_ts_append(cas_time_series_t *ts, double value);
void cas_ts_analyze(cas_time_series_t *ts);
double cas_ts_autocorrelation(const cas_time_series_t *ts, uint32_t lag);
double cas_ts_partial_autocorr(const cas_time_series_t *ts, uint32_t lag);
void cas_ts_detrend(cas_time_series_t *ts);
void cas_ts_destroy(cas_time_series_t *ts);

/* L5: Integrated Information for CAS */
double cas_integrated_information(const cas_population_t *pop);
double cas_causal_density(const cas_network_t *net);

#endif /* CAS_EMERGENCE_H */
